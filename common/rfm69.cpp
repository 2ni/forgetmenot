#include <avr/interrupt.h>

#include "rfm69.h"
#include "rfm69_registers.h"
#include "spi.h"
#include "pins.h"
#include "uart.h"
#include "timer.h"

volatile uint8_t DATALEN;
volatile uint16_t SENDERID;
volatile uint16_t TARGETID;
volatile uint8_t PAYLOADLEN;
volatile uint8_t ACK_RECEIVED;
volatile uint8_t ACK_REQUESTED;
volatile int16_t RSSI;
volatile uint8_t DATA[RF69_MAX_DATA_LEN+1];  // RX/TX payload buffer, including end of string NULL char

volatile uint8_t mode = RF69_MODE_STANDBY;
volatile uint8_t inISR = 0;
uint8_t is_rfm69hw = 1; // high power mode available
uint8_t power_level = 31;
uint16_t address; // device id
uint8_t promiscuous_mode = 0;

uint8_t rfm69_init(uint16_t freq, uint16_t node_id, uint8_t network_id) {
  const uint8_t CONFIG[][2] = {
    /* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
    /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, // no shaping
    /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_55555}, // default: 4.8 KBPS
    /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_55555},
    /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_50000}, // default: 5KHz, (FDEV + BitRate / 2 <= 500KHz)
    /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_50000},

    //* 0x07 */ { REG_FRFMSB, RF_FRFMSB_433},
    //* 0x08 */ { REG_FRFMID, RF_FRFMID_433},
    //* 0x09 */ { REG_FRFLSB, RF_FRFLSB_433},

    /* 0x07 */ { REG_FRFMSB, (uint8_t) (freq==RF_315MHZ ? RF_FRFMSB_315 : (freq==RF_433MHZ ? RF_FRFMSB_433 : (freq==RF_868MHZ ? RF_FRFMSB_868 : RF_FRFMSB_915))) },
    /* 0x08 */ { REG_FRFMID, (uint8_t) (freq==RF_315MHZ ? RF_FRFMID_315 : (freq==RF_433MHZ ? RF_FRFMID_433 : (freq==RF_868MHZ ? RF_FRFMID_868 : RF_FRFMID_915))) },
    /* 0x09 */ { REG_FRFLSB, (uint8_t) (freq==RF_315MHZ ? RF_FRFLSB_315 : (freq==RF_433MHZ ? RF_FRFLSB_433 : (freq==RF_868MHZ ? RF_FRFLSB_868 : RF_FRFLSB_915))) },


    // looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
    // +17dBm and +20dBm are possible on RFM69HW
    // +13dBm formula: Pout = -18 + OutputPower (with PA0 or PA1**)
    // +17dBm formula: Pout = -14 + OutputPower (with PA1 and PA2)**
    // +20dBm formula: Pout = -11 + OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
    ///* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111},
    ///* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, // over current protection (default is 95mA)

    // RXBW defaults are { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_5} (RxBw: 10.4KHz)
    /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2 }, // (BitRate < 2 * RxBw)
    //for BR-19200: /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_3 },
    /* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, // DIO0 is the only IRQ we're using
    /* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, // DIO5 ClkOut disable for power saving
    /* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // writing to this bit ensures that the FIFO & status flags are reset
    /* 0x29 */ { REG_RSSITHRESH, 220 }, // must be set to dBm = (-Sensitivity / 2), default is 0xE4 = 228 so -114dBm
    ///* 0x2D */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE } // default 3 preamble bytes 0xAAAAAA
    /* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0 },
    /* 0x2F */ { REG_SYNCVALUE1, 0x2D },      // attempt to make this compatible with sync1 byte of RFM12B lib
    /* 0x30 */ { REG_SYNCVALUE2, network_id }, // NETWORK ID
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_ON | RF_PACKET1_CRCAUTOCLEAR_ON | RF_PACKET1_ADRSFILTERING_OFF },
    /* 0x38 */ { REG_PAYLOADLENGTH, 66 }, // in variable length mode: the max frame size, not used in TX
    ///* 0x39 */ { REG_NODEADRS, node_id }, // turned off because we're not using address filtering
    /* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, // TX on FIFO not empty
    /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, // RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //for BR-19200: /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, // RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode for Fading Margin Improvement, recommended default for AfcLowBetaOn=0
    {255, 0}
  };
  spi_init();
  set_direction(&rfm_cs, 1); // set as output
  set_output(&rfm_cs, 1);    // set high
  // set pull down. isr on rising

  // simple check
  uint8_t version = rfm69_read_reg(REG_VERSION);
  if (version != 0x24) {
    return 0; // failure
  }

  set_direction(&rfm_interrupt, 0); // set interrupt pin as input (might need pull down)

  while (rfm69_read_reg(REG_SYNCVALUE1) != 0xaa) {
    rfm69_write_reg(REG_SYNCVALUE1, 0xaa);
  }

  while (rfm69_read_reg(REG_SYNCVALUE1) != 0x55) {
    rfm69_write_reg(REG_SYNCVALUE1, 0x55);
  }

  for (uint8_t i = 0; CONFIG[i][0] != 255; i++) {
    rfm69_write_reg(CONFIG[i][0], CONFIG[i][1]);
  }

  encrypt(0); // disable during init to start from a known state
  set_high_power();
  set_mode(RF69_MODE_STANDBY);
  while ((rfm69_read_reg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00);

  // set isr for interrupt pin rising rmf_interrupt = PA5
  // TODO replace PIN5CTRL with pin
  rfm_interrupt.port->PIN5CTRL |= PORT_ISC_RISING_gc;

  address = node_id;
  set_network(network_id);

  return version;
}

/*
 * get lower byte first
 * then higher byte
 * and convert it to uint16_t
 */
