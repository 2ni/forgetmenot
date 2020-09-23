#include <avr/io.h>
#include "string.h"

#include "lorawan.h"
#include "aes.h"
#include "cmac.h"
#include "uart.h"
#include "rfm95.h"
#include "sleep.h"
#include "timer.h"

/*
 * https://lora-alliance.org/sites/default/files/2020-06/rp_2-1.0.1.pdf
 * RECEIVE_DELAY1 1s
 * RECEIVE_DELAY2 2s (RECEIVE_DELAY1 + 1s)
 *
 * JOIN_ACCEPT_DELAY1 5s
 * JOIN_ACCEPT_DELAY2 6s (869.525MHz, SF12)
 *
 * according to @Pshemek:
 *  join accept for SF7 is sent 4s after join request, for SF12 after exactly 5s + airtime
 *
 * SF7 and SF8 should have a join reply in RX1 using the parameters of the request if gw has available airtime
 * SF9+ response is in RX2 at SF12
 *
 * what works
 * - join request: ch 4, SF9,  join accept: ch 4, SF9
 * - join request: ch 7, SF9,  join accept: ch 7,  SF9  (~5.3sec)
 * - join request: ch 7, SF10, join accept: ch 99, SF12 (~6.8sec)
 * - join request: ch 6, SF11, join accept: ch 99, SF12 (~7.8sec)
 * - join request: ch x, SF12, join accept: ch 99, SF12 (~6.8-8.0sec)
 *
 */

TIMER timer;

/*
 * return 0: no errors
 *
 * tries to join several time by lowering the datarate each time
 * maybe it's just better to just start at SF12, as we rarely get any response before
 *
 * TODO set rfm95 to sleep when waiting for receive window
 * TODO set attiny to sleep and wake up on pin interrupt rfm_interrupt or time_out
 *
 */
uint8_t lorawan_join(uint8_t *appeui, uint8_t *deveui, uint8_t *appkey, uint8_t *appskey, uint8_t *nwkskey, uint8_t *devaddr) {
  uint8_t msg[64] = {0}; // MHDR(1)+APPNONCE(3)+NETID(3)+DEVADDR(4)+DLSETTINGS(1)+RXDELAY(1)+CFLIST(0|16)

  uint8_t do_sleep = 0; // if set, we put attiny to sleep until receive window is reached. Timings a bit hard to elaborate

  uint16_t devnonce;
  uint8_t tx_channel;
  uint8_t tx_datarate = 7; // starting datarate to try
  uint16_t sleep_time;
  uint16_t timeout = 3000;

  uint8_t rx_channel;
  uint8_t rx_datarate;

  uint8_t valid_lora = 0;
  uint8_t trials = 0;
  // we try 4x to get a join accept package (trying to fullfill retransmission on p.65 of 1.0.3 specification)
  // 0. random channel, datarate SF9 (~5sec delay)
  // 1. random channel, datarate SF10
  // 2. random channel, datarate SF12 (~8sec delay)
  // 3. random channel, datarate SF12 (~8sec delay)
  while (trials < 8 && !valid_lora) {
    devnonce = rfm95_get_random();
    tx_channel = (uint8_t)(rfm95_get_random(8)) / 32;             // random channel 0-7

    // use correct datarates, channels for join request and accept listening
    if (tx_datarate > 9) {
      rx_channel = 99;
      rx_datarate = 12;
      sleep_time = 5500;
    } else {
      rx_channel = tx_channel;
      rx_datarate = tx_datarate;
      sleep_time = 3500;
    }
    lorawan_send_join_request(appeui, deveui, devnonce, appkey, tx_channel, tx_datarate);
    DF("sending join request ch%u SF%u (sleep:%u)\n", tx_channel, tx_datarate, do_sleep ? sleep_time: 0);
    if (do_sleep) {
      DL("sleep");
      // rfm95_sleep();
      sleep_ms(sleep_time);
      DL("wakeup");
    }

    rfm95_receive_continuous(rx_channel, rx_datarate);

    timer.stop();
    timer.start(do_sleep ? timeout: 8100);
    DF("waiting for data ch%u SF%u\n", rx_channel, rx_datarate);

    while (!timer.timed_out() && !valid_lora) {
      // rfm_interrupt==1 -> we have data ready
      if (get_output(&rfm_interrupt) && !lorawan_decode(msg, devnonce, appskey, nwkskey, devaddr, appkey)) {
        valid_lora = 1;
      }
    }
    trials++;
    tx_datarate++;
    if (tx_datarate > 12) tx_datarate = 12;  // try SF7, SF8, ... until SF12

    // rfm95_sleep();
    if (!valid_lora) {
      DL(NOK("timeout!"));
      sleep_s((uint8_t)(1+rfm95_get_random(8) / 32)); // sleep random time 1-8sec
    }
  }

  timer.stop();

  /* we get appskey, nwkskey, devaddr
  if (valid_lora) {
    uart_arr("appskey", appskey, 16);
    uart_arr("nwkskey", nwkskey, 16);
    uart_arr("devaddr", devaddr, 4);
  }
  */

  return !valid_lora;
}

