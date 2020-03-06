/*
 * DBG_PORT, DBG_DDR, DBG must be defined
 * to a pin on which TXD is connected to
 */

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"
#include "def.h"

#define UART_SET_BIT(port,bit)   (port |=  (1 << bit))
#define UART_CLEAR_BIT(port,bit) (port &= ~(1 << bit))

#define UART_BIT_WAIT()  _delay_us(1000000.0 / (float) UART_BPS)
#define UART_SEND_HIGH() {UART_CLEAR_BIT(DBG_PORT, DBG); UART_BIT_WAIT();}
#define UART_SEND_LOW()  {UART_SET_BIT(DBG_PORT, DBG); UART_BIT_WAIT();}

uint8_t uart_setup(void){
  UART_SET_BIT(DBG_DDR, DBG);
  UART_SET_BIT(DBG_PORT, DBG);
  _delay_ms(400); // ensure serial is ready
  return 0;
}

void uart_putc(char c) {
  uint8_t i = 8;
  // start bit
  UART_SEND_HIGH();
  while(i){
    if(c & 1){
      UART_SEND_LOW();
    } else {
      UART_SEND_HIGH();
    }
    c = c >> 1;
    i--;
  }
  // stop bits
  UART_SEND_LOW();
}

void uart_puts(char* s) {
  while (*s) {
    uart_putc(*s);
    s++;
  }
}

// https://stackoverflow.com/questions/205529/passing-variable-number-of-arguments-around
/*
void DF(const char *format, ...) {
  va_list vargs;
  va_start(vargs, format);

  char buf[50];
  vsprintf(buf, format, vargs);
  uart_puts(buf);
  uart_puts("\r\n");

  va_end(vargs);
}

void DL(char *buf) {
  uart_puts(buf);
  uart_puts("\r\n");
}

void D(char *buf) {
  uart_puts(buf);
}
*/
