#ifndef __CMAC_H_
#define __CMAC_H__

#include <avr/io.h>

void aes128_prepend_b0(uint8_t *msg, uint8_t len, uint16_t counter, uint8_t direction, uint8_t *dev_addr, uint8_t *msg_with_block);
void aes128_generate_subkeys(uint8_t *key, uint8_t *key1, uint8_t *key2);
void aes128_cmac(uint8_t *key, uint8_t *msg, uint8_t len, uint8_t *mic);

#endif
