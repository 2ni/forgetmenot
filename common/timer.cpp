#include <avr/interrupt.h>
#include "timer.h"
#include "uart.h"

TIMER* TIMER::timer_pointer;

ISR(TCA0_OVF_vect) {
  TIMER::timer_pointer->_timeout = 1;
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // clear int flag
}

TIMER::TIMER(uint32_t mhz) {
  _mhz = mhz;
  init();
}

TIMER::TIMER() {
  _mhz = F_CPU;
  init();
}

void TIMER::init() {
  TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
  timer_pointer = this;
}

void TIMER::start(uint16_t ms) {
  _timeout = 0;
  uint32_t ticks = _mhz / 1000 * ms / 1024; // 10MHz/1024*2000ms/1000ms = 19532 (ensure no overflow by dividing by 1000 1st)
  TCA0.SINGLE.PER = ticks;                           // 10MHz/1024/19532 = 0.5Hz
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc |   // prescaler 1024
  TCA_SINGLE_ENABLE_bm;
  sei();
}

void TIMER::stop() {
  TCA0.SINGLE.CTRLA = 0;
}

uint8_t TIMER::timed_out() {
  return _timeout;
}
