/*
 * pin configuration settings for forgetmenot
 */

#ifndef __DEF_H__
#define __DEF_H__

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
#define MOIST      PIN1_bm
#define TEMPSENSOR PIN2_bm // temp2
#define HALL       PIN3_bm
#define MULTI      PIN4_bm
#define TOUCH      PIN5_bm

#define PORT_MOIST PORTC
#define PORT_TEMP  PORTC
#define PORT_HALL  PORTC
#define PORT_TOUCH PORTC
#define PORT_HALL  PORTC

#endif
