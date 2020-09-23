#include <avr/io.h>
#include "string.h"

#include "aes.h"
#include "cmac.h"
#include "uart.h"

/*
 * according to https://hackmd.io/@hVCY-lCeTGeM0rEcouirxQ/S1kg6Ymo-?type=view#MAC-Frame-Payload-Encryption
 */
void lorawan_encrypt_payload(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t direction, uint8_t *key, uint8_t *dev_addr) {
  uint8_t block_a[16];
  uint8_t number_of_blocks = size / 16;

  for (uint8_t block_count=0; block_count<=number_of_blocks; block_count++) {
    // create block A_i
    memset(block_a, 0, sizeof(block_a));
    block_a[0] = 0x01;
    block_a[5] = direction;
    for (uint8_t i=0; i<4; i++) block_a[6+i] = dev_addr[3-i];
    block_a[10] = (counter & 0x00ff);
    block_a[11] = ((counter >> 8) & 0x00ff);
    block_a[15] = block_count+1;  // we count from 0 instead of 1 as in the spec

    aes128_encrypt(key, block_a); // usually APPSKEY

    // only xor if data available, no padding with 0x00!
    uint8_t remaining = size - 16*block_count;
    for (uint8_t i=0; i<((remaining > 16) ? 16 : remaining); i++) {
      msg[16*block_count+i] = msg[16*block_count+i] ^ block_a[i];
    }
  }
}

/*
 * lorawan package is +13bytes longer than original message
 * header: 9bytes
 * mic:    4bytes
 * total:  13bytes + size
 */
void lorawan_create_package(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t direction, uint8_t *appskey, uint8_t *nwkskey, uint8_t *dev_addr, uint8_t *package) {
  memset(package, 0, size+13);
  uint8_t cmac[16];
  uint8_t package_b0[size + 16];

  // load header to package
  package[0] = 0x40;                                         // mac header, mtype=010: unconfirmed data uplink
  for (uint8_t i=0; i<4; i++) package[1+i] = dev_addr[3-i];  // device address
  package[5] = 0x00;                                         // frame control
  package[6] = (counter & 0x00ff);                           // frame tx counter lsb
  package[7] = ((counter >> 8) & 0x00ff);                    // frame tx counter msb
  package[8] = 0x01;                                         // frame port port>1 use AppSKey else NwkSkey

  // load encrypted data to package
  lorawan_encrypt_payload(msg, size, counter, direction, appskey, dev_addr);
  for (uint8_t i=0; i<size; i++) {
    package[9+i] = msg[i];
  }

  // calculate mic of package conisting of header + data
  aes128_prepend_b0(package, size+9, counter, direction, dev_addr, package_b0);
  aes128_cmac(nwkskey, package_b0, size+9+16, cmac);

  // load calculated mic=cmac[0..3] to package
  aes128_copy_array(&package[size+9], cmac, 4);
}

/*
 * data = appeui (8)  deveui (8) DevNonce (2)
 * mic = aes128_cmac(AppKey, MHDR | AppEUI | DevEUI | DevNonce)
 *
 * https://lora-alliance.org/sites/default/files/2020-06/rp_2-1.0.1.pdf
 * RECEIVE_DELAY1 1s
 * RECEIVE_DELAY2 2s (RECEIVE_DELAY1 + 1s)
 *
 * JOIN_ACCEPT_DELAY1 5s
 * JOIN_ACCEPT_DELAY2 6s
 *
 * join accept
 * data = AppNonce(3) NetID(3) DevAddr(4), DLSettings(1) RxDelay(1) CFList(16 optional)
 *
 * NwkSKey = aes128_encrypt(AppKey, 0x01 | AppNonce | NetID | DevNonce | pad16)
 * AppSKey = aes128_encrypt(AppKey, 0x02 | AppNonce | NetID | DevNonce | pad16)
 * pad16 = appends zero octects so that length is multiple of 16
 * cmac = aes128_cmac(AppKey, MHDR | AppNonce | NetID | DevAddr | DLSettings | RxDelay | CFList)
 * mic = cmac[0..3]
 */

/*
void lora_join_request(uint32_t dev_nonce) {
  uint32_t package_length = 23;
  uint8_t package[package_length];
  uint8_t mic[4];
  uint8_t direction = 0x00;

  package[0] = 0x00; // mac_header (mtype 000: join request)

  for (uint8_t i=0; i<8; i++) {
    package[1+i] = APPEUI[i];
  }

  for (uint8_t i=0; i<8; i++) {
    package[9+i] = DEVEUI[i];
  }

  package[17] = (dev_nonce & 0x00FF);
  package[18] = ((dev_nonce >> 8) & 0x00FF);

  aes_calculate_mic(package, mic, package_length);
  for (uint8_t i=0; i<4; i++) {
    package[19+i] = mic[i];
  }

  rfm95_send_package(package, package_length);
}
*/