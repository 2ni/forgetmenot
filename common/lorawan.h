#ifndef __LORAWAN_H__
#define __LORAWAN_H__

#include <avr/io.h>
#include "struct.h"

Status   lorawan_join(Lora_otaa *otaa, Lora_session *session);
Status   lorawan_send(const Packet *payload, const Lora_session *session, const uint8_t channel, const uint8_t datarate, Packet *rx_payload);
Status   lorawan_decode_data_down(const Lora_session *session, Packet *payload);
Status   lorawan_decode_join_accept(const Lora_otaa *otaa, Lora_session *session);
void     lorawan_send_join_request(const Lora_otaa *otaa, uint8_t channel, uint8_t datarate);
void     lorawan_cipher(Packet *payload, const uint16_t counter, const uint8_t direction, const uint8_t *key, const uint8_t *devaddr);
void     lorawan_create_package(const Packet *payload, const Lora_session *session, Packet *lora);
uint8_t  lorawan_is_same(uint8_t *arr1, uint8_t *arr2, uint8_t len);
uint16_t lorawan_airtime(uint8_t len, uint8_t datarate);

#endif
