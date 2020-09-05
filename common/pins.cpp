#include "pins.h"
#include "uart.h"

pin_t pin_touch          = { .port = &PORTC, .pin = 5, .port_adc = &ADC1, .pin_adc = 11 }; // PC5
pin_t pin_moisture       = { .port = &PORTC, .pin = 1, .port_adc = &ADC1, .pin_adc = 7  }; // PC1
pin_t pin_temp_board     = { .port = &PORTC, .pin = 0, .port_adc = &ADC1, .pin_adc = 6  }; // PC0
pin_t pin_temp_moisture  = { .port = &PORTC, .pin = 2, .port_adc = &ADC1, .pin_adc = 8  }; // PC2

pin_t out1               = { .port = &PORTB, .pin = 7 }; // PB7
pin_t out2               = { .port = &PORTA, .pin = 7 }; // PA7

pin_t led_g              = { .port = &PORTB, .pin = 6 }; // PB6
pin_t led_b              = { .port = &PORTB, .pin = 5 }; // PB5

pin_t rfm_cs             = { .port = &PORTA, .pin = 4 }; // PA4
pin_t rfm_interrupt      = { .port = &PORTA, .pin = 5 }; // PA5

pin_t multi              = { .port = &PORTC, .pin = 4, .port_adc = &ADC1, .pin_adc = 10 }; // PC4

/*
 * disable digital input buffer on all io pins
 * to save power
 */
void disable_buffer_of_pins() {
  for (uint8_t pin = 0; pin < 8; pin++) {
    (&PORTA.PIN0CTRL)[pin] = PORT_ISC_INPUT_DISABLE_gc;
    (&PORTB.PIN0CTRL)[pin] = PORT_ISC_INPUT_DISABLE_gc;
    (&PORTC.PIN0CTRL)[pin] = PORT_ISC_INPUT_DISABLE_gc;
  }
}

/*
 * set all pins as input and low
 *
 * this is somehow needed to get low power while sleeping
 */
void clear_all_pins() {
  for (uint8_t i = 0; i < 8; i++) {
    PORTA.DIRSET = (1<<i);
    PORTA.OUTCLR = (1<<i);
  }

  for (uint8_t i = 0; i < 8; i++) {
    PORTB.DIRSET = (1<<i);
    PORTB.OUTCLR = (1<<i);
  }

  for (uint8_t i = 0; i < 6; i++) {
    PORTC.DIRSET = (1<<i);
    PORTC.OUTCLR = (1<<i);
  }
}
/*
 * set input / output for a pin
 * ie set_direction(touch); // defines touch as output
 * direction = 1 -> output (default)
 * direction = 0 -> input
 *
 * default: output
 */
void set_direction(pin_t *pin, uint8_t direction) {
  if (direction) (*pin).port->DIRSET = (1<<(*pin).pin);
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
 * toggle pin output
 */
void toggle_output(pin_t *pin) {
  (*pin).port->OUTTGL = (1<<(*pin).pin);
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
  // set_direction(pin, 0); // set as input

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

  // save power
  // set_direction(pin, 1);
  // set_output(pin, 0);

  return (*pin).port_adc->RES;
}

/*
 *  get 24bit device id
 *  can be printed with DF("Hello from 0x%06lX", get_deviceid());
 */
uint32_t get_deviceid() {
  return ((uint32_t)SIGROW.DEVICEID0<<16) | ((uint16_t)SIGROW.DEVICEID1<<8) | (uint8_t)SIGROW.DEVICEID2;
}
