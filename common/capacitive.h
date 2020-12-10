#ifndef __CAPACITIVE_H__
#define __CAPACITIVE_H__


#include "pins.h"

class CAPACITIVE {
  public:
    CAPACITIVE(pin_t *pin0, pin_t *pin2);
    uint16_t get_humidity(uint8_t relative = 0);
    uint8_t to_relative(uint16_t value);

  private:
    pin_t *pin0;
    pin_t *pin2;
};

#endif
