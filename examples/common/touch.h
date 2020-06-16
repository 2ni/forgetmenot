/*
 * pin configuration settings for forgetmenot
 */

#ifndef __TOUCH_H__
#define __TOUCH_H__


class TOUCH {
  public:
    TOUCH(pin_t *pin);
    TOUCH(pin_t *pin, uint16_t threshold);
    uint16_t get_touch();
    uint8_t is_pressed();
    uint8_t was_pressed();
    uint16_t get_avg();

  private:
    void set_thresholds(uint16_t threshold);
    pin_t *pin;
    uint16_t threshold = 0;
    uint16_t threshold_upper = 0;
    uint16_t threshold_lower = 0;
    uint8_t cleared = 1;
};

#endif
