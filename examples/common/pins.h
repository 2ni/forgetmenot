/*
 * pin configuration settings for forgetmenot
 */

#ifndef __PINS_H__
#define __PINS_H__

#include <avr/io.h>

// PORT A
#define MOSI       PIN1_bm
#define MISO       PIN2_bm
#define SCK        PIN3_bm
#define CS_RFM     PIN4_bm
#define DIO0       PIN5_bm
#define DIO1       PIN6_bm
#define OUT2       PIN7_bm

#define PORT_SPI   PORTA
#define PORT_RFM   PORTA

// PORT B
#define BLK        PIN0_bm
#define DBG        PIN1_bm
#define LED_R      PIN1_bm
#define CS_LCD     PIN2_bm
#define DC         PIN3_bm
#define RST        PIN4_bm
#define LED_B      PIN5_bm
#define LED_G      PIN6_bm
#define OUT1       PIN7_bm

#define PORT_LED   PORTB
#define PORT_DBG   PORTB
#define PORT_LCD   PORTB

// PORT C
#define TEMPBOARD  PIN0_bm // temp1
#define TEMPSENSOR PIN2_bm // temp2
#define HALL       PIN3_bm
#define MULTI      PIN4_bm

#define PORT_TEMP  PORTC
#define PORT_HALL  PORTC
#define PORT_HALL  PORTC

typedef struct {
  PORT_t *port;
  uint8_t pin;
  ADC_t *port_adc;
  uint8_t pin_adc;
} pin_t;

extern pin_t pin_touch, pin_moisture;

void set_direction(pin_t *pin, uint8_t input=0);
void set_pullup(pin_t *pin, uint8_t clear=0);

uint8_t adc_is_running(pin_t *pin);
uint16_t get_adc(pin_t *pin, int8_t muxpos=-1);

uint32_t get_deviceid();

#endif
