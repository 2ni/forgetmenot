#ifndef __LORAWAN_H_
#define __LORAWAN_H__

#include <avr/io.h>

void lorawan_encrypt_payload(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t direction, uint8_t *key, uint8_t *dev_addr);
void lorawan_create_package(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t direction, uint8_t *appskey, uint8_t *nwkskey, uint8_t *dev_addr, uint8_t *package);

#endif
