#include "pins.h"
#include "uart.h"

pin_t pin_touch          = { .port = &PORTC, .pin = 5, .port_adc = &ADC1, .pin_adc = 11 }; // PC5
pin_t pin_moisture       = { .port = &PORTC, .pin = 1, .port_adc = &ADC1, .pin_adc = 7  }; // PC1
pin_t pin_temp_board     = { .port = &PORTC, .pin = 0, .port_adc = &ADC1, .pin_adc = 6  }; // PC0
pin_t pin_temp_moisture  = { .port = &PORTC, .pin = 2, .port_adc = &ADC1, .pin_adc = 8  }; // PC2

pin_t led_g              = { .port = &PORTB, .pin = 6 }; // PB6
pin_t led_b              = { .port = &PORTB, .pin = 5 }; // PB5

pin_t rfm_cs             = { .port = &PORTA, .pin = 4 }; // PA4
pin_t rfm_interrupt      = { .port = &PORTA, .pin = 5 }; // PA5

/*
 * set input / output for a pin
 * ie set_direction(touch); // defines touch as output
 *
 * default: output
 */
void set_direction(pin_t *pin, uint8_t input) {
  if (input) (*pin).port->DIRSET = (1<<(*pin).pin);
  else (*pin).port->DIRCLR = (1<<(*pin).pin);
}

/*
 * set pin value if set as output
 */
void set_output(pin_t *pin, uint8_t value) {
  if (value) (*pin).port->OUTSET = (1<<(*pin).pin);
  else (*pin).port->OUTCLR = (1<<(*pin).pin);
}

/*
 * set pullup or clear it for a pin
 * ie set_pullup(touch); // activates pullup on touch
 *
 * default: set pullup
 */
void set_pullup(pin_t *pin, uint8_t clear) {
  register8_t *pctrl = &(*pin).port->PIN0CTRL;
  pctrl += (*pin).pin;

  if (clear) *pctrl &= ~PORT_PULLUPEN_bm;
  else *pctrl |= PORT_PULLUPEN_bm;
}

/*
 * check if adc on the pin is running
 * ie while(adc_is_running(pin_touch));
 */
uint8_t adc_is_running(pin_t *pin) {
  return !((*pin).port_adc->INTFLAGS & ADC_RESRDY_bm);
}

/*
 * return adc measurement on pin
 * muxpos can be set to ADC_MUXPOS_GND_gc
 * if muxpos not set it takes muxpos from (*pin).pin_adc
 *
 * returns measured value
 */
uint16_t get_adc(pin_t *pin, int8_t muxpos) {
  // if muxpos not defined, use muxpos of pin
  if (muxpos == -1) {
    (*pin).port_adc->MUXPOS = ((ADC_MUXPOS_AIN0_gc + (*pin).pin_adc) << 0);
  } else {
    (*pin).port_adc->MUXPOS = muxpos;
  }
  (*pin).port_adc->CTRLA = (1<<ADC_ENABLE_bp) | (0<<ADC_FREERUN_bp) | ADC_RESSEL_10BIT_gc;
  (*pin).port_adc->COMMAND |= 1;
  while (adc_is_running(pin));

  (*pin).port_adc->CTRLA = 0;  // disable adc

  return (*pin).port_adc->RES;
}

/*
 *  get 24bit device id
 *  can be printed with DF("Hello from 0x%06lX", get_deviceid());
 */
uint32_t get_deviceid() {
  return ((uint32_t)SIGROW.DEVICEID0<<16) + ((uint16_t)SIGROW.DEVICEID1<<8) + (uint8_t)SIGROW.DEVICEID2;
}
