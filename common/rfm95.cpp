/*
 * from https://www.thethingsnetwork.org/forum/t/attiny85-rfm95-temperature-sensor/11211
 *
 * see lorawan spec https://lora-alliance.org/resource-hub/lorawanr-specification-v103
 *
 * ABP implementation, no downlink
 * add device address, network session key, app session key from
 * https://console.thethingsnetwork.org/applications/forgetmenot/devices/<devicename>/settings
 * to common/aes_keys.h
 *
 *
 * -> see https://github.com/ClemensRiederer/TinyLoRa-BME280_v1.1/blob/master/ATtinyLoRa.cpp#L101
 *
 * {
 *   "time": "2020-09-14T16:48:30.275586351Z",
 *   "frequency": 868.1,
 *   "modulation": "LORA",
 *   "data_rate": "SF7BW125",
 *   "coding_rate": "4/5",
 *   "gateways": [
 *     {
 *       "gtw_id": "eui-b827ebfffe03753a",
 *       "timestamp": 98406867,
 *       "time": "2020-09-14T16:48:30.242795Z",
 *       "channel": 0,
 *       "rssi": -110,
 *       "snr": 3.5,
 *       "latitude": 47.382,
 *       "longitude": 8.541,
 *       "altitude": 433
 *     },
 *     {
 *       "gtw_id": "ttn-tdse-gw-01",
 *       "timestamp": 2408293347,
 *       "time": "2020-09-14T16:48:30Z",
 *       "channel": 0,
 *       "rssi": -104,
 *       "snr": -4.75
 *     },
 *     {
 *       "gtw_id": "eui-b827ebfffe162cc4",
 *       "timestamp": 3301675763,
 *       "time": "2020-09-14T16:48:30.240182Z",
 *       "channel": 0,
 *       "rssi": -111,
 *       "snr": -1.5,
 *       "latitude": 47.374,
 *       "longitude": 8.548,
 *       "altitude": 443
 *     },
 *     {
 *       "gtw_id": "eui-58a0cbfffe8002fa",
 *       "timestamp": 4270618907,
 *       "time": "2020-09-14T16:48:30.258169889Z",
 *       "channel": 0,
 *       "rssi": -115,
 *       "snr": -6.25
 *     },
 *     {
 *       "gtw_id": "eui-b827ebfffe97f686",
 *       "timestamp": 247372843,
 *       "time": "2020-09-14T16:48:30.239936Z",
 *       "channel": 0,
 *       "rssi": -107,
 *       "snr": 3.8
 *     }
 *   ]
 * }
 *
 */

#include <avr/delay.h>

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
  _delay_ms(10);

  // set carrier frequency
  // 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B
  rfm95_write_reg(0x06, 0xD9);
  rfm95_write_reg(0x07, 0x06);
  rfm95_write_reg(0x08, 0x8B);

  // PA pin (medium power)
  rfm95_write_reg(0x09, 0xAF);

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
  rfm95_write_reg(0x01, 0x00);

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
 * https://www.thethingsnetwork.org/docs/lorawan/frequency-plans.html
 * DIO0: txdone, rxdone
 * DIO1: rxtimeout
 * https://gist.github.com/wero1414/522fa352ff561bcd845d5fd8149b5105
 */
void rfm95_receive_package() {
  rfm95_write_reg(0x40, 0x00); // DIO0 -> rxdone

  // invert iq back
  rfm95_write_reg(0x33, 0x67);
  rfm95_write_reg(0x3B, 0x19);

  // set downlink carrier frq
  rfm95_set_channel(99); // 868.525MHz
  rfm95_set_datarate(9); // SF9, 125kHz

  // set hop frequency period to 0
  rfm95_write_reg(0x24, 0x00);


  rfm95_set_channel(0); // 868.1MHz
  rfm95_set_mode(5); // continuous rx

  // wait for input
  while(get_output(&rfm_interrupt) == 0) {}

  // read received package
  // TODO needed to read start pos of buffer?
  uint8_t start_pos = rfm95_read_reg(0x10);
  uint8_t len = rfm95_read_reg(0x13);
  uint8_t msg[len];
  rfm95_write_reg(0x0D, start_pos); // set start of package read
  for (uint8_t i=0; i<len; i++) {
    msg[i] = rfm95_read_reg(0x00);
  }
  uart_arr("msg", msg, len);
}

/*
 * TODO
 * improvements (from the comments):
 * if you would like to use a semi-random key to select the channel,
 * just take a byte from the encrypted payload (or a byte from the MIC)
 * and mask-out the 3 least significant bits as a pointer to the channel
 */
