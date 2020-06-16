#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

#include <avr/io.h>

#include "pins.h"

typedef struct {
  int16_t temp;
  uint16_t adc;
} temp_characteristics_struct;

uint16_t get_temp(pin_t *pin);
uint16_t get_temp_board();
uint16_t get_temp_moist();
uint16_t get_temp_chip();
int16_t  adc2temp(uint16_t adc);
int16_t  interpolate(uint16_t adc, const temp_characteristics_struct *characteristics, uint8_t size);

uint16_t get_vcc();

extern const uint8_t temp_characteristics_size;
extern const temp_characteristics_struct temp_characteristics[];

#endif
