
#include <netif/ppp/polarssl/sha1.h>
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>
#include <pico/stdlib.h>
#include <stdbool.h>

#include "websocket.h"
#include "http_server.h"
#include "fpanel.h"

#define WS_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define BASE64_ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

#define WS_OPCODE_CONT    0
#define WS_OPCODE_TEXT    1
#define WS_OPCODE_BINARY  2
#define WS_OPCODE_CLOSE   8
#define WS_OPCODE_PING    9
#define WS_OPCODE_PONG    10

#define WS_SEND_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define WS_RECV_TASK_PRIORITY (tskIDLE_PRIORITY + 3UL)

#define WS_PING_PERIOD  1000
#define WS_PING_TIMEOUT 3000

#define WS_OUT_MSG_BUF_SIZE 300


#define WS_LOG_LEVEL 4

#ifdef WS_LOG_LEVEL
# define WS_LOG(level, ...) do { if (level <= WS_LOG_LEVEL) { printf(__VA_ARGS__); } } while (0)
#endif

typedef struct
{
    uint8_t  length;
    bool     fin;
    uint8_t  opcode;
    bool     masking;
    uint64_t payload_len;
    uint8_t  mask[4];

} ws_header_t;


static TaskHandle_t ws_send_task_handle = NULL;
static TaskHandle_t ws_recv_task_handle = NULL;

static MessageBufferHandle_t out_msg_buf = NULL;

static void base64(const uint8_t *buf, uint16_t buf_len,
                   uint8_t *output, uint16_t *outputlen)
{
    uint8_t pad = (3 - (buf_len % 3)) % 3;
    uint8_t b0, b1, b2;
    uint32_t idx = 0;

    *outputlen = (buf_len + pad) / 3 * 4;

    for (uint32_t i = 0; i < (buf_len + pad); i += 3)
    {
        b0 = (i + 0) >= buf_len ? 0 : buf[i + 0];
        b1 = (i + 1) >= buf_len ? 0 : buf[i + 1];
        b2 = (i + 2) >= buf_len ? 0 : buf[i + 2];

        output[idx++] = BASE64_ALPHABET[b0 >> 2];
        output[idx++] = BASE64_ALPHABET[((b0 & 3) << 4) | (b1 >> 4)];
        output[idx++] = BASE64_ALPHABET[((b1 & 0x0f) << 2) | (b2 >> 6)];
        output[idx++] = BASE64_ALPHABET[b2 & 0x3f];
    }

    for (uint32_t i = *outputlen - pad; i < *outputlen; i++)
    {
        output[i] = '=';
    }

}


static void send_message(struct netconn *conn, const uint8_t *data,
                         uint32_t data_len, uint8_t opcode, uint8_t fin)
{
    static uint8_t hdr[10] = {0};
    uint8_t hdrlen;

    hdr[0] = (fin << 7) | opcode;

    if (data_len <= 125)
    {
        hdrlen = 2;
        hdr[1] = (uint8_t)data_len;
    }
    else if (data_len < (1 << 16))
    {
        hdrlen = 4;
        hdr[1] = 126;
        hdr[2] = (data_len >> 8) & 0xff;
        hdr[3] = data_len & 0xff;
    }
    else
    {
        hdrlen = 10;
        hdr[1] = 127;

        for (uint8_t i = 2, j = 7; i < 10; i++, j--)
        {
            hdr[i] = (data_len >> (8 * j)) & 0xff;
        }
    };

    netconn_write(conn, hdr, hdrlen, NETCONN_NOCOPY);

    while (data_len)
    {
        uint32_t wsize = MIN(1024, data_len);
        netconn_write(conn, data, wsize, NETCONN_NOCOPY);
        data = data + wsize;
        data_len -= wsize;
    }
}


static void send_ping(struct netconn *conn)
{
    WS_LOG(4, "Websocket: sending PING\n");
    send_message(conn, NULL, 0, WS_OPCODE_PING, 1);
}


static void ws_send_task(void *params)
{
    static uint8_t buf[128];
    struct netconn *conn = (struct netconn*)params;
    size_t data_len;

    /* Send initial ping */
    send_ping(conn);

    for (;;)
    {
        data_len = xMessageBufferReceive(out_msg_buf, buf, sizeof(buf), 100);

        if (data_len)
        {
            send_message(conn, buf, data_len, WS_OPCODE_TEXT, 1);
        }

        if (ulTaskNotifyTake(pdTRUE, 0))
        {
            send_ping(conn);
        }
    }

}

