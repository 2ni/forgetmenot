/*
 * simple bitbanging (software) UART to debug output
 * write only
 *
 * TODO: use clk interrupts
 * TODO: use USART TxD (PB2) from chip instead of software solution
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
#define TX_HIGH    PORT_DBG.OUTSET = DBG
#define TX_LOW     PORT_DBG.OUTCLR = DBG
#define UART_DELAY _delay_us(1e6 / (float) UART_BPS) // 19200 -> 52usec

// DEBUG set in Makefile
#ifdef DEBUG
  #define DINIT()               uart_setup()
  #define D(str)                uart_send_string_p(PSTR(str))
  #define DCRLF()               uart_send_string_p(PSTR("\r\n"))
  #define DL(str)               { uart_send_string_p(PSTR(str)); DCRLF(); }
  #define DF(size, format, ...) { char buf[size]; sprintf(buf, format, __VA_ARGS__); uart_send_string(buf); DCRLF(); }
  #define DT_C(key, value)      uart_tuple(PSTR(key), PSTR(value))
  #define DT_S(key, value)      uart_tuple(PSTR(key), value)
  #define DT_I(key, value)      uart_tuple(PSTR(key), value)
  #define DT_IH(key, value)     uart_tuple(PSTR(key), value, 16)
#else
  #define DINIT()
  #define D(str)
  #define DCRLF()
  #define DL(str)
  #define DF(size, format, ...)
  #define DTUPLE(name, value)
#endif

void uart_setup(void);
void uart_tuple(const char* key, const char* value);
void uart_tuple(const char* key, uint16_t value, uint8_t base=10);
void uart_tuple(const char* key, char* value);
void uart_send_char(char c);
void uart_send_string(char* s);
void uart_send_string_p(const char* s);
void uart_send_digit(uint16_t value, uint8_t base=10);

#endif
