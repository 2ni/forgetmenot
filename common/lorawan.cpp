#include <avr/io.h>
#include <util/delay.h>
#include "string.h"

#include "lorawan.h"
#include "aes.h"
#include "cmac.h"
#include "uart.h"
#include "rfm95.h"
#include "sleep.h"
#include "timer.h"
#include "millis.h"

/*
 * https://lora-alliance.org/sites/default/files/2020-06/rp_2-1.0.1.pdf
 * RECEIVE_DELAY1 1s
 * RECEIVE_DELAY2 2s (RECEIVE_DELAY1 + 1s)
 *
 * JOIN_ACCEPT_DELAY1 5sec
 * JOIN_ACCEPT_DELAY2 6sec (869.525MHz, SF12)
 *
 * join accept: 33 bytes -> airtime: 1.8sec
 * -> 6sec + 1.8sec = 7.8sec
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

// TIMER timer;

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

  uint16_t devnonce;
  uint8_t tx_channel = 0;
  uint8_t tx_datarate = 12;

  uint8_t rx_channel;
  uint8_t rx_datarate;

  uint8_t valid_lora;
  uint8_t trials = 0;
  extern uint32_t m;
  uint16_t airtime;
  // we try multiple times to get a join accept package (trying to fullfill retransmission on p.65 of 1.0.3 specification)
  while (trials < 2) {
    valid_lora = 0;
    devnonce = rfm95_get_random();
    // tx_channel = (uint8_t)(rfm95_get_random(8)) / 64 + 4;             // random channel 4+0-3
    tx_channel = (tx_channel+1) % 3; // iterate through channel 0..2
    rx_channel = tx_channel;
    rx_datarate = tx_datarate;

    // use correct datarates, channels for join request and accept listening
    // see "receive windows" in https://lora-alliance.org/sites/default/files/2018-05/lorawan_regional_parameters_v1.0.2_final_1944_1.pdf
    // (RX1DROffset: 0)
    // join accept SF7-SF11 -> RX1 after 5sec: SF7-SF12
    // join accept SF7-SF12 -> RX2 after 6sec: SF12, 869.525MHz

    lorawan_send_join_request(appeui, deveui, devnonce, appkey, tx_channel, tx_datarate);
    m = millis_time();
    DF("\n" BOLD("%u. join request sent ch%u SF%u") "\n", trials, tx_channel, tx_datarate);


    // rx1 window: 5sec + lorawan_airtime(33, rx_datarate)
    // SF10 join accept ~5536ms (airtime: 452ms)
    if (rx_datarate < 12) {
      airtime = lorawan_airtime(33, rx_datarate); // join request len = 33bytes
      // timer.stop();
      DF("(%lu) RX1: waiting for data ch%u SF%u (air: %u)\n", millis_time()-m, rx_channel, rx_datarate, airtime);
      while ((millis_time()-m) < 5000);
      if (!rfm95_wait_for_single_package(rx_channel, rx_datarate) && !lorawan_decode(msg, devnonce, appskey, nwkskey, devaddr, appkey)) {
        valid_lora = 1;
      }
      /*
      rfm95_receive_continuous(rx_channel, rx_datarate);
      timer.start(100+airtime + airtime/8);

      while (!timer.timed_out() && !valid_lora) {
        // rfm_interrupt==1 -> we have data ready
        if (get_output(&rfm_interrupt) && !lorawan_decode(msg, devnonce, appskey, nwkskey, devaddr, appkey)) {
          valid_lora = 1;
        }
      }
      */
      rfm95_sleep();

      if (valid_lora) {
        DF(OK("(%lu) RX1 success") "\n", millis_time()-m);
      } else {
        DF(NOK("(%lu) RX1 timeout") "\n", millis_time()-m);
      }
      if (valid_lora) return 0;
    }

    // rx2 window: 6sec + lorawan_airtime(33, rx_datarate)
    // receive_continuous needs to start at 6000ms
    // join accept ~7946ms (airtime: 1810ms)
    // receive time in middle of preamble except time
    rx_datarate = 12;
    rx_channel = 99;
    valid_lora = 0;
    airtime = lorawan_airtime(33, rx_datarate);
    // timer.stop();
    DF("(%lu) RX2: waiting for data ch%u SF%u (air: %u)\n", millis_time()-m, rx_channel, rx_datarate, airtime);
    while ((millis_time()-m) < 6000);
    if (!rfm95_wait_for_single_package(rx_channel, rx_datarate) && !lorawan_decode(msg, devnonce, appskey, nwkskey, devaddr, appkey)) {
      valid_lora = 1;
    }
    /*
    rfm95_receive_continuous(rx_channel, rx_datarate);
    timer.start(100+airtime + airtime/8);

    while (!timer.timed_out() && !valid_lora) {
      // rfm_interrupt==1 -> we have data ready
      if (get_output(&rfm_interrupt) && !lorawan_decode(msg, devnonce, appskey, nwkskey, devaddr, appkey)) {
        valid_lora = 1;
      }
    }
    */
    rfm95_sleep();

    if (valid_lora) {
      DF(OK("(%lu) RX2 success") "\n", millis_time()-m);
    } else {
      DF(NOK("(%lu) RX2 timeout") "\n", millis_time()-m);
    }
    if (valid_lora) return 0;

    // TODO tx_datarate++;
    if (tx_datarate > 12) tx_datarate = 12;  // try SF7, SF8, ... until SF12

    trials++;
    if (trials < 8) {
      // rfm95_sleep();
      // DL(NOK("timeout!"));
      sleep_s((uint8_t)(1+rfm95_get_random(8) / 16)); // sleep random time 1-8sec
    }
  }

  // timer.stop();

  /* we get appskey, nwkskey, devaddr
  if (valid_lora) {
    uart_arr("appskey", appskey, 16);
    uart_arr("nwkskey", nwkskey, 16);
    uart_arr("devaddr", devaddr, 4);
  }
  */

  return !valid_lora;
}