static bool ws_read_header(const uint8_t *buf, uint16_t buf_len,
                           ws_header_t *hdr)
{
    if (buf_len < 2)
    {
        return false;
    }

    hdr->length = 2;

    hdr->fin = (buf[0] & 0x80) ? true : false;
    hdr->opcode = buf[0] & 0x0f;
    hdr->masking = (buf[1] & 0x80) ? true : false;
    hdr->payload_len = buf[1] & 0x7f;

    if (hdr->masking)
    {
        hdr->length += 4;

        if (hdr->length > buf_len)
        {
            return false;
        }
    }

    if (hdr->payload_len == 126)
    {
        hdr->length += 2;

        if (hdr->length > buf_len)
        {
            return false;
        }

        hdr->payload_len = (buf[2] << 8) | buf[3];
    }
    else if (hdr->payload_len == 127)
    {
        hdr->length += 8;

        if (hdr->length > buf_len)
        {
            return false;
        }

        hdr->payload_len = 0;
        
        for (uint8_t i = 2, j = 7; i < 10; i++, j--)
        {
            hdr->payload_len |= buf[i] << (8 * j);
        }
    }

    if (hdr->masking)
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            hdr->mask[i] = buf[hdr->length - 4 + i];
        }
    }

    WS_LOG(4, "Websocket header:\n");
    WS_LOG(4, "Fin:            %u\n", hdr->fin);
    WS_LOG(4, "Opcode:         %u\n", hdr->opcode);
    WS_LOG(4, "Payload length: %llu\n", hdr->payload_len);
    WS_LOG(4, "Masking:        %u\n", hdr->masking);
    
    if (hdr->masking)
    {
        WS_LOG(4, "Mask:           0x%02x 0x%02x 0x%02x 0x%02x\n",
               hdr->mask[0], hdr->mask[1], hdr->mask[2], hdr->mask[3]);
    }

    if (hdr->fin == false || hdr->opcode == WS_OPCODE_CONT)
    {
        WS_LOG(1, "Websocket: error: multi-frame messages are not supported\n");
        return false;
    }

    if ((buf_len - hdr->length) < hdr->payload_len)
    {
        WS_LOG(1, "Websocket: error: message length exceeds maximum\n");
        return false;
    }

    return true;
}


static bool dispatch_ws_message(struct netconn *conn, const ws_header_t *hdr,
                                uint8_t *buf, bool *close)
{
    bool res = true;

    switch (hdr->opcode)
    {
    
    case WS_OPCODE_TEXT:
        {
            WS_LOG(4, "Websocket message: ");
            for (uint64_t i = 0; i < hdr->payload_len; i++)
            {
                buf[i] ^= hdr->mask[i % 4];
                WS_LOG(4, "%c", buf[i]);
            }
            WS_LOG(4, "\n");
        
            fpanel_send(buf, hdr->payload_len, 100);
        }
        break;

    case WS_OPCODE_BINARY:
        WS_LOG(1, "Websocket: a binary message received");
        break;

    case WS_OPCODE_PING:
        send_message(conn, buf, 0, WS_OPCODE_PONG, 1);
        break;

    case WS_OPCODE_PONG:
        WS_LOG(4, "Websocket: PONG received\n");
        break;

    case WS_OPCODE_CLOSE:
        *close = true;
        break;

    default:
        res = false;
        break;
    }

    return res;
}


static void ws_recv_task(void *params)
{
    struct netconn *conn = (struct netconn*)params;
    struct netbuf *inbuf = NULL;
    uint8_t *buf;
    uint16_t buf_len;
    err_t err;
    bool close = false;
    TickType_t last_time = xTaskGetTickCount(), cur_time;

    netconn_set_recvtimeout(conn, WS_PING_PERIOD);

    for (;;)
    {
        err = netconn_recv(conn, &inbuf);

        if (ERR_OK == err)
        {
            netbuf_data(inbuf, (void**)&buf, &buf_len);
            
            WS_LOG(4, "Websocket: received a message, raw length: %u\n", buf_len);

            ws_header_t ws_hdr;

            if (ws_read_header(buf, buf_len, &ws_hdr))
            {
                if (!dispatch_ws_message(conn, &ws_hdr,
                                         buf + (uint16_t)ws_hdr.length, &close))
                {
                    WS_LOG(1, "Websocket: error dispatching message\n");
                }
            }
            else
            {
                WS_LOG(1, "Websocket: error: malformed header, or unsupported message type\n");
            }

            if (close)
            {
                WS_LOG(1, "Websocket: connection closed by client\n");
                break;
            }

            netbuf_delete(inbuf);
            last_time = xTaskGetTickCount();
        }
        else if (ERR_TIMEOUT == err)
        {
            cur_time = xTaskGetTickCount();
            
            if ((cur_time - last_time) > WS_PING_TIMEOUT)
            {
                WS_LOG(1, "Websocket: closing connection due to client ping timeout\n");
                break;
            }
            else if ((cur_time - last_time) > WS_PING_PERIOD)
            {
                /* Make the send task to issue a ping */
                xTaskNotifyGive(ws_send_task_handle);
            }
        }
    }

    if (ws_send_task_handle != NULL)
    {
        vTaskSuspend(ws_send_task_handle);
        vTaskDelete(ws_send_task_handle);
    }

    netconn_close(conn);
    netconn_delete(conn);

    vMessageBufferDelete(out_msg_buf);
    out_msg_buf = NULL;
    
    ws_send_task_handle = NULL;
    ws_recv_task_handle = NULL;

    vTaskDelete(NULL);
}


