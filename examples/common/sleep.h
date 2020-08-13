/*
 * simple sleep mode
 * code based on https://www.avrfreaks.net/forum/tinyavr-1-series-071ua-rtc-running-how
 *
 * in standby mode periphery can be activated by settings RUNSTDBY in the corresponding periphery
 * ports stay on in sleep mode
 *
 * modes: idle, standby, powerdown
 *
 * RTC needs standby mode
 *
 */
#ifndef __SLEEP_H__
#define __SLEEP_H__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

void sleep_ms(uint32_t ms);

#endif
