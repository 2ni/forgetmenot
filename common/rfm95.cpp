/*
 * from https://www.thethingsnetwork.org/forum/t/attiny85-rfm95-temperature-sensor/11211
 *
 * ABP implementation, no downlink
 * add device address, network session key, app session key from
 * https://console.thethingsnetwork.org/applications/forgetmenot/devices/<devicename>/settings
 * to common/aes_keys.h
 *
 * improvements (from the comments):
 * if you would like to use a semi-random key to select the channel,
 * just take a byte from the encrypted payload (or a byte from the MIC)
 * and mask-out the 3 least significant bits as a pointer to the channel
 *
 */

#include <avr/delay.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "rfm95.h"
#include "spi.h"
#include "sleep.h"
#include "pins.h"
#include "uart.h"
#include "aes_keys.h"

#if !defined(__AES_KEYS_H__)
#warning "pls define keys in aes_keys.h"
#endif

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

  // standby mode wait on mode ready
  rfm95_write_reg(0x01, 0x81);
  _delay_ms(10);

  // set carrier frequency
  // 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B
  rfm95_write_reg(0x06, 0xD9);
  rfm95_write_reg(0x07, 0x06);
  rfm95_write_reg(0x08, 0x8B);

  // PA pin (maximal power)
  rfm95_write_reg(0x09, 0xFF);

  // BW = 125 kHz, Coding rate 4/5, Explicit header mode
  rfm95_write_reg(0x1D, 0x72);

  // spreading factor 7, payloadcrc On
  rfm95_write_reg(0x1E, 0xB4);

  // rx timeout set to 37 symbols
  rfm95_write_reg(0x1F, 0x25);

  // preamble length set to 8 symbols
  // 0x0008 + 4 = 12
  rfm95_write_reg(0x20, 0x00);
  rfm95_write_reg(0x21, 0x08);

  // low datarate optimization off AGC auto on
  rfm95_write_reg(0x26, 0x0C);

  // set lora sync word
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
*****************************************************************************************
* Description : Function contstructs a LoRaWAN package and sends it
*
* Arguments   : *Data pointer to the array of data that will be transmitted
*               Data_Length nuber of bytes to be transmitted
*               Frame_Counter_Up  Frame counter of upstream frames
*****************************************************************************************
*/
void lora_send_data(uint8_t *data, uint8_t data_length, uint16_t frame_counter_tx) {
  // direction of frame is up
  uint8_t direction = 0x00;

  uint8_t package[64];
  uint8_t package_length;

  uint8_t mic[4];

  aes_encrypt_payload(data, data_length, frame_counter_tx, direction);

  // build the radio package
  package[0] = 0x40; // mac_header

  package[1] = DEVADDR[3];
  package[2] = DEVADDR[2];
  package[3] = DEVADDR[1];
  package[4] = DEVADDR[0];

  package[5] = 0x00; // frame_control

  package[6] = (frame_counter_tx & 0x00FF);
  package[7] = ((frame_counter_tx >> 8) & 0x00FF);

  package[8] = 0x01; // frame_port

  // set current package length
  package_length = 9;

  // load data
  for(uint8_t i = 0; i < data_length; i++) {
    package[package_length + i] = data[i];
  }

  //Add data Lenth to package length
  package_length = package_length + data_length;

  // calculate mic
  aes_calculate_mic(package, mic, package_length, frame_counter_tx, direction);

  // load mic in package
  for(uint8_t i = 0; i < 4; i++) {
    package[i + package_length] = mic[i];
  }

  // add mic length to rfm package length
  package_length += 4;

  rfm95_send_package(package, package_length);
}

/*
*****************************************************************************************
* Description : Function for sending a package with the RFM
*
* Arguments   : *package Pointer to arry with data to be send
*               Package_Length  Length of the package to send
*****************************************************************************************
*/