uint16_t get_id() {
  return (uint16_t)(spi_transfer_byte(0)<<8 | (spi_transfer_byte(0)));
}

/*
 * get data interrupt service routine
 *
 * id's of devices is max 16bit
 * LSB first
 *
 * see packet structure of payload https://lowpowerlab.com/2013/06/20/rfm69-library/
 * header(6): len(1), destination(2), sender(2), ctl(1)
 *
 */
ISR(PORTA_PORT_vect) {
  inISR = 1;
  if (mode == RF69_MODE_RX && (rfm69_read_reg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)) {
    set_mode(RF69_MODE_STANDBY);
    rfm69_select();
    spi_transfer_byte(REG_FIFO & 0x7F);
    PAYLOADLEN = spi_transfer_byte(0);
    if (PAYLOADLEN>66) PAYLOADLEN=66;
    TARGETID = get_id();
    if (!(promiscuous_mode || TARGETID == address || TARGETID == RF69_BROADCAST_ADDR) // match this node's address, or broadcast address or anything in promiscuous mode
    || PAYLOADLEN < 5) { // address situation could receive packets that are malformed and don't fit this libraries extra fields
      PAYLOADLEN = 0;
      rfm69_unselect();
      receive_begin();
      return;
    }

    SENDERID = get_id();
    DATALEN = PAYLOADLEN - 5; // uin8t_t len, uint16_t dest, uint16_t sender, uin8t_t ctl
    uint8_t ctl_byte = spi_transfer_byte(0);

    ACK_RECEIVED = ctl_byte & RFM69_CTL_SENDACK; // extract ACK-received flag
    ACK_REQUESTED = ctl_byte & RFM69_CTL_REQACK; // extract ACK-requested flag

    for (uint8_t i = 0; i < DATALEN; i++) {
        DATA[i] = spi_transfer_byte(0);
    }
    if (DATALEN < RF69_MAX_DATA_LEN) DATA[DATALEN] = 0; // add null at end of string
    rfm69_unselect();
    set_mode(RF69_MODE_RX);
  }
  RSSI = read_rssi(0);
  inISR = 0;

  rfm_interrupt.port->INTFLAGS |= (1<<rfm_interrupt.pin); // clear interrupt flag
}

/*
 * get data from buffer if receive_done()
 * and return RSSI
 */
int16_t get_data(char *data, uint8_t size) {
  if (size > RF69_MAX_DATA_LEN) size = RF69_MAX_DATA_LEN;
  for (uint8_t i=0; i<size; i++) {
    data[i] = DATA[i];
  }
  return RSSI;
}

/*
 * enable spi transfer
 */
void rfm69_select() {
  set_output(&rfm_cs, 0);
  cli();
}

/*
 * disable spi transfer
 */
void rfm69_unselect() {
  set_output(&rfm_cs, 1);
  if (!inISR) sei();
}

uint8_t rfm69_read_reg(uint8_t addr) {
  rfm69_select();
  spi_transfer_byte(addr & 0x7F);
  uint8_t val = spi_transfer_byte(0);
  rfm69_unselect();

  return val;
}

void rfm69_write_reg(uint8_t addr, uint8_t value) {
  rfm69_select();
  spi_transfer_byte(addr | 0x80);
  spi_transfer_byte(value);
  rfm69_unselect();
}

/*
 * enable encryption: encrypt("ABCDEFGHIJKLMNOP"); (must be 16 bytes)
 * disable encryption: encrypt(null) or encrypt(0)
 */
