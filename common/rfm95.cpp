/*
 * inspired by https://www.thethingsnetwork.org/forum/t/attiny85-rfm95-temperature-sensor/11211
 *
 * see lorawan spec https://lora-alliance.org/resource-hub/lorawanr-specification-v103
 *
 * https://cdn.hackaday.io/files/286681226531712/RH_RF95.cpp
 * ??? TODO remember last SNR:
 * lastSNR = (int8_t)rfm95_read_reg(0x1a) / 4
 *
 * ??? TODO remember last RSSI
 * lastRssi = rfm95_read_reg(0x1a);
 *  // Adjust the RSSI, datasheet page 87
 *  if (lastSNR < 0)
 *      lastRssi = lastRssi + lastSNR;
 *  else
 *      lastRssi = (int)lastRssi * 16 / 15;
 *  if (_usingHFport)
 *      _lastRssi -= 157;
 *  else
 *      _lastRssi -= 164;
 */

#include <util/delay.h>

#include "rfm95.h"
#include "spi.h"
#include "sleep.h"
#include "pins.h"
#include "uart.h"

uint8_t rfm95_init() {
  spi_init();
  set_direction(&rfm_cs, 1); // set as output
  set_output(&rfm_cs, 1);    // set high (spi bus disabled by default)

  set_direction(&rfm_interrupt, 0); // input, DIO0
  set_direction(&rfm_timeout, 0); // input, DIO1

  // reset might be an issue
  uint8_t version = rfm95_read_reg(0x42);
  DF("version: 0x%02X\n", version);
  if (version != 0x11 && version != 0x12) { // some inofficial chips return ox12
    return 0; // failure
  }

  // sleep
  rfm95_write_reg(0x01, 0x00);

  // lora mode
  rfm95_write_reg(0x01, 0x80);

  // standby mode and lora mode
  rfm95_write_reg(0x01, 0x81);
  // TODO delay needed?
  // _delay_ms(10);

  // set carrier frequency
  rfm95_set_channel(0);

  // SW12, 125kHz
  rfm95_set_datarate(12);

  // PA minimal power 17dbm
  rfm95_setpower(9);
  // rfm95_write_reg(0x09, 0xF0);

  // rx timeout set to 37 symbols
  rfm95_write_reg(0x1F, 0x25);

  // preamble length set to 8 symbols
  // 0x0008 + 4 = 12
  rfm95_write_reg(0x20, 0x00);
  rfm95_write_reg(0x21, 0x08);

  // set lora sync word (0x34)
  rfm95_write_reg(0x39, 0x34);

  // set iq to normal values
  rfm95_write_reg(0x33, 0x27);
  rfm95_write_reg(0x3B, 0x1D);

  // set fifo pointers
  // tx base address
  rfm95_write_reg(0x0E, 0x80);
  // rx base adress
  rfm95_write_reg(0x0F, 0x00);

  // switch rfm to sleep
  rfm95_sleep();
  // rfm95_write_reg(0x01, 0x80);

  return version;
}

/*
 * enable spi transfer
 */
void rfm95_select() {
  set_output(&rfm_cs, 0);
}

/*
 * disable spi transfer
 */
void rfm95_unselect() {
  set_output(&rfm_cs, 1);
}

uint8_t rfm95_read_reg(uint8_t addr) {
  rfm95_select();
  spi_transfer_byte(addr & 0x7F);
  uint8_t val = spi_transfer_byte(0);
  rfm95_unselect();

  return val;
}

void rfm95_write_reg(uint8_t addr, uint8_t value) {
  rfm95_select();
  spi_transfer_byte(addr | 0x80);
  spi_transfer_byte(value);
  rfm95_unselect();
}

/*
 * receive on 869.525, SF9
 * (directly opening window RX2)
 *
 */
void rfm95_receive_continuous(uint8_t channel, uint8_t datarate) {
  rfm95_write_reg(0x40, 0x00); // DIO0 -> rxdone

  // invert iq back
  rfm95_write_reg(0x33, 0x67);
  rfm95_write_reg(0x3B, 0x19);

  rfm95_write_reg(0x01, 0x81); // standby mode
  // _delay_ms(10);

  // set downlink carrier frq
  rfm95_set_channel(channel); // eg 99: 869.525MHz
  rfm95_set_datarate(datarate); // eg 12 for SF12. SF9 for "normal", SF12 for join accept in rx2

  // set hop frequency period to 0
  rfm95_write_reg(0x24, 0x00);

  rfm95_write_reg(0x01, 0x85); // continuous rx
  DF("listening on ch%u SF%u\n", channel, datarate);
}