/*
 * TODO get length of received package
 */
void lorawan_send(uint8_t *msg, uint8_t size, uint16_t counter, uint8_t *appskey, uint8_t *nwkskey, uint8_t *devaddr, uint8_t channel, uint8_t datarate) {
  uint8_t package_size = size + 13; // 9 header + 4 mic
  uint8_t package[package_size] = {0};
  uint32_t m;
  uint8_t msg_receive[8] = {0};
  uint8_t rx_channel = channel;
  uint8_t rx_datarate = datarate;
  lorawan_create_package(msg, size, counter, 0, appskey, nwkskey, devaddr, package);

  rfm95_send_package(package, package_size, channel, datarate);
  m = millis_time();
  DF("package sent ch%u SF%u\n", channel, datarate);

  /*
  rx_channel = 99;
  rx_datarate = 9;
  uint8_t valid_lora = 0;
  rfm95_receive_continuous(rx_channel, rx_datarate);
  while ((millis_time() - m) < 10000 && !valid_lora) {
    if (get_output(&rfm_interrupt) && !lorawan_decode(msg_receive, 0, appskey, nwkskey, devaddr, 0)) {
      valid_lora = 1;
      uart_arr("received", msg_receive, 8);
    }
  }
  return;
  */


  // rx1 window
  DF("(%lu) RX1: waiting for data ch%u SF%u\n", millis_time()-m, rx_channel, rx_datarate);
  while ((millis_time()-m) < 1000);
  if (!rfm95_wait_for_single_package(rx_channel, rx_datarate) && !lorawan_decode(msg_receive, 0, appskey, nwkskey, devaddr, 0)) {
    uart_arr("received rx1", msg_receive, 8);
  }

  // rx2 window
  rx_channel = 99;
  rx_datarate = 9;
  DF("(%lu) RX2: waiting for data ch%u SF%u\n", millis_time()-m, rx_channel, rx_datarate);
  while ((millis_time()-m) < 2000);
  if (!rfm95_wait_for_single_package(rx_channel, rx_datarate) && !lorawan_decode(msg_receive, 0, appskey, nwkskey, devaddr, 0)) {
    uart_arr("received rx2", msg_receive, 8);
  }
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

    // uart_arr(WARN("package received:"), package, len);

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
        /*
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
        */
        return 0;
      } else {
        // DF("%s %02x %02x %02x %02x\n", NOK("MIC fail for"), msg[10], msg[9], msg[8], msg[7]);
        // uart_arr("mic calculated", cmac, 4);
        // uart_arr("mic from msg", &msg[len], 4);
        return 1;
      }
    // data packet
    // downlink (un)confirmed message (read)
    // TODO if confirmed ack must be set in header on the next uplink
    // mtype 011, 101
    // mhdr(1) devaddr(4) fctrl(1) fcnt(2)
    // fctrl = package[5] -> foptslen package[5] bits 3..0
    // counter = package[6..7]
    } else if (mhdr == 0x60 || mhdr == 0xa0) {
      uint8_t devaddr_received[4] = {0};
      for (uint8_t i=0; i<4; i++) devaddr_received[i] = package[4-i]; // package[4..1]
      // uart_arr("data for", devaddr_received, 4);
      if (lorawan_is_same(devaddr_received, devaddr, 4)) {
        uart_arr("you've got mail", package, len);
        // verify mic by doing mic over received msg (see https://tools.ietf.org/html/rfc4493 chapter 2.5)
        uint8_t cmac[16] = {0};
        uint8_t package_with_b0[len+16] = {0};
        len -= 4; // remove mic
        uint16_t counter = package[7];
        counter = (counter << 8) + package[6];
        uint8_t direction = 1;

        aes128_prepend_b0(package, len, counter, direction, devaddr, package_with_b0);
        aes128_cmac(nwkskey, package_with_b0, len+16, cmac);
        if (lorawan_is_same(cmac, &package[len], 4)) {
          // get data
          uint8_t frame_options_len = (package[5] & 0x0F); // FCtrl[3..0]
          uint8_t ack = (package[5] & 0x20); //  FCtrl[5]
          DF("ack: %u\n", ack);
          DF("fctrl: 0x%02x\n", package[5]);
          if (frame_options_len) {
            DF("fopts: 0x%02x\n", frame_options_len);
            uart_arr("options", &package[8], frame_options_len);
          }
          uint8_t fport = package[8+frame_options_len];
          uint8_t pdata = 9 + frame_options_len; // MHDR(1) + FHDR(7+FOptsLen) + FPort(1)
          if (pdata < len) { // len = package with data (w/o MIC)
            for (uint8_t i=0; i<(len-pdata); i++) {
              msg[i] = package[pdata+i];
            }
            lorawan_cipher(msg, len-pdata, counter, direction, (fport ? appskey : nwkskey), devaddr);
            return 0;
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
  package[0] = 0x40;                                         // mac header, mtype=010: unconfirmed data uplink (unconfirmed: 0x40, confirmed: 0x80)
  for (uint8_t i=0; i<4; i++) package[1+i] = devaddr[3-i];   // device address
  package[5] = 0x00;                                         // frame control (TODO respond ack requests: 0x20)
  package[6] = (counter & 0x00ff);                           // frame tx counter lsb
  package[7] = ((counter >> 8) & 0x00ff);                    // frame tx counter msb
  package[8] = 0x01;                                         // frame port hardcoded for now. port>1 use AppSKey else NwkSkey

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

/*
 *
 * https://docs.google.com/spreadsheets/d/19SEkw9gAO1NuBZgpJsPshfnaFuJ7NUc4utmLDjeL3eo/edit#gid=585312605
 *
 * len: total length of packet to send
 * datarate: 7..12 for SFx
 *
 * returns airtime in ms
 */
uint16_t lorawan_airtime(uint8_t len, uint8_t datarate) {
  uint16_t bandwith = 125;
  uint8_t preamble = 8;
  uint8_t explicit_header = 1;
  uint8_t coding_rate = 5;
  uint8_t de = 0;

  de = datarate > 10 ? 1:0;
  uint32_t tsym = ((uint32_t)128<<datarate)/bandwith;           // 128 for better precision
  uint16_t tpreamble = (preamble*128 + 4.25*128)*tsym/128;      // 128 for better precision
  int32_t temp = (16*8*len-4*datarate+28-16-20*(1-explicit_header)) / (4*(datarate-2*de)); // *16 for better precision
  if (temp<0) temp = 0;
  uint8_t symbnb = 8+(temp+16)/16*coding_rate;
  uint32_t tpaypload = symbnb*tsym;

  return (tpreamble+tpaypload)/128;
}
