#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

#include <avr/io.h>

typedef struct {
  int16_t temp;
  uint16_t adc;
} temp_characteristics_struct;

uint16_t measure_temperature(char device='b');
uint16_t measure_internal_temperature();
int16_t  adc2temp(uint16_t adc);
int16_t  interpolate(uint16_t adc, const temp_characteristics_struct *characteristics, uint8_t size);

uint16_t measure_vcc();

extern const uint8_t temp_characteristics_size;
extern const temp_characteristics_struct temp_characteristics[];

#endif