/*
 * only get one package within a tight timeline
 * and times out if nothing received
 *
 * rx1 with same channel, datarate as tx
 * rx2 with 869.525, SF9
 *
 * 0: no errors (no timeout)
 * 1: timeout
 */
Status rfm95_wait_for_single_package(uint8_t channel, uint8_t datarate) {
  rfm95_write_reg(0x40, 0x00); // DIO0 -> rxdone

  //invert iq back
  rfm95_write_reg(0x33, 0x67);
  rfm95_write_reg(0x3B, 0x19);

  // set downlink carrier frq
  rfm95_set_channel(channel);
  rfm95_set_datarate(datarate);

  rfm95_write_reg(0x01, 0x86); // single rx

  // wait for rx done or timeout
  while (get_output(&rfm_interrupt) == 0 && get_output(&rfm_timeout) == 0) {}

  if (get_output(&rfm_timeout) == 1) {
    rfm95_write_reg(0x12, 0xE0); // clear interrupt
    // DL(NOK("timeout"));
    return TIMEOUT;
  }
  // DL(OK("packet!"));

  return OK;
}

/*
 * get PHYPayload
 * return error code
 * 0: no error
 * 1: crc error
 */
Status rfm95_read(Packet *packet) {
  uint8_t interrupts = rfm95_read_reg(0x12);
  Status status = OK;

  if ((interrupts & 0x20)) {
    status = ERROR; // PayloadCrcError set, we've got an error
  }

  // read rfm package
  uint8_t start_pos = rfm95_read_reg(0x10);
  packet->len = rfm95_read_reg(0x13);
  rfm95_write_reg(0x0D, start_pos);
  for (uint8_t i=0; i<packet->len; i++) {
    packet->data[i] = rfm95_read_reg(0x00);
  }

  rfm95_write_reg(0x12, 0xE0); // clear interrupt register
  rfm95_sleep();

  return status;
}

/*
 * TODO
 * improvements (from the comments):
 * if you would like to use a semi-random key to select the channel,
 * just take a byte from the encrypted payload (or a byte from the MIC)
 * and mask-out the 3 least significant bits as a pointer to the channel
 */
void rfm95_send(const Packet *packet, const uint8_t channel, const uint8_t datarate) {
  // set rfm in standby mode and lora mode
  rfm95_write_reg(0x01, 0x81);

  // switch DIO0 to TxDone
  // 0x40: RegDioMapping1, bits 7, 6
  // 00: rxdone
  // 01: txdone
  rfm95_write_reg(0x40, 0x40);

  rfm95_set_channel(channel); // eg 4 868.1MHz
  rfm95_set_datarate(datarate); // eg 12 for SF12

  // set IQ to normal values
  rfm95_write_reg(0x33, 0x27);
  rfm95_write_reg(0x3B, 0x1D);

  rfm95_write_reg(0x22, packet->len); // set length

  uint8_t tx_pos = rfm95_read_reg(0x0E);
  rfm95_write_reg(0x0D, tx_pos);
  // rfm95_write_reg(0x0D, 0x80); // hardcoded fifo location according RFM95 specs

  // write payload to fifo
  for (uint8_t i = 0; i<packet->len; i++) {
    rfm95_write_reg(0x00, packet->data[i]);
  }

  rfm95_write_reg(0x01, 0x83); // switch rfm to tx

  while(get_output(&rfm_interrupt) == 0) {} // wait for txdone

  rfm95_write_reg(0x12, 0x08); // clear interrupt

  rfm95_sleep();
  // rfm95_write_reg(0x01, 0x80); // switch rfm to sleep

  // uart_arr(WARN("pkg sent"), packet->data, packet->len);
}

/*
 * mode considers only 3 low bits
 * 0 = sleep
 * 1 = standby
 * 2 = fs mode tx (FSTx)
 * 3 = transmitter mode (Tx)
 * 4 = fs mode rx (FSRx)
 * 5 = receiver continuous
 * 6 = receiver single
 * 7 = channel activity detection
 */
void rfm95_set_mode(uint8_t mode) {
  rfm95_write_reg(0x01, 0x80 | mode);
}

/*
 * set power 5-23dBm
 * PA_BOOST is used on RFM95
 *
 * based on
 * - https://github.com/adafruit/RadioHead/blob/master/RH_RF95.cpp#L392
 * - https://github.com/dragino/RadioHead/blob/master/RH_RF95.cpp#L367
 *
 */
