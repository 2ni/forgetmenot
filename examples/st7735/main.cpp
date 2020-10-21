#include <avr/io.h>
#include <string.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "st7735.h"
#include "spi.h"
#include "sleep.h"

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKOUT_bm); // output clk to PB5

  DINIT();
  DF("\n\033[1;38;5;226;48;5;18m Hello from 0x%06lX \033[0m\n", get_deviceid());

  led_flash(&led_b, 3);

  spi_init();
  st7735_init();
  //st7735_set_orientation(ST7735_LANDSCAPE);
  st7735_fill_rect(0, 0, st7735_get_width(), st7735_get_height(), ST7735_COLOR_BLACK);
  st7735_on();

  // counter clockwise
  uint8_t size = 10;
  st7735_fill_rect(0, 0, size, size, st7735_rgb_color(0, 255, 0));
  st7735_fill_rect(0, st7735_get_height()-size, size, size, ST7735_COLOR_YELLOW);
  st7735_fill_rect(st7735_get_width()-size, st7735_get_height()-size, size, size, ST7735_COLOR_BLUE);
  st7735_fill_rect(st7735_get_width()-size, 0, size, size, ST7735_COLOR_RED);
  st7735_draw_line(0, 0, st7735_get_width(), st7735_get_height(), ST7735_COLOR_WHITE);
  st7735_pixel((st7735_get_width()-size)/2, (st7735_get_height()-size)/2, ST7735_COLOR_YELLOW);
  st7735_circle(20, 20, 20, ST7735_COLOR_MAGENTA);
  st7735_char('a', st7735_get_width(), st7735_get_height()/2-8, 2, ST7735_COLOR_YELLOW, ST7735_COLOR_BLACK);
  st7735_char('b', st7735_get_width()-10, st7735_get_height()/2-8, 2, ST7735_COLOR_YELLOW, ST7735_COLOR_BLACK);
  st7735_text("Hello", 5, st7735_get_width(), size, 2, 2, ST7735_COLOR_YELLOW, ST7735_COLOR_BLACK);

  uint8_t largesize = 2*size;
  uint8_t color = 0;
  int8_t inc = 2;
  while(1) {
    st7735_fill_rect((st7735_get_width()-largesize)/2, (st7735_get_height()-largesize)/2, largesize, largesize, st7735_rgb_color(0, color, 0));
    color += inc;
    if (color == 254 || color == 0) inc = (-1)*inc;
    sleep_ms(1);
  }
}