void encrypt(const char* key) {
  set_mode(RF69_MODE_STANDBY);
  if (key != 0) {
    rfm69_select();
    spi_transfer_byte(REG_AESKEY1 | 0x80);
    for (uint8_t i=0; i<16; i++) {
      spi_transfer_byte(key[i]);
    }
    rfm69_unselect();
  }
  rfm69_write_reg(REG_PACKETCONFIG2, (rfm69_read_reg(REG_PACKETCONFIG2) & 0xFE) | (key ? 1:0));
}

void set_mode(uint8_t new_mode) {
  if (new_mode == mode) return;

  switch (new_mode) {
    case RF69_MODE_TX:
      rfm69_write_reg(REG_OPMODE, (rfm69_read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
      if (is_rfm69hw) set_high_power_regs(1);
      break;
    case RF69_MODE_RX:
      rfm69_write_reg(REG_OPMODE, (rfm69_read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
      if (is_rfm69hw) set_high_power_regs(0);
      break;
    case RF69_MODE_SYNTH:
      rfm69_write_reg(REG_OPMODE, (rfm69_read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
      break;
    case RF69_MODE_STANDBY:
      rfm69_write_reg(REG_OPMODE, (rfm69_read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
      break;
    case RF69_MODE_SLEEP:
      rfm69_write_reg(REG_OPMODE, (rfm69_read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
      break;
    default:
      return;
  }
  // we are using packet mode, so this check is not really needed
  // but waiting for mode ready is necessary when going from sleep because the FIFO may not be immediately available from previous mode
  while (mode == RF69_MODE_SLEEP && (rfm69_read_reg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // wait for ModeReady
  mode = new_mode;
}

void set_high_power_regs(uint8_t on_off) {
  if (on_off == 1) {
    rfm69_write_reg(REG_TESTPA1, 0x5D);
    rfm69_write_reg(REG_TESTPA2, 0x7C);
  } else {
    rfm69_write_reg(REG_TESTPA1, 0x55);
    rfm69_write_reg(REG_TESTPA2, 0x70);
  }
}

void set_high_power() {
  rfm69_write_reg(REG_OCP, is_rfm69hw ? RF_OCP_OFF : RF_OCP_ON);
  if (is_rfm69hw == 1)
    rfm69_write_reg(REG_PALEVEL, (rfm69_read_reg(REG_PALEVEL) & 0x1F) | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON); // enable P1 & P2 amplifier stages
  else
    rfm69_write_reg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | power_level); // enable P0 only
}

void set_power_level(uint8_t level) {
  power_level = (level > 31 ? 31 : level);
  if (is_rfm69hw == 1) power_level /= 2;
  rfm69_write_reg(REG_PALEVEL, (rfm69_read_reg(REG_PALEVEL) & 0xE0) | power_level);

}

void rfm69_sleep() {
  set_mode(RF69_MODE_SLEEP);
}

uint8_t read_temperature() {
  set_mode(RF69_MODE_STANDBY);
  rfm69_write_reg(REG_TEMP1, RF_TEMP1_MEAS_START);
  while ((rfm69_read_reg(REG_TEMP1) & RF_TEMP1_MEAS_RUNNING));
  return ~rfm69_read_reg(REG_TEMP2) + COURSE_TEMP_COEF;
}

int16_t read_rssi(uint8_t force_trigger) {
  int16_t rssi = 0;
  if (force_trigger == 1) {
    // RSSI trigger not needed if DAGC is in continuous mode
    rfm69_write_reg(REG_RSSICONFIG, RF_RSSI_START);
    while ((rfm69_read_reg(REG_RSSICONFIG) & RF_RSSI_DONE) == 0x00); // wait for RSSI_Ready
  }
  rssi = -rfm69_read_reg(REG_RSSIVALUE);
  rssi >>= 1;
  return rssi;
}

uint8_t ack_received(uint8_t node_id) {
  if (receive_done())
    return (SENDERID == node_id || node_id == RF69_BROADCAST_ADDR) && ACK_RECEIVED;

  return 0;
}

uint8_t can_send() {
  if (mode == RF69_MODE_RX && PAYLOADLEN == 0 && read_rssi(0) < CSMA_LIMIT) { // if signal stronger than -100dBm is detected assume channel activity
    set_mode(RF69_MODE_STANDBY);
    return 1;
  }
  return 0;
}

/*
 * checks if a packet was received and/or puts transceiver in receive (RX or listen) mode
 */
uint8_t receive_done() {
  cli();
  if (mode == RF69_MODE_RX && PAYLOADLEN > 0) {
    set_mode(RF69_MODE_STANDBY); // enables interrupts
    return 1;
  } else if (mode == RF69_MODE_RX) { // already in RX, no payload yet
    sei(); // explicitely re-enable interrupts
    return 0;
  }
  receive_begin();
  sei();
  return 0;
}

void receive_begin() {
  DATALEN = 0;
  SENDERID = 0;
  TARGETID = 0;
  PAYLOADLEN = 0;
  ACK_REQUESTED = 0;
  ACK_RECEIVED = 0;
  RSSI = 0;

  if (rfm69_read_reg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)
    rfm69_write_reg(REG_PACKETCONFIG2, (rfm69_read_reg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks

  rfm69_write_reg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01); // set DIO0 to "PAYLOADREADY" in receive mode
  set_mode(RF69_MODE_RX);
}

void send(uint16_t to, const void* buffer, uint8_t size, uint8_t request_ack) {
  rfm69_write_reg(REG_PACKETCONFIG2, (rfm69_read_reg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  while (!can_send()) receive_done();
  send_frame(to, buffer, size, request_ack, 0);
}

/*
 * timeout is handled by a timer
 * pass a pointer to a timer class
 * eg
 * include "timer.h"
 * TIMER timer;
 * send_retry(..., &timer);
 *
 */
uint8_t send_retry(uint16_t to, const void* buffer, uint8_t size, uint8_t retries, TIMER* ptimer) {
  for (uint8_t i=0; i<=retries; i++) {
    send(to, buffer, size, 1);
    ptimer->start(50); // timeout 50ms
    while (!ptimer->timed_out()) {
      if (ack_received(to)) return 1;
    }
  }
  return 0;
}

void send_frame(uint16_t to, const void* buffer, uint8_t size, uint8_t request_ack, uint8_t send_ack) {
  // DF("%u: '%s' (%u)", to, (char*)buffer, size);
  set_mode(RF69_MODE_STANDBY); // turn off receiver to prevent reception while filling fifo
  while ((rfm69_read_reg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // wait for ModeReady
  if (size > RF69_MAX_DATA_LEN) size = RF69_MAX_DATA_LEN;

  // control byte
  uint8_t ctl_byte = 0x00;
  if (send_ack)
    ctl_byte = RFM69_CTL_SENDACK;
  else if (request_ack)
    ctl_byte = RFM69_CTL_REQACK;

  // write to FIFO
  rfm69_select();
  spi_transfer_byte(REG_FIFO | 0x80);
  spi_transfer_byte(size + 5);
  spi_transfer_byte((uint8_t)((to >> 8) & 0xff)); // high byte
  spi_transfer_byte((uint8_t)(to & 0xff)); // low byte
  spi_transfer_byte((uint8_t)((address >> 8) & 0xff)); // high byte
  spi_transfer_byte((uint8_t)(address & 0xff)); // low byte
  spi_transfer_byte(ctl_byte);

  for (uint8_t i = 0; i < size; i++)
    spi_transfer_byte(((uint8_t*) buffer)[i]);
  rfm69_unselect();

  // no need to wait for transmit mode to be ready since its handled by the radio
  set_mode(RF69_MODE_TX);
  //uint32_t txStart = millis();
  //while (digitalRead(_interruptPin) == 0 && millis() - txStart < RF69_TX_LIMIT_MS); // wait for DIO0 to turn HIGH signalling transmission finish
  while ((rfm69_read_reg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT) == 0x00); // wait for PacketSent
  set_mode(RF69_MODE_STANDBY);
}

void promiscuous(uint8_t on_off) {
  promiscuous_mode = on_off;
  if (promiscuous_mode == 0)
    rfm69_write_reg(REG_PACKETCONFIG1, (rfm69_read_reg(REG_PACKETCONFIG1) & 0xF9) | RF_PACKET1_ADRSFILTERING_NODEBROADCAST);
  else
    rfm69_write_reg(REG_PACKETCONFIG1, (rfm69_read_reg(REG_PACKETCONFIG1) & 0xF9) | RF_PACKET1_ADRSFILTERING_OFF);
}

void set_network(uint8_t id) {
  rfm69_write_reg(REG_SYNCVALUE2, id);
}

void send_ack(const void *buffer, uint8_t size) {
  ACK_REQUESTED = 0;   // TWS added to make sure we don't end up in a timing race and infinite loop sending Acks
  uint8_t sender = SENDERID;
  int16_t _RSSI = RSSI; // save payload received RSSI value
  rfm69_write_reg(REG_PACKETCONFIG2, (rfm69_read_reg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  while (!can_send()) receive_done();
  SENDERID = sender;    // TWS: Restore SenderID after it gets wiped out by receiveDone() n.b. actually now there is no receiveDone() :D
  send_frame(sender, buffer, size, 0, 1);
  RSSI = _RSSI; // restore payload RSSI
}

uint8_t ack_requested() {
  return ACK_REQUESTED && (TARGETID != RF69_BROADCAST_ADDR);
}
