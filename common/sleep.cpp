#include "sleep.h"
#include "pins.h"
#include "uart.h"

/*
 * RTC.PER: 16bit max
 * using 1024Hz internal clock
 *
 * max sleep = 2^16*1000*prescaler/1024
 * min sleep = 1*1000*prescaler/1024
 *
 * -> 0.98ms  - 64s      (prescaler 1)     00:00:01:04
 *    1.95ms  - 128s     (prescaler 2)     00:00:02:08
 *    3.91ms  - 256s     (prescaler 4)     00:00:04:16
 *    7.81ms  - 512s     (prescaler 8)     00:00:08:32
 *    15.6ms  - 1024s    (prescaler 16)    00:00:17:04
 *    31.25ms - 2048s    (prescaler 32)    00:00:34:08
 *    62.5ms  - 4096s    (prescaler 64)    00:01:08:16
 *    125ms   - 8192s    (prescaler 128)   00:02:16:32
 *    250ms   - 16384s   (prescaler 256)   00:04:33:04
 *    500ms   - 32768s   (prescaler 512)   00:09:06:08
 * -> 1s      - 65536s   (prescaler 1024)  00:18:12:16
 *    2s      - 131072s  (prescaler 2048)  01:12:24:32
 *    4s      - 262144s  (prescaler 4096)  03:00:49:40
 *    8s      - 524288s  (prescaler 8192)  06:01:38:08
 *    16s     - 1048576s (prescaler 16384) 12:03:16:16
 *    32s     - 2097152s (prescaler 32768) 24:06:32:32
 */

ISR(RTC_CNT_vect) {
  RTC.INTFLAGS = RTC_OVF_bm;
}

/*
 * prescaler
 * 0 -> :1
 * 1 -> :2
 * 2 -> :4
 * 3 -> :8
 * ...
 * 9 -> :512
 */
void _s_sleep(uint16_t per, uint8_t prescaler) {
  while (RTC.STATUS > 0);             // wait for all register to be synchronized

  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;     // 32.768kHz / 32 -> 1024Hz
  RTC.PER = per;
  RTC.INTCTRL = (1 << RTC_OVF_bp);      // overflow interrupt
  RTC.CTRLA = (prescaler<<3) | RTC_RTCEN_bm | RTC_RUNSTDBY_bm;

  /*
  _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA, CLKCTRL_ENABLE_bm | CLKCTRL_RUNSTDBY_bm); // external crystal, runstdby, enable
  RTC.CLKSEL = (0<<CLKCTRL_ENABLE_bp);
  while (CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm); // wait until status = 0
  RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc | CLKCTRL_ENABLE_bm;
  */

  RTC.CNT = 0;

  sei();

  SLPCTRL.CTRLA = (SLPCTRL_SMODE_STDBY_gc | SLPCTRL_SEN_bm); // idle, standby or power down
  asm("sleep");

  // while (RTC.STATUS > 0); // somehow this blocks ~1ms and things still work if commented out
  // seems that resetting CLKSEL and CTRLA make the inital RTC.STATUS faster
  RTC.CLKSEL = 0;
  RTC.CTRLA = 0; // turn off RTC
}

/*
 * prescaler 1
 *
 * 1ms - 64'000ms
 * (precision: 1000/1024 = 0.9765625 per bit)
 * max 2^16bit
 *
 * division is slow! if you need speed, use _s_sleep() and precalculate per
 *
 */
void sleep_ms(uint16_t ms) {
  _s_sleep(((uint32_t)ms*1024)/1000, 0);
}

/* prescaler 1024
 *
 * 1s - 65536s (precision: 1000/1024*1024 = 1s per bit)
 * max 2^16bit
 */
void sleep_s(uint16_t seconds) {
  _s_sleep(seconds, 0xa);
}
