/*
 * simple bitbanging (software) UART to debug output
 * write only
 *
 * TODO: use clk interrupts, eg https://github.com/MarcelMG/AVR8_BitBang_UART_TX/blob/master/main.c
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

#define UART_BPS   19200
#define UART_DELAY _delay_us(1000000.0 / (float) UART_BPS) // 19200 -> 52usec
#define UART_HIGH  PORT_DBG.OUTSET = DBG
#define UART_LOW   PORT_DBG.OUTCLR = DBG

// DEBUG set in Makefile
#ifdef DEBUG
  #define DINIT()            uart_setup()
  #define DLF()              uart_send_string_p(PSTR("\n"))
  #define D(str)             uart_send_string_p(PSTR(str))
  #define DL(str)            { uart_send_string_p(PSTR(str)); DLF(); }
  #define DF(format, ...)    { char uart_buf[50]; sprintf(uart_buf, format, __VA_ARGS__); uart_send_string(uart_buf); DLF(); }
  #define DT_C(key, value)   uart_tuple(PSTR(key), PSTR(value))
  #define DT_S(key, value)   uart_tuple(PSTR(key), value)
  #define DT_I(key, value)   uart_tuple(PSTR(key), value)
  #define DT_IH(key, value)  uart_tuple(PSTR(key), value, 16)
#else
  #define DINIT()
  #define D(str)
  #define DLF()
  #define DL(str)
  #define DF(size, format, ...)
  #define DTUPLE(name, value)
#endif

void uart_setup(void);
void uart_tuple(const char* key, const char* value);
void uart_tuple(const char* key, uint16_t value, uint8_t base=10);
void uart_tuple(const char* key, char* value);
void uart_send_char(unsigned char c);
void uart_send_string(char* s);
void uart_send_string_p(const char* s);
void uart_send_digit(uint16_t value, uint8_t base=10);
void uart_int2float(char *buf, uint16_t value, uint8_t precision=2);

#endif
