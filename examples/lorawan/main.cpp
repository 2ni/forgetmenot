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
#include "struct.h"
#include "sleep.h"
#include "millis.h"

uint32_t m;

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

  millis_init();

  uint8_t len = 3;
  uint8_t data[len] = { 0x61, 0x62, 0x63 };
  Packet payload = { .data=data, .len=len };
  uint16_t counter = 0x0005;

  uint8_t rx_len = 64;
  uint8_t rx_data[rx_len] = {0};
  Packet rx_packet = { .data=rx_data, .len=rx_len };

#ifdef OTAA
  extern uint8_t DEVEUI[8];
  extern uint8_t APPEUI[8];
  extern uint8_t APPKEY[16];

  uint8_t appskey[16] = {0};
  uint8_t nwkskey[16] = {0};
  uint8_t devaddr[4] = {0};
  uint16_t devnonce = 0;

  Lora_otaa otaa = { .deveui=DEVEUI, .appeui=APPEUI, .appkey=APPKEY, .devnonce=devnonce };
  Lora_session session = { .nwkskey=nwkskey, .appskey=appskey, .devaddr=devaddr, .counter=counter };

  if (lorawan_join(&otaa, &session) == OK) {
    uart_arr("appskey", session.appskey, 16);
    uart_arr("nwkskey", session.nwkskey, 16);
    uart_arr("devaddr", session.devaddr, 4);
    session.counter = 0;
    if (lorawan_send(&payload, &session, 2, 9, &rx_packet) == OK) {
      uart_arr("received message", rx_packet.data, rx_packet.len);
    }
  } else {
    DL(NOK("joining failed"));
  }
  DL("done");
#else
  extern uint8_t DEVADDR[4];
  extern uint8_t NWKSKEY[16];
  extern uint8_t APPSKEY[16];

  Lora_session session = { .nwkskey=NWKSKEY, .appskey=APPSKEY, .devaddr=DEVADDR, .counter=counter };

  // send package (final test)
  if (lorawan_send(&payload, &session, 2, 9, &rx_packet) == OK) {
    uart_arr("received message", rx_packet.data, rx_packet.len);
  }
  DL("done.");
#endif

  while (true) {}
}
