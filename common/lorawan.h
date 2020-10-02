#ifndef __LORAWAN_H_
#define __LORAWAN_H__

#include <avr/io.h>

uint8_t  lorawan_join(uint8_t *appeui, uint8_t *deveui, uint8_t *appkey, uint8_t *appskey, uint8_t *nwkskey, uint8_t *devaddr);
void     lorawan_send(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t *appskey, uint8_t *nwkskey, uint8_t *devaddr, uint8_t channel, uint8_t datarate);
uint8_t  lorawan_decode(uint8_t *msg, uint16_t devnonce, uint8_t *appskey, uint8_t *nwkskey, uint8_t *devaddr, uint8_t *appkey);
void     lorawan_send_join_request(uint8_t *appeui, uint8_t *deveui, uint16_t devnonce, uint8_t *appkey, uint8_t channel, uint8_t datarate);
void     lorawan_cipher(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t direction, uint8_t *key, uint8_t *devaddr);
void     lorawan_create_package(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t direction, uint8_t *appskey, uint8_t *nwkskey, uint8_t *devaddr, uint8_t *package);
uint8_t  lorawan_is_same(uint8_t *arr1, uint8_t *arr2, uint8_t len);
uint16_t lorawan_airtime(uint8_t len, uint8_t datarate);

#endif
