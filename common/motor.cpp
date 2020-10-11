#include <avr/io.h>
#include "motor.h"
#include "pins.h"

void motor_forward() {
  set_direction(&out1);
  set_direction(&out2);
  set_output(&out1, 1);
  set_output(&out2, 0);
}

void motor_reverse() {
  set_direction(&out1);
  set_direction(&out2);
  set_output(&out1, 0);
  set_output(&out2, 1);
}
void motor_brake() {
  set_direction(&out1);
  set_direction(&out2);
  set_output(&out1, 1);
  set_output(&out2, 1);
}

void motor_off() {
  set_direction(&out1);
  set_direction(&out2);
  set_output(&out1, 0);
  set_output(&out2, 0);
}
