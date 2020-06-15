/*
 * control led by interrups and timer
 * isrt should always be kept as short as possible
 * that's why we work with an timer bitmask timers_done
 * containing information of potentially several counts
 */

#include <avr/interrupt.h>

#include "pins.h"
#include "uart.h"
#include "led.h"

#define timer_bm 0x01
#define timer_bp 0

volatile uint8_t timers_done = 0x10;                 // we set an higher bit for control

ISR(TCA0_OVF_vect) {
  timers_done |= (1<<timer_bp);                      // set bit at position 0

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;          // clear interrupt flag
}


void timer_init(void) {

  TCA0.SINGLE.PER = 4883;                            // 10MHz/1024/4883 = 2Hz

  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc   // prescaler 1024
    | TCA_SINGLE_ENABLE_bm;                          // enable counter

  TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
}

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  DL("Hello.");

  led_setup();
  led_flash('g', 3);
  led_on('g');

  timer_init();
  sei();

  while (1) {
    if (timers_done & timer_bm) {                   // check if bit set
      DT_IH("timers", timers_done);                 // should print 0x11 every .5sec
      timers_done &= ~(1<<timer_bp);
      led_toggle('b');
      led_toggle('g');
    }
  }
}
