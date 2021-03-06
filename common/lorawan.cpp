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
 * CAUTION: we have an additional waiting of 50ms because millis_time() seems to be ~50ms too fast
 *
 * TODO set attiny to sleep and wake up on pin interrupt rfm_interrupt or time_out
 *
 * TODO save received cflist, uplink datarate, offset and delay
 * TODO handle sending power (rssi)
 *
 */
Status lorawan_join(Lora_otaa *otaa, Lora_session *session, uint8_t wholescan) {
  uint8_t tx_channel = 0;
  uint8_t tx_datarate = 7;

  uint8_t rx_channel;
  uint8_t rx_datarate;

  uint8_t valid_lora;
  uint8_t trials = 0;
  uint32_t now;
  uint16_t airtime;
  uint16_t num_trials = 7;
  // we try multiple times to get a join accept package (trying to fullfill retransmission on p.65 of 1.0.3 specification)
  while (trials < num_trials) {
    valid_lora = 0;
    otaa->devnonce = rfm95_get_random();
    // tx_channel = (uint8_t)(rfm95_get_random(8)) / 64 + 4;             // random channel 4+0-3
    tx_channel = (tx_channel+1) % 3; // iterate through channel 0..2
    rx_channel = tx_channel;
    rx_datarate = tx_datarate;
    uint16_t sleep_window_rx1= 4990;

    // use correct datarates, channels for join request and accept listening
    // see "receive windows" in https://lora-alliance.org/sites/default/files/2018-05/lorawan_regional_parameters_v1.0.2_final_1944_1.pdf
    // (RX1DROffset: 0)
    // join accept SF7-SF11 -> RX1 after 5sec: SF7-SF12
    // join accept SF7-SF12 -> RX2 after 6sec: SF12, 869.525MHz

    lorawan_send_join_request(otaa, tx_channel, tx_datarate);
    now = millis_time();
    DF("\n" BOLD("%u. join request sent ch%u SF%u") "\n", trials, tx_channel, tx_datarate);


    // rx1 window: 5sec + lorawan_airtime(33, rx_datarate)
    // SF10 join accept ~5536ms (airtime: 452ms)
    if (rx_datarate < 12) {
      airtime = lorawan_airtime(33, rx_datarate); // join request len = 33bytes
      // timer.stop();
      DF("(%lu) RX1: waiting for data ch%u SF%u (air: %u)\n", millis_time()-now, rx_channel, rx_datarate, airtime);
      // while ((millis_time()-now) < (5050)) {}
      sleep_ms(sleep_window_rx1-(millis_time()-now));
      now = millis_time();
      if (rfm95_wait_for_single_package(rx_channel, rx_datarate) == OK && lorawan_decode_join_accept(otaa, session) == OK) {
        session->datarate = tx_datarate;
        valid_lora = 1;
      }
      /*
      rfm95_receive_continuous(rx_channel, rx_datarate);
      timer.start(100+airtime + airtime/8);

       while (!timer.timed_out() && !valid_lora) {
        // rfm_interrupt==1 -> we have data ready
        if (get_output(&rfm_interrupt) && lorawan_decode_join_accept(otaa, session) == OK) {
          valid_lora = 1;
        }
      }
      */

      if (valid_lora) {
        DF(OK("(%lu) RX1 success") "\n", millis_time()-now);
        if (!wholescan) {
          return OK;
        }
      } else {
        DF(NOK("(%lu) RX1 timeout") "\n", millis_time()-now);
      }
      sleep_window_rx1 = 0;
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
    DF("(%lu) RX2: waiting for data ch%u SF%u (air: %u)\n", millis_time()-now, rx_channel, rx_datarate, airtime);
    // while ((millis_time()-now) < (6060)) {}
    sleep_ms(sleep_window_rx1+990-(millis_time()-now));
    if (rfm95_wait_for_single_package(rx_channel, rx_datarate) == OK && lorawan_decode_join_accept(otaa, session) == OK) {
      session->datarate = tx_datarate;
      valid_lora = 1;
    }
    /*
    rfm95_receive_continuous(rx_channel, rx_datarate);
    timer.start(100+airtime + airtime/8);

    while (!timer.timed_out() && !valid_lora) {
      // rfm_interrupt==1 -> we have data ready
      if (get_output(&rfm_interrupt) && !lorawan_decode_join_accept(otaa, session) = OK) {
        valid_lora = 1;
      }
    }
    */

    if (valid_lora) {
      DF(OK("(%lu) RX2 success") "\n", millis_time()-now);
      if (!wholescan) {
        return OK;
      }
    } else {
      DF(NOK("(%lu) RX2 timeout") "\n", millis_time()-now);
    }

    tx_datarate++;
    if (tx_datarate > 12) tx_datarate = 12;  // try SF7, SF8, ... until SF12

    trials++;
    if (trials < num_trials) {
      // DL(NOK("timeout!"));
      uint8_t sleep_time = (uint8_t)(5+rfm95_get_random(8) / 8);
      DF("sleeping: %us\n", sleep_time);
      sleep_s(sleep_time); // sleep random time 5-37sec
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

  return ERROR;
}

Status lorawan_send(const Packet *payload, Lora_session *session, const uint8_t datarate, Packet *rx_payload) {
  uint8_t len = payload->len+13;
  uint8_t data[len];
  memset(data, 0, len);
  Packet lora = { .data=data, .len=len };
  lorawan_create_package(payload, session, &lora);
  uint8_t channel = (uint8_t)(rfm95_get_random(2)); // random channel 0-3

  rfm95_send(&lora, channel, datarate);
  uint32_t now = millis_time();
  session->counter++;
  DF("package sent ch%u SF%u\n", channel, datarate);

  uint8_t rx_channel = channel;
  uint8_t rx_datarate = datarate;

  uint16_t sleep_window_rx1 = 990;

  /*
  rx_channel = 99;
  rx_datarate = 9;
  uint8_t valid_lora = 0;
  rfm95_receive_continuous(rx_channel, rx_datarate);
  while ((millis_time() - now) < 3000 && !valid_lora) {
    if (get_output(&rfm_interrupt) && lorawan_decode_data_down(session, rx_payload) == OK) {
      valid_lora = 1;
    }
  }
  if (valid_lora) {
    DF(OK("(%lu) RX success") "\n", millis_time()-now);
    return OK;
  } else {
    DF(NOK("(%lu) RX timeout") "\n", millis_time()-now);
    return NO_DATA;
  }
  */

  // rx1 window
  // no rx1 window for SF12 as it would takte too much time for 1sec window (airtime 1.8sec)
  if (datarate != 12) {
    DF("(%lu) RX1: waiting for data ch%u SF%u\n", millis_time()-now, rx_channel, rx_datarate);
    // while ((millis_time()-now) < 1010);
    sleep_ms(sleep_window_rx1-(millis_time()-now));
    now = millis_time();
    if (rfm95_wait_for_single_package(rx_channel, rx_datarate) == OK && lorawan_decode_data_down(session, rx_payload) == OK) {
      DL(OK("received rx1"));
      return OK;
    }
    sleep_window_rx1 = 0;
  }

  // rx2 window
  rx_channel = 99;
  rx_datarate = 9;
  DF("(%lu) RX2: waiting for data ch%u SF%u\n", millis_time()-now, rx_channel, rx_datarate);
  // while ((millis_time()-now) < 2020);
  sleep_ms(sleep_window_rx1+990-(millis_time()-now));
  if (rfm95_wait_for_single_package(rx_channel, rx_datarate) == OK && lorawan_decode_data_down(session, rx_payload) == OK) {
    DL(OK("received rx2"));
    return OK;
  }

  return NO_DATA;
}

Status lorawan_decode_data_down(const Lora_session *session, Packet *payload) {
  uint8_t datap[64] = {0};
  Packet phy = { .data=datap, .len=64 }; // PhyPayload MHDR(1) MAC Payload|Join Accept MIC(4)
  Status status = rfm95_read(&phy);
  // uart_arr("raw rx package", phy.data, phy.len);

  // check crc
  if (status != OK) {
    DF("error (%u)\n", status);
    return status;
  }

  // check data type (mhdr)
  uint8_t mhdr = phy.data[0];
  if (mhdr != 0x60 && mhdr != 0xa0) {
    DF(NOK("unsupported type:") "%02x\n", mhdr);
    return NO_DATA;
  }

  // check devaddr
  uint8_t len = 4;
  uint8_t data[len];
  memset(data, 0, len);
  Packet mic = { .data=data, .len=len };

  uint8_t devaddr[4] = {0};
  for (uint8_t i=0; i<4; i++) {
    devaddr[i] = phy.data[4-i]; // phy.data[4..1]
  }
  // uart_arr("data for", devaddr_received, 4);
  if (!lorawan_is_same(session->devaddr, devaddr, 4)) {
    return NO_DATA;
  }

  // uart_arr("you've got mail", phy.data, phy.len);

  // check mic by calculating mic over received msg (see https://tools.ietf.org/html/rfc4493 chapter 2.5)
  phy.len -= 4; // ignore mic for calculation
  uint16_t counter = phy.data[7];
  counter = (counter << 8) + phy.data[6];
  uint8_t direction = 1;

  uint8_t lenb0 = phy.len+16;
  uint8_t datab0[lenb0];
  memset(datab0, 0, lenb0);
  Packet b0 = { .data=datab0, .len=lenb0 };
  aes128_b0(&phy, counter, direction, devaddr, &b0);
  aes128_mic(session->nwkskey, &b0, &mic);
  if (!lorawan_is_same(mic.data, &phy.data[phy.len], 4)) {
    DL(NOK("MIC fail!"));
    return NO_DATA;
  }

  // get frmpayload
  // TODO if confirmed, ack must be set in header on the next uplink
  // mhdr(1) devaddr(4) fctrl(1) fcnt(2)
  // fctrl = phy.data[5] -> foptslen phy.data[5] bits 3..0
  // counter = phy.data[6..7]

  uint8_t frame_options_len = (phy.data[5] & 0x0F); // FCtrl[3..0]
  uint8_t ack = (phy.data[5] & 0x20);               //  FCtrl[5]
  DF("ack: %u\n", ack);
  DF("fctrl: 0x%02x\n", phy.data[5]);
  if (frame_options_len) {
    DF("fopts: 0x%02x\n", frame_options_len);
    uart_arr("options", &phy.data[8], frame_options_len);
  }
  uint8_t fport = phy.data[8+frame_options_len];
  uint8_t pdata = 9 + frame_options_len;            // MHDR(1) + FHDR(7+FOptsLen) + FPort(1)

  if (pdata < phy.len) {
    payload->len = phy.len - pdata;
    for (uint8_t i=0; i<payload->len; i++) {
      payload->data[i] = phy.data[pdata+i];
    }
    // decrypt payload_encrypted by encrypting it (=FRMPayload)
    lorawan_cipher(payload, counter, direction, (fport ? session->appskey : session->nwkskey), devaddr);
    return OK;
  } else {
    DL(NOK("no data"));
    return NO_DATA;
  }
}

Status lorawan_decode_join_accept(const Lora_otaa *otaa, Lora_session *session) {
  uint8_t datap[64] = {0};
  Packet phy = { .data=datap, .len=64 }; // PhyPayload MHDR(1) MAC Payload|Join Accept MIC(4)

  // check crc
  Status status = rfm95_read(&phy);
  // uart_arr("raw join accept package", phy.data, phy.len);
  if (status != OK) {
    DF("error (%u)\n", status);
    return status;
  }

  // check data type (mhdr)
  uint8_t mhdr = phy.data[0];
  if (mhdr != 0x20) {
    DF(NOK("unsupported type:") "%02x\n", mhdr);
    return NO_DATA;
  }

  // decrypt
  // mhdr is not part of decrypting
  // -> loop over len-1 and +1 in aes128_encrypt
  for (uint8_t i=0; i<((phy.len-1)/16); i++) {
    aes128_encrypt(otaa->appkey, &(phy.data[i*16+1]));
  }
  // uart_arr("phypayload decrypted", phy.data, phy.len);
  // calculate mic and compare with received mic
  // mhdr is part of mic calculation
  phy.len -= 4; // "remove" mic

  uint8_t len = 4;
  uint8_t data[len];
  memset(data, 0, len);
  Packet mic = { .data=data, .len=len };
  aes128_mic(otaa->appkey, &phy, &mic);
  if (!lorawan_is_same(mic.data, &phy.data[phy.len], 4)) {
    DL(NOK("MIC fail!"));
    return NO_DATA;
  }

  // appnonce   = phy.data[1..3]
  // netid      = phy.data[4..6]
  // devaddr    = phy.data[10..7]
  // dlsettings = phy.data[11]
  // rxdelay    = phy.data[12]
  // cflist     = phy.data[13..29]
  session->appskey[0] = 0x02;
  for(uint8_t i=1; i<7; i++) {
    session->appskey[i] = phy.data[i];
  }
  session->appskey[7] = otaa->devnonce & 0x00ff;
  session->appskey[8] = (otaa->devnonce >> 8) & 0x00ff;
  aes128_copy_array(session->nwkskey, session->appskey);
  session->nwkskey[0] = 0x01;
  aes128_encrypt(otaa->appkey, session->appskey);
  aes128_encrypt(otaa->appkey, session->nwkskey);

  for (uint8_t i=0; i<4; i++) {
    session->devaddr[i] = phy.data[10-i];
  }

  // cflist = list of frequencies
  // uart_arr("appskey", session->appskey, 16);
  // uart_arr("nwkskey", session->nwkskey, 16);
  // uart_arr("devaddr", session->devaddr, 4);
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
  return OK;
}

void lorawan_send_join_request(const Lora_otaa *otaa, uint8_t channel, uint8_t datarate) {
  uint8_t len = 23; // mhrd(1) + appeui(8) + deveui(8) + devnonce(2) + mic(4)
  uint8_t data[len];
  memset(data, 0, len);
  Packet join = { .data=data, .len=len };

  join.data[0] = 0x00; // mac_header (mtype 000: join request)

  for (uint8_t i=0; i<8; i++) {
    join.data[1+i] = otaa->appeui[7-i];
  }

  for (uint8_t i=0; i<8; i++) {
    join.data[9+i] = otaa->deveui[7-i];
  }

  join.data[17] = (otaa->devnonce & 0x00FF);
  join.data[18] = ((otaa->devnonce >> 8) & 0x00FF);

  // uart_arr(WARN("pkg join"), join.data, join.len);

  join.len -= 4; // ignore mic
  uint8_t datam[4] = {0};
  Packet mic = { .data=datam, .len=4 };
  aes128_mic(otaa->appkey, &join, &mic); // len without mic
  join.len += 4;
  for (uint8_t i=0; i<4; i++) {
    join.data[join.len-4+i] = mic.data[i];
  }

  rfm95_send(&join, channel, datarate);
}

/*
 * according to https://hackmd.io/@hVCY-lCeTGeM0rEcouirxQ/S1kg6Ymo-?type=view#MAC-Frame-Payload-Encryption
 */
void lorawan_cipher(Packet *payload, const uint16_t counter, const uint8_t direction, const uint8_t *key, const uint8_t *devaddr) {
  uint8_t block_a[16];
  uint8_t number_of_blocks = payload->len / 16;

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
    uint8_t remaining = payload->len - 16*block_count;
    for (uint8_t i=0; i<((remaining > 16) ? 16 : remaining); i++) {
      payload->data[16*block_count+i] = payload->data[16*block_count+i] ^ block_a[i];
    }
  }
}

/*
 * lorawan packet: header+port(9) + FRMpayload() + mic(3)
 * payload: len
 * lora: len+13
 * (with B0: len+9+16)
 *
 * package:     40 11 34 01 26 00 05 00 01 6f c9 03
 * b0: 49 00 00 00 00 00 11 34 01 26 05 00 00 00 00 0c 40 11 34 01 26 00 05 00 01 6f c9 03
 * mic: 76 0c e1 ab
 * package+mic: 40 11 34 01 26 00 05 00 01 6f c9 03 76 0c e1 ab
 *
 * lorawan_cipher: 61 62 63 -> 6f c9 03
 *
 */
void lorawan_create_package(const Packet *payload, const Lora_session *session, Packet *lora) {
  uint8_t direction = 0;
  // header
  lora->data[0] = 0x40;                                                  // mac header, mtype=010: unconfirmed data uplink (unconfirmed: 0x40, confirmed: 0x80)
  for (uint8_t i=0; i<4; i++) lora->data[1+i] = session->devaddr[3-i];   // device address
  lora->data[5] = 0x00;                                                  // frame control (TODO respond ack requests: 0x20)
  lora->data[6] = (session->counter & 0x00ff);                           // frame tx counter lsb
  lora->data[7] = ((session->counter >> 8) & 0x00ff);                    // frame tx counter msb
  lora->data[8] = 0x01;                                                  // frame port hardcoded for now. port>1 use AppSKey else NwkSkey

  // encrypt payload (=FRMPayload)
  uint8_t data[payload->len];
  memset(data, 0, payload->len);
  Packet payload_encrypted = { .data=data, .len=payload->len };
  payload_encrypted.len = payload->len;
  for (uint8_t i=0; i<payload_encrypted.len; i++) {
    payload_encrypted.data[i] = payload->data[i];
  }
  lorawan_cipher(&payload_encrypted, session->counter, direction, session->appskey, session->devaddr);
  for (uint8_t i=0; i<payload_encrypted.len; i++) {
    lora->data[9+i] = payload_encrypted.data[i];
  }
  lora->len -= 4; // ignore mic for now

  // calculate mic of package consisting of header + data
  uint8_t lenb0 = payload->len+9+16;
  uint8_t datab0[lenb0];
  memset(datab0, 0, lenb0);
  Packet b0 = { .data=datab0, .len=lenb0 };
  aes128_b0(lora, session->counter, direction, session->devaddr, &b0);
  uint8_t dmic[4] = {0};
  Packet mic = { .data=dmic, .len=4 };
  aes128_mic(session->nwkskey, &b0, &mic);

  // load calculated mic
  lora->len += 4;
  for (uint8_t i=0; i<4; i++) {
    lora->data[lora->len-4+i] = mic.data[i];
  }
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