void rfm95_setpower(int8_t power) {
  if (power > 23) power = 23;
  if (power < 5) power = 5;

  if (power > 20) {
    rfm95_write_reg(0x4d, 0x07); // RH_RF95_PA_DAC_ENABLE: adds ~3dBm to all power levels, use it for 21, 22, 23dBm
    power -= 3;
  } else {
    rfm95_write_reg(0x4d, 0x04); // RH_RF95_PA_DAC_DISABLE
  }

  // RegPAConfig MSB = 1 to choose PA_BOOST
  //
  rfm95_write_reg(0x09, 0x80 | (power-5));
}

/*
 *
 * set transmitter to sleep
 * set bit 2..0: 000
 */
void rfm95_sleep() {
  rfm95_write_reg(0x01, (rfm95_read_reg(0x01) & ~0x07) | 0x00);
}

/*
 * based on
 * https://github.com/ErichStyger/mcuoneclipse/blob/master/Examples/KDS/tinyK20/LoRaMac-node/src/radio/sx1276/sx1276.c
 *
 */
uint32_t rfm95_get_random(uint8_t bits) {
  uint32_t rnd = 0x0000;

  // disable interrupts
  // rfm95_write_reg(0x11, 0x80 | 0x40 | 0x20 | 0x10 | 0x08 | 0x04 | 0x02 | 0x01);
  // DF("interrupts: 0x%02x\n", rfm95_read_reg(0x11));

  // continuous receive
  rfm95_write_reg(0x01, 0x85);

  for (uint8_t i=0; i<bits; i++) {
    _delay_us(100);
    rnd |= ((uint32_t)rfm95_read_reg(0x2C) & 0x01 ) << i; // unfiltered RSSI value reading (lsb value)
  }

  // set to sleep
  rfm95_sleep();
  // rfm95_write_reg(0x01, 0x80);

  return rnd;
}

/*
 * https://www.thethingsnetwork.org/docs/lorawan/frequency-plans.html
 */
void rfm95_set_datarate(uint8_t rate) {
  switch(rate) {
    case 12: // SF12 BW 125 kHz (DR0)
      rfm95_write_reg(0x1E, 0xC4); // SF12 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x0C); // Low datarate optimization on AGC auto on
      break;
    case 11: // SF11 BW 125 kHz (DR1)
      rfm95_write_reg(0x1E, 0xB4); // SF11 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x0C); // Low datarate optimization on AGC auto on
      break;
    case 10: // SF10 BW 125 kHz (DR2)
      rfm95_write_reg(0x1E, 0xA4); // SF10 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
      break;
    case 9: // SF9 BW 125 kHz (DR3)
      rfm95_write_reg(0x1E, 0x94); // SF9 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
    break;
    case 8: // SF8 BW 125 kHz (DR4)
      rfm95_write_reg(0x1E, 0x84); // SF8 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
    break;
    case 7: // SF7 BW 125 kHz (DR5)
      rfm95_write_reg(0x1E, 0x74); // SF7 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
    break;
    /*
    case 72: // SF7 BW 250kHz (DR6)
      rfm95_write_reg(0x1E, 0x74); // SF7 CRC On
      rfm95_write_reg(0x1D, 0x82); // 250 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
    break;
    */
  }
}

/*
 * set frequency of transmitter
 * in kHz to spare some 0's
 * registers = frq*2^19/32MHz
 *
 * eg 868100 or 869525
 */
void rfm95_set_frq(uint32_t frq) {
    uint32_t frf = (uint64_t)frq*524288/32000;
    // DF("frf: 0x%lx\n", frf);
    // DF("frq: %lu\n", frq);
    rfm95_write_reg(0x06, (uint8_t)(frf>>16));
    rfm95_write_reg(0x07, (uint8_t)(frf>>8));
    rfm95_write_reg(0x08, (uint8_t)(frf>>0));
}

/*
 *
 * https://www.thethingsnetwork.org/docs/lorawan/frequency-plans.html
 * default channels 0..2: 868.1, 868.3, 868.5
 * only changeable in sleep or standby
 * regs = 2^19*frq/32MHz
 *
 */
void rfm95_set_channel(uint8_t channel) {
  if (channel == 99) rfm95_set_frq(FREQUENCY_UP);
  else rfm95_set_frq(FREQUENCIES[channel]);
}
