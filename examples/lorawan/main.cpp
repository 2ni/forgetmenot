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
#include "timer.h"

uint32_t now;

#define OTAA // choose if you want to send a package by otaa (or abp)

void scan(Lora_otaa *otaa, Lora_session *session) {
  TIMER t_test;
  uint8_t tx_channel = 0;
  uint8_t rx_channel;
  uint8_t rx_datarate;
  uint8_t num_trials = 1;
  for (uint8_t window=1; window<=2; window++) {
    for (uint8_t tx_datarate = 7; tx_datarate<13; tx_datarate++) {
      for (uint8_t trial=0; trial<num_trials; trial++) {
        otaa->devnonce = rfm95_get_random();
        // for rx1 we scan on same datarate and channel
        if (window == 1) {
          rx_channel = tx_channel;
          rx_datarate = tx_datarate;
        } else {
          rx_channel = 99;
          rx_datarate = 12;
        }
        now = millis_time();
        lorawan_send_join_request(otaa, tx_channel, tx_datarate);
        DF(BOLD("\njoin request sent ch%u SF%u") "\n", tx_channel, tx_datarate);
        rfm95_receive_continuous(rx_channel, rx_datarate);
        t_test.start(15000);
        uint8_t valid_lora = 0;
        while (!t_test.timed_out() && !valid_lora) {
          if (get_output(&rfm_interrupt) && lorawan_decode_join_accept(otaa, session) == OK) {
            valid_lora = 1;
          }
        }
        // tx_channel = (tx_channel+1) % 3;
        t_test.stop();
        if (valid_lora) {
          DF(OK("(%lu) RX%u success on SF%u") "\n", millis_time()-now, window, rx_datarate);
          uart_arr("appskey", session->appskey, 16);
        } else {
          DF(NOK("(%lu) RX%u timeout on SF%u") "\n", millis_time()-now, window, rx_datarate);
        }
        sleep_s(10);
      }
    }
    DL("\n******* window 2 **********\n");
  }
  DL("scan done.");
}

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
  DL("OTAA sample");
  extern uint8_t DEVEUI[8];
  extern uint8_t APPEUI[8];
  extern uint8_t APPKEY[16];

  uint8_t appskey[16] = {0};
  uint8_t nwkskey[16] = {0};
  uint8_t devaddr[4] = {0};
  uint16_t devnonce = 0;

  Lora_otaa otaa = { .deveui=DEVEUI, .appeui=APPEUI, .appkey=APPKEY, .devnonce=devnonce };
  Lora_session session = { .nwkskey=nwkskey, .appskey=appskey, .devaddr=devaddr, .counter=counter };

  // whole scan to test
  // lorawan_join(&otaa, &session, 1);
  // DL("done.");

  // manual scan by listening all the time
  // scan(&otaa, &session);
  // while(1);

  if (lorawan_join(&otaa, &session) == OK) {
    uart_arr("appskey", session.appskey, 16);
    uart_arr("nwkskey", session.nwkskey, 16);
    uart_arr("devaddr", session.devaddr, 4);
    DF("datarate: %u\n", session.datarate);
    session.counter = 0;
    if (lorawan_send(&payload, &session, session.datarate, &rx_packet) == OK) {
      uart_arr("received message", rx_packet.data, rx_packet.len);
    }
  } else {
    DL(NOK("joining failed"));
  }
  DL("done");
#else
  DL("ABP sample");

  extern uint8_t DEVADDR[4];
  extern uint8_t NWKSKEY[16];
  extern uint8_t APPSKEY[16];

  Lora_session session = { .nwkskey=NWKSKEY, .appskey=APPSKEY, .devaddr=DEVADDR, .counter=counter };

  // send package (final test)
  for (uint8_t trial=0; trial<1; trial++) {
    for (uint8_t datarate = 7; datarate<13; datarate++) {
      if (lorawan_send(&payload, &session, datarate, &rx_packet) == OK) {
        uart_arr("received message", rx_packet.data, rx_packet.len);
      }
      DL("");
      sleep_s(5);
    }
  }
  DL("done.");
#endif

  while (true) {}
}