bool websocket_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay)
{
    if (out_msg_buf != NULL)
    {
        return (xMessageBufferSend(out_msg_buf, msg, msg_len, max_delay) > 0);
    }

    return false;
}


void websocket_handshake(struct netconn *conn,
                         const uint8_t *sec_key,
                         uint16_t sec_key_len)
{
    uint8_t sha_buf[20];
    uint8_t accept_key[40];
    uint16_t accept_key_len;

    sha1_context shactx;

    if (ws_send_task_handle != NULL || ws_recv_task_handle != NULL)
    {
        printf("Websocket: error - exceeded max number(1) of websocket connections\n");
        netconn_write(conn, HTTP_RESP_CODE_500, CSTR_SIZE(HTTP_RESP_CODE_500), NETCONN_NOCOPY);
        netconn_write(conn, HTTP_NEWLINE, CSTR_SIZE(HTTP_NEWLINE), NETCONN_NOCOPY);
        netconn_write(conn, HTTP_NEWLINE, CSTR_SIZE(HTTP_NEWLINE), NETCONN_NOCOPY);

        netconn_close(conn);
        netconn_delete(conn);

        return;
    }

    sha1_starts(&shactx);

    sha1_update(&shactx, sec_key, sec_key_len);
    sha1_update(&shactx, WS_MAGIC_STRING, sizeof(WS_MAGIC_STRING) - 1);
    sha1_finish(&shactx, sha_buf);

    base64(sha_buf, sizeof(sha_buf), accept_key, &accept_key_len);

    netconn_write(conn, HTTP_RESP_CODE_101, CSTR_SIZE(HTTP_RESP_CODE_101), NETCONN_NOCOPY);
    netconn_write(conn, HTTP_UPGRADE_WEBSOCKET, CSTR_SIZE(HTTP_UPGRADE_WEBSOCKET), NETCONN_NOCOPY);
    netconn_write(conn, HTTP_NEWLINE, CSTR_SIZE(HTTP_NEWLINE), NETCONN_NOCOPY);
    netconn_write(conn, HTTP_CONNECTION_UPGRADE, CSTR_SIZE(HTTP_CONNECTION_UPGRADE), NETCONN_NOCOPY);
    netconn_write(conn, HTTP_NEWLINE, CSTR_SIZE(HTTP_NEWLINE), NETCONN_NOCOPY);
    netconn_write(conn, HTTP_SCE_WEBSOCKET_ACCEPT, CSTR_SIZE(HTTP_SCE_WEBSOCKET_ACCEPT), NETCONN_NOCOPY);
    netconn_write(conn, accept_key, accept_key_len, NETCONN_NOCOPY);
    netconn_write(conn, HTTP_NEWLINE, CSTR_SIZE(HTTP_NEWLINE), NETCONN_NOCOPY);
    netconn_write(conn, HTTP_NEWLINE, CSTR_SIZE(HTTP_NEWLINE), NETCONN_NOCOPY);

    if (out_msg_buf == NULL)
    {
        out_msg_buf = xMessageBufferCreate(WS_OUT_MSG_BUF_SIZE);
    }

    xTaskCreate(ws_send_task,
                "ws_send",
                1024 * 8,
                conn,
                WS_SEND_TASK_PRIORITY,
                &ws_send_task_handle);

    xTaskCreate(ws_recv_task,
                "ws_recv",
                1024 * 8,
                conn,
                WS_RECV_TASK_PRIORITY,
                &ws_recv_task_handle);

}