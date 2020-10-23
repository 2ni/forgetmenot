#ifndef __TWI_H__
#define __TWI_H__

#include <avr/io.h>

#define TWI0_BAUD(F_SCL)      ((((float)F_CPU / (float)F_SCL)) - 10 )

void twi_init();
uint8_t twi_start(uint8_t addr);
uint8_t twi_read(uint8_t ack);
uint8_t twi_write(uint8_t data);
void twi_stop(void);


#endif