void lorawan_send(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t *appskey, uint8_t *nwkskey, uint8_t *devaddr, uint8_t channel, uint8_t datarate) {
  uint8_t package_size = size + 13; // 9 header + 4 mic
  uint8_t package[package_size] = {0};
  lorawan_create_package(msg, size, counter, 0, appskey, nwkskey, devaddr, package);

  rfm95_send_package(package, package_size, channel, datarate);
}

/*
 * TODO send ack if confirmed data message received, aka ACK set in FHDR
 * return error codes
 * 0: ok
 *
 * "normal" traffic uses 869.525MHz SF9
 *
 */
uint8_t lorawan_decode(uint8_t *msg, uint16_t devnonce, uint8_t *appskey, uint8_t *nwkskey, uint8_t *devaddr, uint8_t *appkey) {
  uint8_t package[64] = {0};
  uint8_t len; // this is the size of the PHYPayload
  if (!rfm95_read_package(package, &len)) {
    // if (!len) return 1;  // no data

    uart_arr(WARN("package received:"), package, len);

    // package = PHYpayload (MHDR MACPayload MIC)
    uint8_t mhdr = package[0];

    // join accept packet
    // all the paypload without the mhdr is encrypted with appkey
    if (mhdr == 0x20) {
      for (uint8_t i=0; i<len; i++) {
        msg[i] = package[i];
      }
      // decrypt
      // mhdr is not part of decrypting
      // -> loop over len-1 and +1 in aes128_encrypt
      for (uint8_t i=0; i<((len-1)/16); i++) {
        aes128_encrypt(appkey, &(msg[i*16+1]));
      }
      // uart_arr("JA decrypted", msg, len);
      // calculate mic and compare with received mic
      // mhdr is part of mic calculation
      uint8_t cmac[16] = {0};
      len -= 4; // "remove" mic
      aes128_cmac(appkey, msg, len, cmac);
      if (lorawan_is_same(cmac, &msg[len], 4)) {
        // appnonce   = msg[1..3]
        // netid      = msg[4..6]
        // devaddr    = msg[10..7]
        // dlsettings = msg[11]
        // rxdelay    = msg[12]
        // cflist     = msg[13..29]
        appskey[0] = 0x02;
        for(uint8_t i=1; i<7; i++) {
          appskey[i] = msg[i];
        }
        appskey[7] = devnonce & 0x00ff;
        appskey[8] = (devnonce >> 8) & 0x00ff;
        aes128_copy_array(nwkskey, appskey);
        nwkskey[0] = 0x01;
        aes128_encrypt(appkey, appskey);
        aes128_encrypt(appkey, nwkskey);

        for (uint8_t i=0; i<4; i++) {
          devaddr[i] = msg[10-i];
        }

        // cflist = list of frequencies
        // uart_arr("appskey", appskey, 16);
        // uart_arr("nwkskey", nwkskey, 16);
        // uart_arr("devaddr", devaddr, 4);
        DF("rxdelay: %u\n", msg[12]);
        DF("rx2 datarate: SF%u\n", 12-(msg[11] & 0x0F));
        DF("rx1 dr offset: 0x%02x\n", (msg[11] & 0x70) > 4);
        // frq
        uint32_t frq;
        for (uint8_t i=0; i<5; i++) {
          frq = 0;
          for (uint8_t ii=0; ii<3; ii++) {
            frq = (frq<<8) + msg[13+(2-ii)+3*i];
          }
          DF("frq ch%u: %lu\n", i+4, frq);
        }
        return 0;
      } else {
        // DF("%s %02x %02x %02x %02x\n", NOK("MIC fail for"), msg[10], msg[9], msg[8], msg[7]);
        // uart_arr("mic calculated", cmac, 4);
        // uart_arr("mic from msg", &msg[len], 4);
        return 1;
      }
    // data packet
    // downlink (un)confirmed message (read)
    // mtype 011, 101
    // mhdr(1) devaddr(4) fctrl(1) fcnt(2)
    } else if (mhdr == 0x60 || mhdr == 0xa0) {
      DF("data for %02x %02x %02x %02x\n", package[4], package[3], package[2], package[1]);
      // verify devaddr TODO is order correct?
      if (lorawan_is_same(&package[1], devaddr, 4)) {
        // verify mic by doing mic over received msg (see https://tools.ietf.org/html/rfc4493 chapter 2.5)
        uint8_t cmac[16] = {0};
        len -= 4; // remove mic
        uint8_t package_with_b0[len + 16];
        uint8_t counter = package[7];
        counter = (counter << 8) + package[6];

        //                                         direction, devaddr
        aes128_prepend_b0(package, len+9, counter, 1, &package[1], package_with_b0);
        aes128_cmac(nwkskey, package_with_b0, len+9+16, cmac);
        if (lorawan_is_same(cmac, &package[len], 4)) {
          // get data
          uint8_t frame_options_len = (package[5] & 0x0F); // 3..0 bits from FCtrl
          uint8_t len_pkg_without_data = 9 + frame_options_len; // MHDR(1) + FHDR(7+FOptsLen) + FPort(1)
          if (len_pkg_without_data > len) { // len = package with data (w/o MIC)
            len -= len_pkg_without_data;
            for (uint8_t i=0; i<len; i++) {
              msg[i] = package[len_pkg_without_data+i];
            }
            //                                    FPort
            lorawan_cipher(msg, len, counter, 1, (package[8+frame_options_len] == 0 ? nwkskey : appskey), devaddr);
            return 1;
          } else {
            DL(NOK("no data"));
            return 1;
          }
        } else {
          // DL(NOK("MIC fail!"));
          return 1;
        }
      } else {
        /*
        DL(NOK("not for us. skipping!"));
        uart_arr("pck", &package[1], 4);
        uart_arr("our", devaddr, 4);
        */
        return 1;
      }
    } else {
      DF(NOK("unsupported type:") "%02x\n", mhdr);
      return 1;
    }
  } else {
    return 1;
  }
}

