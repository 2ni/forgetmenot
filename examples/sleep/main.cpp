#include <avr/interrupt.h>
#include "pins.h"
#include "uart.h"
#include "led.h"
#include "sleep.h"
#include "motor.h"

/*
 * simple standby example
 * periphery is off by default unless activated by RUNSTDBY
 * ports stay on in standby mode
 */
int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSCULP32K_gc); // select internal 32.768kHz as main clock to save power

  /*
  while (RTC.STATUS > 0) {}

  RTC.INTCTRL = 0 << RTC_CMP_bp         // compare match
    | 1 << RTC_OVF_bp;                  // overflow interrupt
  RTC.PER = 5000;

  RTC.CTRLA = RTC_PRESCALER_DIV1_gc	//NO Prescaler
    | RTC_RTCEN_bm       	//Enable RTC
    | RTC_RUNSTDBY_bm;   	//Run in standby

  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;

  sei();

  SLPCTRL.CTRLA |= (SLPCTRL_SMODE_STDBY_gc | SLPCTRL_SEN_bm);
  asm("sleep");
  while(1) {}
  */

  disable_buffer_of_pins();

  DINIT(); // for unknown reasons UART_HIGH consumes ~0.6mA if CH330 connected (even if not powered)
  DF("Hello from 0x%06lX\n", get_deviceid());

  clear_all_pins(); // this is important to get real low power. Only keep the ones you really need, eg led_b
  led_flash(&led_b, 3);

  while (1) {
    led_on(&led_b);
    sleep_ms(500);
    led_off(&led_b);

    sleep_ms(10000);
  }
}
