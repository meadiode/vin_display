
#include "websocket.h"
#include "http_server.h"
#include "netif/ppp/polarssl/sha1.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"

#include <stdbool.h>

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

static void base64(const uint8_t *buf, uint16_t buflen,
                   uint8_t *output, uint16_t *outputlen)
{
    uint8_t pad = (3 - (buflen % 3)) % 3;
    uint8_t b0, b1, b2;
    uint32_t idx = 0;

    *outputlen = (buflen + pad) / 3 * 4;

    for (uint32_t i = 0; i < (buflen + pad); i += 3)
    {
        b0 = (i + 0) >= buflen ? 0 : buf[i + 0];
        b1 = (i + 1) >= buflen ? 0 : buf[i + 1];
        b2 = (i + 2) >= buflen ? 0 : buf[i + 2];

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
                         uint32_t datalen, uint8_t opcode, uint8_t fin)
{
    uint8_t hdr[10] = {0};
    uint8_t hdrlen;

    hdr[0] = (fin << 7) | opcode;

    if (datalen <= 125)
    {
        hdrlen = 2;
        hdr[1] = (uint8_t)datalen;
    }
    else if (datalen < (1 << 16))
    {
        hdrlen = 4;
        hdr[1] = 126;
        hdr[2] = (datalen >> 8) & 0xff;
        hdr[3] = datalen & 0xff;
    }
    else
    {
        hdrlen = 10;
        hdr[1] = 127;

        for (uint8_t i = 2, j = 7; i < 10; i++, j--)
        {
            hdr[i] = (datalen >> (8 * j)) & 0xff;
        }
    };

    netconn_write(conn, hdr, hdrlen, NETCONN_NOCOPY);

    while (datalen)
    {
        uint32_t wsize = MIN(1024, datalen);
        netconn_write(conn, data, wsize, NETCONN_NOCOPY);
        data = data + wsize;
        datalen -= wsize;
    }
}

static void ws_send_task(void *params)
{
    struct netconn *conn = (struct netconn*)params;


    for (;;)
    {
        vTaskDelay(5000);
        send_message(conn, NULL, 0, WS_OPCODE_PING, 1);
    }

}

static bool read_ws_header(const uint8_t *buf, uint16_t buflen,
                           ws_header_t *hdr)
{
    if (buflen < 2)
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

        if (hdr->length > buflen)
        {
            return false;
        }
    }

    if (hdr->payload_len == 126)
    {
        hdr->length += 2;

        if (hdr->length > buflen)
        {
            return false;
        }

        hdr->payload_len = (buf[2] << 8) | buf[3];
    }
    else if (hdr->payload_len == 127)
    {
        hdr->length += 8;

        if (hdr->length > buflen)
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

    printf("Websocket header:\n");
    printf("Fin:            %u\n", hdr->fin);
    printf("Opcode:         %u\n", hdr->opcode);
    printf("Payload length: %llu\n", hdr->payload_len);
    printf("Masking:        %u\n", hdr->masking);
    
    if (hdr->masking)
    {
        printf("Mask:           0x%02x 0x%02x 0x%02x 0x%02x\n",
               hdr->mask[0], hdr->mask[1], hdr->mask[2], hdr->mask[3]);
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
            printf("Websocket message: ");
            for (uint64_t i = 0; i < hdr->payload_len; i++)
            {
                printf("%c", buf[i] ^ hdr->mask[i % 4]);
            }

            printf("\n");
        }
        break;

    case WS_OPCODE_BINARY:
        printf("Websocket: a binary message received");
        break;

    case WS_OPCODE_PING:
        send_message(conn, buf, 0, WS_OPCODE_PONG, 1);
        break;

    case WS_OPCODE_PONG:
        printf("Websocket: PONG received\n");
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
    struct netbuf *inbuf;
    uint8_t *buf;
    uint16_t buflen;
    err_t err;
    bool close = false;

    for (;;)
    {
        err = netconn_recv(conn, &inbuf);

        if (ERR_OK == err)
        {
            netbuf_data(inbuf, (void**)&buf, &buflen);
            
            printf("Websocket: received a message, raw length: %u\n", buflen);

            ws_header_t ws_hdr;

            if (read_ws_header(buf, buflen, &ws_hdr))
            {
                if (!dispatch_ws_message(conn, &ws_hdr,
                                         buf + (uint16_t)ws_hdr.length, &close))
                {
                    printf("Websocket: error dispatching message\n");
                }
            }
            else
            {
                printf("Websocket: error - malformed header\n");
            }
        }

        netbuf_delete(inbuf);

        if (close)
        {
            break;
        }
    }

    if (ws_send_task_handle != NULL)
    {
        vTaskDelete(ws_send_task_handle);
    }

    netconn_close(conn);
    netconn_delete(conn);

    printf("Websocket: connection closed by client\n");
    
    ws_send_task_handle = NULL;
    ws_recv_task_handle = NULL;

    vTaskDelete(NULL);
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

    xTaskCreate(ws_send_task,
                "ws_send",
                DEFAULT_THREAD_STACKSIZE,
                conn,
                WS_SEND_TASK_PRIORITY,
                &ws_send_task_handle);


    xTaskCreate(ws_recv_task,
                "ws_recv",
                DEFAULT_THREAD_STACKSIZE,
                conn,
                WS_RECV_TASK_PRIORITY,
                &ws_recv_task_handle);
}