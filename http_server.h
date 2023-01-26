#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#define HTTP_RESP_CODE_OK   "HTTP/1.1 200 OK\r\n"
#define HTTP_RESP_CODE_101  "HTTP/1.1 101 Switching Protocols\r\n"
#define HTTP_RESP_CODE_404  "HTTP/1.1 404 Resource not found\r\n"
#define HTTP_RESP_CODE_500  "HTTP/1.1 500 Internal server error\r\n"

#define HTTP_NEWLINE              "\r\n"
#define HTTP_CONTENT_TYPE         "Content-type: "
#define HTTP_UPGRADE_WEBSOCKET    "Upgrade: websocket"
#define HTTP_CONNECTION_UPGRADE   "Connection: Upgrade"
#define HTTP_SEC_WEBSOCKET_KEY    "Sec-WebSocket-Key: "
#define HTTP_SCE_WEBSOCKET_ACCEPT "Sec-WebSocket-Accept: "


#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define CSTR_SIZE(const_str) (sizeof((const_str)) - 1)


void http_server_init(void);

#endif /* HTTP_SERVER_H */
