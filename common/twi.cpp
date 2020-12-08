#include "twi.h"
#include "pins.h"

void twi_init() {
  set_pullup(&mosi, 1);
  set_pullup(&miso, 1);
  PORTMUX.CTRLB = PORTMUX_TWI0_ALTERNATE_gc; // use alternate pins for scl = pa2 (miso), sda = pa1 (mosi)
  TWI0.MBAUD = (uint8_t)TWI0_BAUD(400000);
  TWI0.MCTRLA = TWI_ENABLE_bm;
  TWI0.MCTRLB = 0;
  TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

uint8_t twi_start(uint8_t addr) {
  if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_BUSY_gc) { // Verify Bus is not busy
    TWI0.MCTRLB &= ~(1 << TWI_ACKACT_bp);
    TWI0.MADDR = addr ;
    if (addr & 0x01) {
      while (!(TWI0.MSTATUS & TWI_RIF_bm));
    } else {
      while (!(TWI0.MSTATUS & TWI_WIF_bm));
    }
    return 0;
  } else {
    return 1; // Bus is busy
  }
}

uint8_t twi_write(uint8_t data) {
  if ((TWI0.MSTATUS&TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc) { // Verify Master owns the bus
    TWI0.MDATA = data;
    while (!((TWI0.MSTATUS & TWI_WIF_bm) | (TWI0.MSTATUS & TWI_RXACK_bm))); // Wait until WIF set and RXACK cleared
    return 0;
  } else {
    return 1;	//Master does not own the bus
  }
}

uint8_t twi_read(uint8_t ack) { // ack=1 send ack ; ack=0 send nack
  if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc) { // Verify Master owns the bus
    while (!(TWI0.MSTATUS & TWI_RIF_bm)); // Wait until RIF set
    uint8_t data = TWI0.MDATA;
    // si ack=1 mise à 0 ACKACT => action send ack
    // sinon (ack=0) => mise à 1 ACKACT => nack préparé pour actionstop
    if (ack==1) {
      TWI0.MCTRLB &= ~(1<<TWI_ACKACT_bp);
    } else {
      TWI0.MCTRLB |= (1<<TWI_ACKACT_bp);
    }

    return data ;
  } else {
    return 1; // Master does not own the bus
  }
}

void twi_stop(void) {
  TWI0.MCTRLB |= TWI_ACKACT_NACK_gc;
  TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}
