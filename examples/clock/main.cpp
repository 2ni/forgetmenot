/*
 *
 * calibrate clock speed
 * check bod (brwon out detection)
 *
 */

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "sleep.h"
#include "millis.h"
#include "timer.h"

uint32_t now;

ISR(TCB1_INT_vect) {
  TCB1.INTFLAGS = TCB_CAPT_bm;
  PORTB.OUTTGL = (1<<0);
  // do not use function call in isr to allow speed
  // if not, max frequ was ~58kHz
  // toggle_output(&lcd_blk);
}


int main(void) {
  /*
  // use internal 32.768kHz as main clock
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKOUT_bm | CLKCTRL_CLKSEL_OSCULP32K_gc);
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0); // no prescaler
  */

  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  uint32_t node = get_deviceid();
  // see https://stackoverflow.com/questions/4842424/list-of-ansi-color-escape-sequences for colors
  DF("\n\033[1;38;5;226;48;5;18m Hello from 0x%06lX \033[0m\n", node);

  led_flash(&led_g, 3);

  // load error compensation and calculate real frequency
  int8_t  sigrow_val    = SIGROW.OSC20ERR3V;
  uint32_t frq = F_CPU + F_CPU*sigrow_val/1024;
  DF("frq: %lu\n", frq);

  // pwm signal on BLK pin (PB0) should be 10kHz
  // some examples:
  // see http://www.ghart.com/blf/firmware/Tests/pwm_current_test.c
  // https://github.com/microchip-pic-avr-examples/attiny817-realistic-heartbeat-led-cip-video-series-studio
  PORTB.DIRSET = (1<<0); // set lcd_blk as output
  TCB1.CNT = 0;
  TCB1.CCMP = (frq/10000/2)-1; // 10kHz
  TCB1.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;
  TCB1.CTRLB = 0; // periodic interrupt
  TCB1.INTCTRL = TCB_CAPT_bm;
  sei();

  while(1) {
    DL("\ntest delay timing without error compensation.");
    uint32_t now;
    millis_init();
    DL("start");
    now = millis_time();
    while((millis_time()-now) < 5000); // wait
    DL("window 1");
    while((millis_time()-now) < 6000); // wait
    DL("window 2");

    DL("\ntest delay timing with error compensation.");
    millis_init(frq);
    DL("start");
    now = millis_time();
    while((millis_time()-now) < 5000); // wait
    DL("window 1");
    while((millis_time()-now) < 6000); // wait
    DL("window 2");

    DL("\ntest sleep with internal 32.768kHz clock");
    DL("start");
    sleep_ms(5000);
    DL("window 1");
    sleep_ms(1000);
    DL("window 2");
  }
}
