/*
 * inspidred by
 * - http://www.justgeek.de/a-simple-simplex-uart-for-debugging-avrs/
 * - https://github.com/MartinD-CZ/AVR-Li-Ion-charger/blob/master/firmware/ATtiny%20USB%20charger/
 */
#ifndef __UART_H__
#define __UART_H__
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include "def.h"

#define UART_BPS 19200
#define TX_HIGH    PORT_DBG.OUTCLR |= (1<<DBG)
#define TX_LOW     PORT_DBG.OUT    |= (1<<DBG)
#define UART_DELAY _delay_us(F_CPU / (float) UART_BPS)

// DEBUG set in platformio.ini
#ifdef DEBUG
  #define DINIT()              uart_setup()
  #define D(str)               uart_send_string_p(str)
  #define DCRLF()              uart_send_string_p(PSTR("\r\n"))
  #define DL(str)              { uart_send_string_p(PSTR(str)); DCRLF(); }
  #define DF(format, ...)      { char buf[50]; sprintf(buf, format, __VA_ARGS__); uart_send_string(buf); DCRLF(); }
  #define DLTUPLE(name, value) { uart_send_string_p(name); uart_send_string_p(": "); uart_send_string(value); }
#else
  #define DINIT()
  #define D(str)
  #define DCRLF()
  #define DL(str)
  #define DF(format, ...)
  #define DLTUPLE(name, value)
#endif

#define foo 123

uint8_t uart_setup(void);
void uart_send_char(char c);
void uart_send_string(char* s);
void uart_send_string_p(const char* s);
void uart_send_digit(uint16_t value);
/*
void DF(const char *format, ...);
void DL(char *buf);
void D(char *buf);
*/

#endif
