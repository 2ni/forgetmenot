/*
 * example to use ttn over otaa (basic eu-868 only) and abp
 * be sure to include the file keys.h like:
 *   #ifndef __KEYS_H__
 *   #define __KEYS_H__
 *
 *   // abp
 *   uint8_t DEVADDR[4] = { 0x00, 0x00, 0x00, 0x00 };
 *   uint8_t NWKSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
 *   uint8_t APPSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
 *
 *   // otaa
 *   uint8_t DEVEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
 *   uint8_t APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
 *   uint8_t APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
 *
 *   #endif
 */

#include <util/delay.h>
#include <string.h>

#include "keys.h"
#include "pins.h"
#include "uart.h"
#include "led.h"
#include "rfm95.h"
#include "cmac.h"
#include "lorawan.h"
#include "aes.h"
#include "lorawan_struct.h"
#include "sleep.h"

#define OTAA // choose if you want to send a package by otaa (or abp)

const char *print_compare(uint8_t *arr1, uint8_t *arr2, uint8_t len) {
  return lorawan_is_same(arr1, arr2, 16) ? OK("ok") : NOK("fail");
}

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  uint32_t node = get_deviceid();
  // see https://stackoverflow.com/questions/4842424/list-of-ansi-color-escape-sequences for colors
  DF("\n\033[1;38;5;226;48;5;18m Hello from 0x%06lX \033[0m\n", node);

  led_flash(&led_g, 3);

  uint8_t version = rfm95_init();
  if (!version) {
    DL(NOK("RFM95 init failed"));
    while(true) {}
  } else {
    DF("RFM95 Version: 0x%02x\n", version);
  }

  uint8_t len_msg = 20;
  uint8_t msg[len_msg] = { 0x00, 0x69, 0xDC, 0xD9, 0xEC, 0xB2, 0xE4, 0xB7, 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x03, 0x48, 0x7D, 0x26, 0x01, 0x34, 0x11 };
  uint16_t frame_counter = 0x0003;

#ifdef OTAA
  extern uint8_t DEVEUI[8];
  extern uint8_t APPEUI[8];
  extern uint8_t APPKEY[16];

  uint8_t appskey[16] = {0};
  uint8_t nwkskey[16] = {0};
  uint8_t devaddr[4] = {0};
  if(!lorawan_join(APPEUI, DEVEUI, APPKEY, appskey, nwkskey, devaddr)) {
    uart_arr("appskey", appskey, 16);
    uart_arr("nwkskey", nwkskey, 16);
    uart_arr("devaddr", devaddr, 4);
  } else {
    DL(NOK("joining failed"));
  }
  lorawan_send(msg, len_msg, frame_counter, appskey, nwkskey, devaddr, 4, 7); // channel 4, SF7
  DL("done");
#else
  extern uint8_t DEVADDR[4];
  extern uint8_t NWKSKEY[16];
  extern uint8_t APPSKEY[16];

  D("sending: ");
  // send package (final test)
  lorawan_send(msg, len_msg, frame_counter, APPSKEY, NWKSKEY, DEVADDR, 4, 7); // channel 4 SF7
  DL(OK("ok"));
#endif

  while (true) {}
}
