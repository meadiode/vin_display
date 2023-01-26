#ifndef WEBSOCKET_H
#define WEBSOCKET_H


#include <stdint.h>
#include "lwip/api.h"

void websocket_handshake(struct netconn *conn,
                         const uint8_t *sec_key,
                         uint16_t sec_key_len);

#endif /* WEBSOCKET_H */