void lorawan_send_join_request(uint8_t *appeui, uint8_t *deveui, uint16_t devnonce, uint8_t *appkey, uint8_t channel, uint8_t datarate) {
  uint8_t len_nomic = 19;
  uint32_t len_withmic = len_nomic + 4; // appeui(8) + deveui(8) + devnonce(2) + mic(4)
  uint8_t package[len_withmic] = {0};
  uint8_t cmac[16];

  package[0] = 0x00; // mac_header (mtype 000: join request)

  for (uint8_t i=0; i<8; i++) {
    package[1+i] = appeui[7-i];
  }

  for (uint8_t i=0; i<8; i++) {
    package[9+i] = deveui[7-i];
  }

  package[17] = (devnonce & 0x00FF);
  package[18] = ((devnonce >> 8) & 0x00FF);

  // uart_arr(WARN("pkg join"), package, 23);

  aes128_cmac(appkey, package, len_nomic, cmac); // len without mic
  for (uint8_t i=0; i<4; i++) {
    package[len_nomic+i] = cmac[i];
  }

  rfm95_send_package(package, len_withmic, channel, datarate);
}

/*
 * according to https://hackmd.io/@hVCY-lCeTGeM0rEcouirxQ/S1kg6Ymo-?type=view#MAC-Frame-Payload-Encryption
 */
void lorawan_cipher(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t direction, uint8_t *key, uint8_t *devaddr) {
  uint8_t block_a[16];
  uint8_t number_of_blocks = size / 16;

  for (uint8_t block_count=0; block_count<=number_of_blocks; block_count++) {
    // create block A_i
    memset(block_a, 0, sizeof(block_a));
    block_a[0] = 0x01;
    block_a[5] = direction; // 0=send, 1=receive
    for (uint8_t i=0; i<4; i++) block_a[6+i] = devaddr[3-i];
    block_a[10] = (counter & 0x00ff);
    block_a[11] = ((counter >> 8) & 0x00ff);
    block_a[15] = block_count+1;  // we iterate from 0 instead of 1 as in the spec

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
void lorawan_create_package(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t direction, uint8_t *appskey, uint8_t *nwkskey, uint8_t *devaddr, uint8_t *package) {
  memset(package, 0, size+13);
  uint8_t cmac[16];
  uint8_t package_with_b0[size + 16];

  // load header to package
  package[0] = 0x40;                                         // mac header, mtype=010: unconfirmed data uplink (for confirmed: 0x80)
  for (uint8_t i=0; i<4; i++) package[1+i] = devaddr[3-i];  // device address
  package[5] = 0x00;                                         // frame control
  package[6] = (counter & 0x00ff);                           // frame tx counter lsb
  package[7] = ((counter >> 8) & 0x00ff);                    // frame tx counter msb
  package[8] = 0x01;                                         // frame port hardcoded for now. port>1 use AppSKey else NwkSkey TODO

  // load encrypted msg (=FRMPayload) to package
  lorawan_cipher(msg, size, counter, direction, appskey, devaddr);
  for (uint8_t i=0; i<size; i++) {
    package[9+i] = msg[i];
  }

  // calculate mic of package conisting of header + data
  aes128_prepend_b0(package, size+9, counter, direction, devaddr, package_with_b0);
  aes128_cmac(nwkskey, package_with_b0, size+9+16, cmac);

  // load calculated mic=cmac[0..3] to package
  aes128_copy_array(&package[size+9], cmac, 4);
}

uint8_t lorawan_is_same(uint8_t *arr1, uint8_t *arr2, uint8_t len) {
  for (uint8_t i=0; i<len; i++) {
    if (arr1[i] != arr2[i]) return 0;
  }
  return 1;
}