void rfm95_send_package(uint8_t *package, uint8_t package_length) {
  uint8_t i;
  // uint8_t rfm_tx_location = 0x00;

  // set rfm in standby mode wait on mode ready
  rfm95_write_reg(0x01, 0x81);
  // while (digitalRead(DIO5) == LOW) {}
  _delay_ms(10);

  //Switch DIO0 to TxDone
  rfm95_write_reg(0x40, 0x40);

  //Set carrier frequency
  // 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B
  rfm95_write_reg(0x06, 0xD9);
  rfm95_write_reg(0x07, 0x06);
  rfm95_write_reg(0x08, 0x8B);

  //SF7 BW 125 kHz
  rfm95_write_reg(0x1E, 0x74); //SF7 CRC On
  rfm95_write_reg(0x1D, 0x72); //125 kHz 4/5 coding rate explicit header mode
  rfm95_write_reg(0x26, 0x04); //Low datarate optimization off AGC auto on

  //Set IQ to normal values
  rfm95_write_reg(0x33, 0x27);
  rfm95_write_reg(0x3B, 0x1D);

  //Set payload length to the right length
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

void aes_encrypt_payload(uint8_t *data, uint8_t data_length, uint16_t frame_counter, uint8_t direction) {
  uint8_t i = 0;
  uint8_t j;
  uint8_t number_of_blocks = 0x00;
  uint8_t incomplete_block_size = 0x00;

  uint8_t block_a[16];

  //Calculate number of blocks
  number_of_blocks = data_length / 16;
  incomplete_block_size = data_length % 16;
  if (incomplete_block_size != 0) {
    number_of_blocks++;
  }

  for (i=1; i<=number_of_blocks; i++) {
    block_a[0] = 0x01;
    block_a[1] = 0x00;
    block_a[2] = 0x00;
    block_a[3] = 0x00;
    block_a[4] = 0x00;

    block_a[5] = direction;

    block_a[6] = DEVADDR[3];
    block_a[7] = DEVADDR[2];
    block_a[8] = DEVADDR[1];
    block_a[9] = DEVADDR[0];

    block_a[10] = (frame_counter & 0x00FF);
    block_a[11] = ((frame_counter >> 8) & 0x00FF);

    block_a[12] = 0x00; // frame counter upper bytes
    block_a[13] = 0x00;

    block_a[14] = 0x00;

    block_a[15] = i;

    // calculate S
    aes_encrypt(block_a, APPSKEY); // original


    // check for last block
    if (i != number_of_blocks) {
      for (j=0; j<16; j++) {
        *data = *data ^ block_a[j];
        data++;
      }
    } else {
      if (incomplete_block_size == 0) {
        incomplete_block_size = 16;
      }
      for (j=0; j<incomplete_block_size; j++) {
        *data = *data ^ block_a[j];
        data++;
      }
    }
  }
}

void aes_calculate_mic(uint8_t *data, uint8_t *final_mic, uint8_t data_length, uint16_t frame_counter, uint8_t direction) {
  uint8_t i;
  uint8_t block_b[16];

  uint8_t key_k1[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t key_k2[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t old_data[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t new_data[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  uint8_t number_of_blocks = 0;
  uint8_t incomplete_block_size = 0;
  uint8_t block_counter = 0x01;

  //Create block_b
  block_b[0] = 0x49;
  block_b[1] = 0x00;
  block_b[2] = 0x00;
  block_b[3] = 0x00;
  block_b[4] = 0x00;

  block_b[5] = direction;

  block_b[6] = DEVADDR[3];
  block_b[7] = DEVADDR[2];
  block_b[8] = DEVADDR[1];
  block_b[9] = DEVADDR[0];

  block_b[10] = (frame_counter & 0x00FF);
  block_b[11] = ((frame_counter >> 8) & 0x00FF);

  block_b[12] = 0x00; // frame counter upper bytes
  block_b[13] = 0x00;

  block_b[14] = 0x00;
  block_b[15] = data_length;

  // calculate number of Blocks and blocksize of last block
  number_of_blocks = data_length / 16;
  incomplete_block_size = data_length % 16;

  if (incomplete_block_size != 0) {
    number_of_blocks++;
  }

  aes_generate_keys(key_k1, key_k2);

  aes_encrypt(block_b, NWKSKEY);

  // copy block_b to old_data
  for (i=0; i<16; i++) {
    old_data[i] = block_b[i];
  }

  // perform full calculating until n-1 messsage blocks
  while (block_counter < number_of_blocks) {
    // copy data into array
    for (i=0; i<16; i++) {
      new_data[i] = *data;
      data++;
    }

    // perform xor with old data
    aes_xor(new_data, old_data);

    // perform aes encryption
    aes_encrypt(new_data, NWKSKEY);

    // copy new_data to old_data
    for (i=0; i<16; i++) {
      old_data[i] = new_data[i];
    }

    block_counter++;
  }

  // perform calculation on last block
  // check if datalength is a multiple of 16
  if (incomplete_block_size == 0) {
    // copy last data into array
    for (i=0; i<16; i++) {
      new_data[i] = *data;
      data++;
    }

    // perform xor with key 1
    aes_xor(new_data, key_k1);

    // perform xor with old data
    aes_xor(new_data, old_data);

    // perform last aes routine
    // read NWKSKEY from PROGMEM
    aes_encrypt(new_data, NWKSKEY);
  } else {
    // copy the remaining data and fill the rest
    for (i=0; i<16; i++) {
      if (i < incomplete_block_size) {
        new_data[i] = *data;
        data++;
      }
      if (i == incomplete_block_size) {
        new_data[i] = 0x80;
      }
      if (i > incomplete_block_size) {
        new_data[i] = 0x00;
      }
    }

    // perform xor with key 2
    aes_xor(new_data, key_k2);

    // perform xor with old data
    aes_xor(new_data, old_data);

    // perform last aes routine
    aes_encrypt(new_data, NWKSKEY);
  }

  final_mic[0] = new_data[0];
  final_mic[1] = new_data[1];
  final_mic[2] = new_data[2];
  final_mic[3] = new_data[3];
}

void aes_generate_keys(uint8_t *k1, uint8_t *k2) {
  uint8_t msb_key;

  //Encrypt the zeros in k1 with the NWKSKEY
  aes_encrypt(k1, NWKSKEY);

  // create k1
  // check if msb is 1
  if ((k1[0] & 0x80) == 0x80) {
    msb_key = 1;
  } else {
    msb_key = 0;
  }

  aes_shift_left(k1);

  if (msb_key == 1) {
    k1[15] = k1[15] ^ 0x87;
  }

  // copy k1 to k2
  for (uint8_t i = 0; i < 16; i++) {
    k2[i] = k1[i];
  }

  // check if msb is 1
  if ((k2[0] & 0x80) == 0x80) {
    msb_key = 1;
  } else {
    msb_key = 0;
  }

  aes_shift_left(k2);

  // check if msb was 1
  if (msb_key == 1) {
    k2[15] = k2[15] ^ 0x87;
  }
}

void aes_shift_left(uint8_t *data) {
  uint8_t i;
  uint8_t overflow = 0;

  for (i=0; i<16; i++) {
    // check for overflow on next byte except for the last byte
    if (i < 15) {
      // check if upper bit is one
      if ((data[i+1] & 0x80) == 0x80) {
        overflow = 1;
      } else {
        overflow = 0;
      }
    } else {
      overflow = 0;
    }

    // shift one left
    data[i] = (data[i] << 1) + overflow;
  }
}

void aes_xor(uint8_t *new_data, uint8_t *old_data) {
  for (uint8_t i = 0; i < 16; i++) {
    new_data[i] = new_data[i] ^ old_data[i];
  }
}

void aes_encrypt(uint8_t *data, uint8_t *key) {
  uint8_t number_of_rounds = 10;
  uint8_t round_key[16];
  uint8_t state[4][4];

  //  copy input to state array
  for (uint8_t column=0; column<4; column++) {
    for (uint8_t row=0; row<4; row++) {
      state[row][column] = data[row + (column << 2)];
    }
  }

  // copy key to round key
  memcpy(&round_key[0], &key[0], 16);

  aes_add_round_key(round_key, state);

  // perform 9 full rounds with mixed columns
  for (uint8_t round=1; round<number_of_rounds; round++ ) {
    // perform byte substitution with S table
    for (uint8_t column=0 ; column<4 ; column++) {
      for (uint8_t row=0 ; row<4; row++) {
        state[row][column] = aes_sub_byte( state[row][column] );
      }
    }

    aes_shift_rows(state);
    aes_mix_columns(state);
    aes_calculate_round_key(round, round_key);
    aes_add_round_key(round_key, state);
  }

  // perform byte substitution with S table whitout mix columns
  for (uint8_t column=0; column<4; column++) {
    for (uint8_t row=0; row<4; row++ ) {
      state[row][column] = aes_sub_byte(state[row][column]);
    }
  }

  aes_shift_rows(state);
  aes_calculate_round_key(number_of_rounds, round_key);
  aes_add_round_key(round_key, state);

  // copy the state into the data array
  for (uint8_t column=0; column<4; column++ ) {
    for (uint8_t row=0; row<4; row++ ) {
      data[row + (column << 2)] = state[row][column];
    }
  }
}

void aes_add_round_key(uint8_t *round_key, uint8_t (*state)[4]) {
  for (uint8_t col=0; col<4; col++) {
    for (uint8_t row=0; row<4; row++) {
      state[row][col] ^= round_key[row + (col << 2)];
    }
  }
}

uint8_t aes_sub_byte(uint8_t byte) {
//  uint8_t S_Row,S_Collum;
//  uint8_t S_Byte;
//
//  S_Row    = ((byte >> 4) & 0x0F);
//  S_Collum = ((byte >> 0) & 0x0F);
//  S_Byte   = S_TABLE [S_Row][S_Collum];

  //return S_TABLE [ ((byte >> 4) & 0x0F) ] [ ((byte >> 0) & 0x0F) ]; // original
  return pgm_read_byte(&(S_TABLE [((byte >> 4) & 0x0F)] [((byte >> 0) & 0x0F)]));
}

void aes_shift_rows(uint8_t (*state)[4]) {
  uint8_t buffer;

  // store 1st byte in buffer
  buffer      = state[1][0];
  // shift all bytes
  state[1][0] = state[1][1];
  state[1][1] = state[1][2];
  state[1][2] = state[1][3];
  state[1][3] = buffer;

  buffer      = state[2][0];
  state[2][0] = state[2][2];
  state[2][2] = buffer;
  buffer      = state[2][1];
  state[2][1] = state[2][3];
  state[2][3] = buffer;

  buffer      = state[3][3];
  state[3][3] = state[3][2];
  state[3][2] = state[3][1];
  state[3][1] = state[3][0];
  state[3][0] = buffer;
}

void aes_mix_columns(uint8_t (*state)[4]) {
  uint8_t col;
  uint8_t a[4], b[4];


  for(col=0; col<4; col++) {
    for (uint8_t row=0; row<4; row++) {
      a[row] =  state[row][col];
      b[row] = (state[row][col] << 1);

      if ((state[row][col] & 0x80) == 0x80) {
        b[row] ^= 0x1B;
      }
    }

    state[0][col] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
    state[1][col] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
    state[2][col] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
    state[3][col] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
  }
}

void aes_calculate_round_key(uint8_t round, uint8_t *round_key) {
  uint8_t b, rcon;
  uint8_t temp[4];


    //Calculate rcon
  rcon = 0x01;
  while(round != 1) {
    b = rcon & 0x80;
    rcon = rcon << 1;

    if (b == 0x80) {
      rcon ^= 0x1b;
    }
    round--;
  }

  // calculate first temp
  // copy laste byte from previous key and subsitute the byte, but shift the array contents around by 1.
    temp[0] = aes_sub_byte(round_key[12 + 1]);
    temp[1] = aes_sub_byte(round_key[12 + 2]);
    temp[2] = aes_sub_byte(round_key[12 + 3]);
    temp[3] = aes_sub_byte(round_key[12 + 0]);

  // xor with rcon
  temp[0] ^= rcon;

  // calculate new key
  for (uint8_t i=0; i<4; i++) {
    for (uint8_t j=0; j<4; j++) {
      round_key[j + (i << 2)]  ^= temp[j];
      temp[j]                   = round_key[j + (i << 2)];
    }
  }
}
