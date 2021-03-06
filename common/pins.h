/*
 * pin configuration settings for forgetmenot
 */

#ifndef __PINS_H__
#define __PINS_H__

#include <avr/io.h>

// PORT A
#define OUT2       PIN7_bm // PA7

// PORT B
#define BLK        PIN0_bm
#define DBG        PIN1_bm
#define CS_LCD     PIN2_bm
#define DC         PIN3_bm
#define RST        PIN4_bm
#define OUT1       PIN7_bm

#define PORT_DBG   PORTB

// PORT C
#define HALL       PIN3_bm
#define MULTI      PIN4_bm

#define PORT_HALL  PORTC
#define PORT_HALL  PORTC

typedef struct {
  PORT_t *port;
  uint8_t pin;
  ADC_t *port_adc;
  uint8_t pin_adc;
} pin_t;

extern pin_t pin_touch, pin_moisture, pin_temp_board, pin_temp_moisture, led_g, led_b;
extern pin_t out1, out2;
extern pin_t rfm_cs, rfm_interrupt, rfm_timeout;
extern pin_t multi;
extern pin_t mosi, miso, sck;
extern pin_t lcd_blk, lcd_cs, lcd_dc, lcd_rst;

void disable_buffer_of_pins();
void clear_all_pins();
void set_direction(pin_t *pin, uint8_t direction=1);
void set_output(pin_t *pin, uint8_t value);
void toggle_output(pin_t *pin);
uint8_t get_output(pin_t *pin);
void set_pullup(pin_t *pin, uint8_t clear=0);

uint8_t adc_is_running(pin_t *pin);
uint16_t get_adc(pin_t *pin, int8_t muxpos=-1);

uint32_t get_deviceid();

#endif
