#include "sensors.h"

/*
 * values according to sheet https://docs.google.com/spreadsheets/d/1KOPVIqWLB8RtdWV2ETXrc3K58mqvxZQhDzQeA-0KVI4/edit#gid=679079026
 * values are configured for Vref=1.5v and Vcc=3.1v
 * for other references the values can be calculated with Vref/Vrefnew eg (*15/11)
 *                Vref    * Vccnew
 * adc(new) = adc ----------------
 *                Vrefnew * Vcc
 * (the lower Vref the higher the adc values will be)
 * (the higher Vcc the higher the adc values will be)
 */
const uint8_t temp_characteristics_size = 13;
const temp_characteristics_struct temp_characteristics[] = {
  {-150, 980},
  {-100, 823},
  {-50,  680},
  {0,    557},
  {50,   452},
  {100,  366},
  {150,  295},
  {200,  238},
  {250,  192},
  {300,  156},
  {350,  127},
  {400,  103},
  {450,  85}
};


/*
 * measure temperature with sensors on the board or humidity sensor
 *
 * device: 'b' for board  (PC0, ADC1 AIN6, TEMPBOARD=TEMP1)
 *         's' for sensor (PC2, ADC1 AIN8, TEMPSENSOR=TEMP2)
 */
uint16_t get_temp_board() {
  return get_temp(&pin_temp_board);
}

uint16_t get_temp_moist() {
  return get_temp(&pin_temp_moisture);
}

uint16_t get_temp(pin_t *pin) {
  VREF.CTRLC |= VREF_ADC1REFSEL_1V5_gc;
  (*pin).port_adc->CTRLC = ADC_PRESC_DIV128_gc | ADC_REFSEL_INTREF_gc | (0<<ADC_SAMPCAP_bp);

  // get average
  uint32_t value = 0;
  uint8_t samples = 1;
  for (uint8_t i=0; i<samples; i++) {
    uint16_t v = get_adc(pin);
    // DF("  v: %u\n", v);
    value += v;
  }

  return adc2temp(value/samples);

  /*
  uint8_t MUXPOS = ADC_MUXPOS_AIN6_gc;
  if (device == 's') {
    MUXPOS = ADC_MUXPOS_AIN8_gc;
  }

  ADC1.MUXPOS = MUXPOS;
  ADC1.CTRLC = ADC_PRESC_DIV64_gc | ADC_REFSEL_INTREF_gc | (0<<ADC_SAMPCAP_bp);
  ADC1.CTRLA = (1<<ADC_ENABLE_bp) | (0<<ADC_FREERUN_bp) | ADC_RESSEL_10BIT_gc;

  VREF.CTRLC |= VREF_ADC1REFSEL_1V5_gc;

  ADC1.COMMAND |= 1;
  while (!(ADC1.INTFLAGS & ADC_RESRDY_bm));

  ADC1.CTRLA = 0;                       // disable adc
  return adc2temp(ADC1.RES);
  */
}

/*
 * measure temperature from chip
 * works only with ADC0
 */
uint16_t get_temp_chip() {
  VREF.CTRLA = VREF_ADC0REFSEL_1V1_gc;
  ADC0.CTRLC = ADC_PRESC_DIV256_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm;
  ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
  ADC0.CTRLA = ADC_ENABLE_bm;
  ADC0.COMMAND |= 1;
  while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));

  // calculate temperature in °C
  int8_t sigrow_offset = SIGROW.TEMPSENSE1;
  uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
  int32_t tmp = ADC0.RES - sigrow_offset;
  tmp *= sigrow_gain;
  tmp += 0x80;
  tmp >>= 8;
  tmp -= 273; // °K to °C
  return ((int16_t) tmp);
}

/*
 * convert adc value to temperature according to the
 * characteristics of the NTC resistor
 * see https://docs.google.com/spreadsheets/d/1KOPVIqWLB8RtdWV2ETXrc3K58mqvxZQhDzQeA-0KVI4
 *
 * @return temperature 1/10°C precision as integer
 */
int16_t adc2temp(uint16_t adc) {
  return interpolate(adc, temp_characteristics, temp_characteristics_size);
}

/*
 * interpolate a value according to a characterstic table
 */
int16_t interpolate(uint16_t adc, const temp_characteristics_struct *characteristics, uint8_t size) {
  uint8_t match = size-1;
  for (uint8_t i=0; i<size; i++) {
    if (adc > characteristics[i].adc) {
      match=i;
      break;
    }
  }

  //  adc outside of characteristics, return min or max
  if (match==0) return characteristics[0].temp;
  else if (match==size-1) return characteristics[size-1].temp;

  // linear interpolation
  uint16_t temp_start = characteristics[match-1].temp;
  uint16_t temp_end = characteristics[match].temp;
  int16_t adc_start = characteristics[match-1].adc;
  int16_t adc_end = characteristics[match].adc;

  int16_t r = (temp_end-temp_start) * (adc-adc_start);
  int16_t d = adc_end-adc_start;
  r /= d;

  return temp_start+r;
}
/*
 * get current voltage of power, aka battery
 * used resistor divider 1M - 220k, 1.5v internale reference
 * returns precision 1/100 volt, eg 495 -> 4.95v
 * max vbat: 8.3v (1.5v ref)
 *           6.1v (1.1v ref)
 *
 * for some strange reasons I get wrong values if presclaer is lower than 128
 */
uint32_t get_vcc_battery() {
  multi.port_adc->CTRLC = ADC_PRESC_DIV128_gc | ADC_REFSEL_INTREF_gc | (0<<ADC_SAMPCAP_bp);
  VREF.CTRLC |= VREF_ADC1REFSEL_1V1_gc;

  uint16_t adc = get_adc(&multi);
  // (vref * (r1+r2) * precision * adc) / (r2 * adc_precision)
  // (1.5*1220*100*adc) / (220*1024)
  return (134200*adc)/225280; // 1.1v reference
  // return (183000*adc)/225280; // 1.5v reference
}

/*
 * measures vcc of chip
 * returns 1/100 volt precision as int
 * eg 316 -> 3.16v
 */
uint16_t get_vcc_cpu() {
  ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;   // set pin to int ref

  ADC0.CTRLC = ADC_PRESC_DIV128_gc       // 10MHz with prescaler 128 -> 78kHz
    | ADC_REFSEL_VDDREF_gc              // VDD
    | (0<<ADC_SAMPCAP_bp);              // disable sample capacitance

  ADC0.CTRLA = (1<<ADC_ENABLE_bp)       // enable adc
    | (0<<ADC_FREERUN_bp)               // no free run
    | ADC_RESSEL_10BIT_gc;              // set resolution to 10bit

  VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc; // 1.1v reference

  ADC0.COMMAND |= 1;
  while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));

  ADC0.CTRLA = 0;                       // disable adc
  return (112640 / ADC0.RES);           // 1024*1.1*100
}
