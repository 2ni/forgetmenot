#include <util/delay.h>
#include <string.h>

#include "uart.h"
#include "def.h"
#include "led.h"

/*
 * https://github.com/tuomasnylund/avr-capacitive-touch/blob/master/code/touch.c
 *
typedef struct {
    volatile uint8_t *port;    //PORTx register for pin
    volatile uint8_t portmask; //mask for the bit in port
    volatile uint8_t mux;      //ADMUX value for the channel
    uint16_t min;
    uint16_t max;
} touch_channel_t;

static touch_channel_t btn1 = {
    .mux = 7,
    .port = &PORTF,
    .portmask = (1<<PF7),
    .min = 500,
    .max = 800
};

touch_measure(&btn1);

touch_measure(*channe) {
  channerl->portmask;
}
*/


// PC5 = AIN11 (ADC1)
#define PORT_BUTTON     PORTC
#define PIN_BUTTON      PIN5_bm
#define ADC_BUTTON_gc   ADC_MUXPOS_AIN11_gc
#define BUTTON_CTRL     PIN5CTRL
#define PORT_ADC        ADC1

/*
 * Ctouch: capacity on touch pin
 * Csh   : capacity s/h of adc (sample and hold)
 *
 *        Ctouch
 * Vsh = --------   * Vcc
 *      (Ctouch+Csh)
 *
 * measurement:
 * 1. discharge Csh and Ctouch
 * 2. charge Ctouch to Vcc
 * 3. disable charge (pullup)
 * 4. connect Ctouch to Csh (by enabling adc
 * 5. measure voltage. the higher the value, the higher Ctouch
 *
 */

uint16_t touch() {
  uint16_t result;

  PORT_BUTTON.DIRSET = PIN_BUTTON; // set output and low to discharge touch
  PORT_ADC.MUXPOS = ADC_MUXPOS_GND_gc; // discharge s/h cap by setting muxpos = gnd and run
  PORT_ADC.CTRLA = (1<<ADC_ENABLE_bp) | (0<<ADC_FREERUN_bp) | ADC_RESSEL_10BIT_gc;
  PORT_ADC.COMMAND |= 1;
  while (!(PORT_ADC.INTFLAGS & ADC_RESRDY_bm));
  result = PORT_ADC.RES;

  PORT_BUTTON.DIRCLR = PIN_BUTTON; // set input with pullup to charge touch
  PORT_BUTTON.BUTTON_CTRL |= PORT_PULLUPEN_bm;
  _delay_us(10);
  PORT_BUTTON.BUTTON_CTRL &= ~PORT_PULLUPEN_bm; // disable pullup

  // enable adc, equalize s/h cap charge with touch charge
  PORT_ADC.MUXPOS =  ADC_BUTTON_gc;
  PORT_ADC.CTRLA = (1<<ADC_ENABLE_bp) | (0<<ADC_FREERUN_bp) | ADC_RESSEL_10BIT_gc;
  PORT_ADC.COMMAND |= 1;
  while (!(PORT_ADC.INTFLAGS & ADC_RESRDY_bm));
  result = PORT_ADC.RES;

  PORT_ADC.CTRLA = 0; // disable adc

  return result;
}

void touch_init(void) {
  PORT_ADC.CTRLC = ADC_PRESC_DIV64_gc | ADC_REFSEL_VDDREF_gc | (0<<ADC_SAMPCAP_bp);
}


int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  DL("Hello.");

  led_setup();
  led_flash('g', 3);

  touch_init();

  uint16_t avg = 0;
  for(uint8_t i=0; i<10; i++) {
    if (i>4) avg += touch();
  }
  avg /= 5;
  DF("avg: %u", avg);

  uint16_t threshold_upper = avg + 30;
  uint16_t threshold_lower = avg + 15;
  uint16_t v;
  uint8_t touch_clr = 1;

  while(1) {
    v = touch();
    if (v>threshold_upper && touch_clr) {
      touch_clr = 0;
      led_on('g');
      _delay_ms(1000);
      led_off('g');
    }

    if (v<threshold_lower) touch_clr = 1;

    // DF("value: %u", v);
    // _delay_ms(200);
  }
}