void rfm95_send_package(uint8_t *package, uint8_t package_length) {
  uint8_t i;
  // uint8_t rfm_tx_location = 0x00;

  // set rfm in standby mode and lora mode
  rfm95_write_reg(0x01, 0x81);
  // while (digitalRead(DIO5) == LOW) {}
  _delay_ms(10);

  // switch DIO0 to TxDone
  // 0x40: RegDioMapping1, bits 7, 6
  // 00: rxdone
  // 01: txdone
  rfm95_write_reg(0x40, 0x40);

  rfm95_set_channel(0); // 868.1MHz
  rfm95_set_datarate(71); // SF7 BW 125kHz

  // set IQ to normal values
  rfm95_write_reg(0x33, 0x27);
  rfm95_write_reg(0x3B, 0x1D);

  // set payload length to the right length
  rfm95_write_reg(0x22, package_length);

  // get location of Tx part of FiFo
  // rfm_tx_location = RFM_Read(0x0E);

  // set SPI pointer to start of Tx part in FiFo
  //rfm95_write_reg(0x0D, rfm_tx_location);

  rfm95_write_reg(0x0D, 0x80); // hardcoded fifo location according RFM95 specs

  // write payload to fifo
  for (i = 0;i < package_length; i++) {
    rfm95_write_reg(0x00, *package);
    package++;
  }

  // switch rfm to tx
  rfm95_write_reg(0x01, 0x83);

  // wait for txdone
  while(get_output(&rfm_interrupt) == 0) {}

  // switch rfm to sleep
  rfm95_write_reg(0x01, 0x00);
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
 * based on https://github.com/adafruit/RadioHead/blob/master/RH_RF95.cpp#L392
 */
void rfm95_setpower(int8_t power) {
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

void rfm95_set_datarate(uint8_t rate) {
  switch(rate) {
    case 12: // SF12 BW 125 kHz
      rfm95_write_reg(0x1E, 0xC4); // SF12 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x0C); // Low datarate optimization on AGC auto on
      break;
    case 11: // SF11 BW 125 kHz
      rfm95_write_reg(0x1E, 0xB4); // SF11 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x0C); // Low datarate optimization on AGC auto on
      break;
    case 10: // SF10 BW 125 kHz
      rfm95_write_reg(0x1E, 0xA4); // SF10 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
      break;
    case 9: // SF9 BW 125 kHz
      rfm95_write_reg(0x1E, 0x94); // SF9 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
    break;
    case 8: // SF8 BW 125 kHz
      rfm95_write_reg(0x1E, 0x84); // SF8 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
    break;
    case 71: // SF7 BW 125 kHz
      rfm95_write_reg(0x1E, 0x74); // SF7 CRC On
      rfm95_write_reg(0x1D, 0x72); // 125 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
    break;
    case 72: // SF7 BW 250kHz
      rfm95_write_reg(0x1E, 0x74); // SF7 CRC On
      rfm95_write_reg(0x1D, 0x82); // 250 kHz 4/5 coding rate explicit header mode
      rfm95_write_reg(0x26, 0x04); // Low datarate optimization off AGC auto on
    break;
  }
}

void rfm95_set_channel(uint8_t channel) {
  switch(channel) {
    case 0: // channel 0: 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B
      rfm95_write_reg(0x06, 0xD9);
      rfm95_write_reg(0x07, 0x06);
      rfm95_write_reg(0x08, 0x8B);
      break;

    case 1: // channel 1: 868.300 MHz / 61.035 Hz = 14226264 = 0xD91358
      rfm95_write_reg(0x06,0xD9);
      rfm95_write_reg(0x07,0x13);
      rfm95_write_reg(0x08,0x58);
      break;

    case 2: // channel 2 868.500 MHz / 61.035 Hz = 14229540 = 0xD92024
      rfm95_write_reg(0x06,0xD9);
      rfm95_write_reg(0x07,0x20);
      rfm95_write_reg(0x08,0x24);
      break;

    case 3: // channel 3 867.100 MHz / 61.035 Hz = 14206603 = 0xD8C68B
      rfm95_write_reg(0x06,0xD8);
      rfm95_write_reg(0x07,0xC6);
      rfm95_write_reg(0x08,0x8B);
      break;

    case 4: // channel 4 867.300 MHz / 61.035 Hz = 14209880 = 0xD8D358
      rfm95_write_reg(0x06,0xD8);
      rfm95_write_reg(0x07,0xD3);
      rfm95_write_reg(0x08,0x58);
      break;

    case 5: // channel 5 867.500 MHz / 61.035 Hz = 14213156 = 0xD8E024
      rfm95_write_reg(0x06,0xD8);
      rfm95_write_reg(0x07,0xE0);
      rfm95_write_reg(0x08,0x24);
      break;

    case 6: // channel 6 867.700 MHz / 61.035 Hz = 14216433 = 0xD8ECF1
      rfm95_write_reg(0x06,0xD8);
      rfm95_write_reg(0x07,0xEC);
      rfm95_write_reg(0x08,0xF1);
      break;

    case 7: // channel 7 867.900 MHz / 61.035 Hz = 14219710 = 0xD8F9BE
      rfm95_write_reg(0x06,0xD8);
      rfm95_write_reg(0x07,0xF9);
      rfm95_write_reg(0x08,0xBE);
      break;
    case 99: // downlink channel 869.525 MHz / 61.035 Hz = 14246334 = 0xD961BE
      // set downlink carrier frq
      // https://www.thethingsnetwork.org/docs/lorawan/frequency-plans.html
      rfm95_write_reg(0x06, 0xD9);
      rfm95_write_reg(0x07, 0x61);
      rfm95_write_reg(0x08, 0xBE);
      break;
  }
}
