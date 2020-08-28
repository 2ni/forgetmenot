#include "sleep.h"
#include "pins.h"

ISR(RTC_CNT_vect) {
  RTC.INTFLAGS = RTC_OVF_bm;
}

/*
 * RTC.PER: 16bit max
 * -> max 65 seconds
 */
void sleep_ms(uint32_t ms) {
  while (RTC.STATUS > 0) {}             // wait for all register to be synchronized

  RTC.INTCTRL = 0 << RTC_CMP_bp         // compare match
    | 1 << RTC_OVF_bp;                  // overflow interrupt

  RTC.CTRLA = RTC_PRESCALER_DIV1_gc     // prescaler 1
    | 1 << RTC_RTCEN_bp                 // enable rtc
    | 1 << RTC_RUNSTDBY_bp;             // run in standby

  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;     // 32.768kHz / 32 -> 1024Hz

  sei();

  // DF("per: %u\n", (uint16_t)(ms*1024/1000));
  RTC.PER = (ms*1024)/1000;
  SLPCTRL.CTRLA |= (SLPCTRL_SMODE_STDBY_gc | SLPCTRL_SEN_bm); // idle, standby or power down

  asm("sleep");
}
