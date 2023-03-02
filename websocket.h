#ifndef WEBSOCKET_H
#define WEBSOCKET_H


#include <stdint.h>
#include "lwip/api.h"

void websocket_handshake(struct netconn *conn,
                         const uint8_t *sec_key,
                         uint16_t sec_key_len);

bool websocket_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay);

#endif /* WEBSOCKET_H */
