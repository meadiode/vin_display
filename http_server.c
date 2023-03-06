
#define _GNU_SOURCE /* for memmem */

#include <lwip/opt.h>
#include <lwip/arch.h>
#include <lwip/api.h>
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <stdbool.h>

#include "http_server.h"
#include "fsdata.h"
#include "websocket.h"


#ifndef HTTPD_DEBUG
#define HTTPD_DEBUG         LWIP_DBG_OFF
#endif

#define HTTP_SERVER_TASK_PRIORITY 5

static TaskHandle_t http_server_task_handle = NULL;

const struct
{
    const char *uri;
    const char *alias;

} uri_alias_map[] =
    {
        {"/", "/index.html"}
    };

#define URI_ALIAS_MAP_SIZE (sizeof(uri_alias_map) / sizeof(uri_alias_map[0]))

const struct cont_type_map
{
    const char *ext;
    const char *cont_type;

} cont_type_map[] =
    {
        {"html", "text/html"},
        {"css",  "text/css"},
        {"js",   "text/javascript"},
        {"py",   "text/x-script.python"},
        {"txt",  "text/plain"},
        {"jpg",  "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"png",  "image/png"},
        {"gif",  "image/gif"},
        {"svg",  "image/svg+xml"},
        {"ico",  "image/x-icon"},
    };


#define CONT_TYPE_MAP_SIZE (sizeof(cont_type_map) / sizeof(cont_type_map[0]))


extern const uint32_t FSDATA_404_HTML_SIZE;
extern const uint8_t FSDATA_404_HTML_CONTENTS[];


static void http_server_serve_get(struct netconn *conn,
                                  uint8_t *buf, uint16_t buflen)
{
    const char *uri;
    const char *uri_end;
    const char *response;

    uri = buf + 4;
    uri_end = memchr(uri, ' ', buflen - 4);

    if (NULL != uri_end)
    {

        uint32_t uri_len = uri_end - uri;

        for (uint8_t i = 0; i < URI_ALIAS_MAP_SIZE; i++)
        {
            if (0 == strncmp(uri, uri_alias_map[i].uri, uri_len))
            {
                uri = uri_alias_map[i].alias;
                uri_len = strlen(uri_alias_map[i].alias);
                break;
            }
        }

        const char *ext_start;
        const char *cont_type;
        const char *contents;
        uint32_t contents_size;

        ext_start = memchr(uri, '.', uri_len);
        cont_type = NULL;
        contents = NULL;
        contents_size = 0;

        if (NULL != ext_start)
        {
            ext_start += 1;

            for (uint8_t i = 0; i < CONT_TYPE_MAP_SIZE; i++)
            {
                if (0 == strncmp(cont_type_map[i].ext,
                                 ext_start,
                                 strlen(cont_type_map[i].ext)))
                {
                    cont_type = cont_type_map[i].cont_type;
                    break;
                }
            }
        }

        if (NULL == cont_type)
        {
            cont_type = "application/octet-stream";
        }

        if (uri[0] == '/')
        {
            uri = uri + 1;
            uri_len -= 1;
        }

        for (uint8_t i = 0; i < fsdata_list_size; i++)
        {
            if (0 == strncmp(fsdata_list[i].file_name, uri, uri_len))
            {
                contents = fsdata_list[i].file_contents;
                contents_size = fsdata_list[i].size;
                break;
            }
        }

        if (NULL == contents)
        {
            contents = FSDATA_404_HTML_CONTENTS;
            contents_size = FSDATA_404_HTML_SIZE;
            response = HTTP_RESP_CODE_404;
            cont_type = "text/html";
        }
        else
        {
            response = HTTP_RESP_CODE_OK;
        }

        netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        netconn_write(conn, HTTP_CONTENT_TYPE, strlen(HTTP_CONTENT_TYPE), NETCONN_NOCOPY);
        netconn_write(conn, cont_type, strlen(cont_type), NETCONN_NOCOPY);
        netconn_write(conn, HTTP_NEWLINE, strlen(HTTP_NEWLINE), NETCONN_NOCOPY);
        netconn_write(conn, HTTP_NEWLINE, strlen(HTTP_NEWLINE), NETCONN_NOCOPY);

        while (contents_size)
        {
            uint32_t wsize = MIN(1024, contents_size);
            netconn_write(conn, contents, wsize, NETCONN_NOCOPY);
            contents = contents + wsize;
            contents_size -= wsize;
        }
    }

    /* Close the connection (server closes in HTTP) */
    netconn_close(conn);
    netconn_delete(conn);
}


static bool http_server_check_ws_handshake(const uint8_t *buf, uint16_t buflen,
                                           uint8_t  **sec_key,
                                           uint16_t *sec_key_len)
{
    const uint8_t *line;
    const uint8_t *lbreak;
    bool ws_handshake = false;
    uint16_t llen;
    
    *sec_key_len = 0;
    line = buf;
    
    while (0 != memcmp(line, HTTP_NEWLINE, CSTR_SIZE(HTTP_NEWLINE)))
    {
        lbreak = memmem(line, buflen, HTTP_NEWLINE, CSTR_SIZE(HTTP_NEWLINE));
        
        if (NULL == lbreak)
        {
            break;
        }

        llen = lbreak - line;

        if (0 == memcmp(line, HTTP_UPGRADE_WEBSOCKET, CSTR_SIZE(HTTP_UPGRADE_WEBSOCKET)))
        {
            ws_handshake = true;
        }

        if (0 == memcmp(line, HTTP_SEC_WEBSOCKET_KEY, CSTR_SIZE(HTTP_SEC_WEBSOCKET_KEY)))
        {
            *sec_key = (uint8_t*)(line + CSTR_SIZE(HTTP_SEC_WEBSOCKET_KEY));
            *sec_key_len = llen - CSTR_SIZE(HTTP_SEC_WEBSOCKET_KEY);            
        }

        if (*sec_key_len != 0 && ws_handshake)
        {
            break;
        }

        buflen -= llen + 2;
        line += llen + 2;
    }

    return *sec_key != 0 && ws_handshake;
}


static void http_server_serve(struct netconn *conn)
{
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;
    err_t err;
    uint8_t *sec_key = NULL;
    uint16_t sec_key_len = 0;

    /* Read the data from the port, blocking if nothing yet there.
       We assume the request (the part we care about) is in one netbuf */
    err = netconn_recv(conn, &inbuf);

    if (err == ERR_OK)
    {
        netbuf_data(inbuf, (void**)&buf, &buflen);

        if (0 == strncmp(buf, "GET ", 4))
        {
            if (http_server_check_ws_handshake(buf, buflen, &sec_key, &sec_key_len))
            {
                printf("WebSocket Sec-Key: %.*s\n", sec_key_len, sec_key);
                websocket_handshake(conn, sec_key, sec_key_len);
            }
            else
            {
                http_server_serve_get(conn, buf, buflen);
            }
        }
    }

    /* Delete the buffer (netconn_recv gives us ownership,
    so we have to make sure to deallocate the buffer) */
    netbuf_delete(inbuf);    
}


static void http_server_task(void *params)
{
    struct netconn *conn, *newconn;
    err_t err;

    netconn_thread_init();

    /* Create a new TCP connection handle */
    /* Bind to port 80 (HTTP) with default IP address */

    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, IP_ADDR_ANY, 80);

    /* Put the connection into LISTEN state */
    netconn_listen(conn);

    for (;;)
    {
        err = netconn_accept(conn, &newconn);

        if (err == ERR_OK)
        {
            http_server_serve(newconn);
        }
        else
        {
            printf("\nERROR: http server netconn_accept error: %d\n", err);
            vTaskDelay(10);
        }
    }

    // for (;;)
    // {
    //     vTaskDelay(1000);
    // }
  
    netconn_close(conn);
    netconn_delete(conn);
}

/** Initialize the HTTP server (start its thread) */
void http_server_init(void)
{
    xTaskCreate(http_server_task,
                "http_server",
                1024 * 4,
                NULL,
                HTTP_SERVER_TASK_PRIORITY,
                &http_server_task_handle);
}
