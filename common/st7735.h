#ifndef __ST7735_H__
#define __ST7735_H__

#include <avr/pgmspace.h>

#define DELAY_FLAG 0x80
#define _swap_uint8_t(a, b) { uint8_t t = a; a = b; b = t; }

enum ST7735_COMMANDS {
  ST7735_NOP = 0x00,
  ST7735_SWRESET = 0x01,
  ST7735_RDDID = 0x04,
  ST7735_RDDST = 0x09,

  ST7735_SLPIN = 0x10,
  ST7735_SLPOUT = 0x11,
  ST7735_PTLON = 0x12,
  ST7735_NORON = 0x13,

  ST7735_INVOFF = 0x20,
  ST7735_INVON = 0x21,
  ST7735_DISPOFF = 0x28,
  ST7735_DISPON = 0x29,
  ST7735_CASET = 0x2A,
  ST7735_RASET = 0x2B,
  ST7735_RAMWR = 0x2C,
  ST7735_RAMRD = 0x2E,

  ST7735_PTLAR = 0x30,
  ST7735_COLMOD = 0x3A,
  ST7735_MADCTL = 0x36,

  ST7735_FRMCTR1 = 0xB1,
  ST7735_FRMCTR2 = 0xB2,
  ST7735_FRMCTR3 = 0xB3,
  ST7735_INVCTR = 0xB4,
  ST7735_DISSET5 = 0xB6,

  ST7735_PWCTR1 = 0xC0,
  ST7735_PWCTR2 = 0xC1,
  ST7735_PWCTR3 = 0xC2,
  ST7735_PWCTR4 = 0xC3,
  ST7735_PWCTR5 = 0xC4,
  ST7735_VMCTR1 = 0xC5,

  ST7735_RDID1 = 0xDA,
  ST7735_RDID2 = 0xDB,
  ST7735_RDID3 = 0xDC,
  ST7735_RDID4 = 0xDD,

  ST7735_PWCTR6 = 0xFC,

  ST7735_GMCTRP1 = 0xE0,
  ST7735_GMCTRN1 = 0xE1
};

enum ST7735_COLORS {
  ST7735_COLOR_BLACK = 0x0000,
  ST7735_COLOR_BLUE = 0x001F,
  ST7735_COLOR_RED = 0xF800,
  ST7735_COLOR_GREEN = 0x07E0,
  ST7735_COLOR_CYAN = 0x07FF,
  ST7735_COLOR_MAGENTA = 0xF81F,
  ST7735_COLOR_YELLOW = 0xFFE0,
  ST7735_COLOR_WHITE = 0xFFFF
};

typedef enum {
  ST7735_LANDSCAPE,
  ST7735_PORTRAIT,
  ST7735_LANDSCAPE_INV,
  ST7735_PORTRAIT_INV
} ST_Orientation;

enum ST7735_MADCTL_ARGS {
  MADCTL_MY = 0x80,  // mirror Y
  MADCTL_MX = 0x40,  // mirrror x
  MADCTL_MV = 0x20,  // swap XY
  MADCTL_ML = 0x10,  // scan address order
  MADCTL_RGB = 0x00,
  MADCTL_BGR = 0x08,
  MADCTL_MH = 0x04   // horizontal scan oder
};

void st7735_init();
uint8_t st7735_get_width();
uint8_t st7735_get_height();
void st7735_on();
void st7735_off();
void st7735_sleep();
void st7735_wakeup();
void st7735_set_addr_win(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void st7735_cmd(uint8_t cmd);
void st7735_data(uint8_t data);
void st7735_color(uint16_t color);
uint16_t st7735_rgb_color(uint8_t r, uint8_t g, uint8_t b);
void st7735_set_orientation(ST_Orientation orientation);
void st7735_pixel(int16_t x, int16_t y, uint16_t color);
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void st7735_vline(uint8_t x, uint8_t y, uint8_t h, uint16_t color);
void st7735_hline(uint8_t x, uint8_t y, uint8_t w, uint16_t coor);
void st7735_fill_screen(uint16_t color);
void st7735_circle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color);
void st7735_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
void st7735_char(char c, uint8_t x, uint8_t y, uint8_t scale, uint16_t fgcolor, uint16_t bgcolor);
void st7735_text(const char *text, uint8_t length, uint8_t x, uint8_t y, uint8_t scale, uint8_t space, uint16_t fgcolor, uint16_t bgcolor);

#endif
