/*
 * ST7735 1.8" 128x160 display
 *
 * based on
 *   - https://github.com/LongHairedHacker/avr-st7735
 *   - https://github.com/adafruit/Adafruit-ST7735-Library
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "stdlib.h"
#include "pins.h"
#include "spi.h"
// #include "sleep.h"
#include "st7735.h"
#include "uart.h"

uint8_t st7735_default_width = 128;
uint8_t st7735_width = st7735_default_width;
uint8_t st7735_default_height = 160;
uint8_t st7735_height = st7735_default_height;
uint8_t st7735_column_start = 0;
uint8_t st7735_row_start = 0;

/*
 * spi mode 0
 * default orientation: portrait inv
 *     y
 *     ^
 *     | 160
 * x<--┘
 *  128
 *
 */
void st7735_init() {
  uint8_t rotate=0;

  // define outputs
  set_direction(&lcd_rst, 1);
  set_direction(&lcd_cs, 1);
  set_direction(&lcd_dc, 1); // also called rs
  set_direction(&lcd_blk, 1);

  set_output(&lcd_rst, 1);
  // sleep_ms(50);

  st7735_cmd(ST7735_SWRESET); // software reset
  //sleep_ms(150);
  st7735_cmd(ST7735_SLPOUT);  // out of sleep mode
  st7735_cmd(ST7735_COLMOD);  // color mode 16bit
  st7735_data(0x05);
  st7735_cmd(ST7735_INVOFF);  // invert
  st7735_cmd(ST7735_MADCTL);
  st7735_data(rotate<<5);     // MADCTL
  st7735_cmd(ST7735_DISPON);  // display on
}

uint8_t st7735_get_width() {
  return st7735_width;
}

uint8_t st7735_get_height() {
  return st7735_height;
}

void st7735_on() {
  set_output(&lcd_blk, 1);
}

void st7735_off() {
  set_output(&lcd_blk, 0);
}

void st7735_sleep() {
  st7735_cmd(ST7735_SLPIN);
  st7735_off();
}

void st7735_wakeup() {
  st7735_cmd(ST7735_SLPOUT);
  st7735_on();
}

void st7735_set_addr_win(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  st7735_cmd(ST7735_CASET); // Column addr set
  st7735_data(0x00);
  st7735_data(x0 + st7735_column_start);	// XSTART
  st7735_data(0x00);
  st7735_data(x1 + st7735_column_start); // XEND

  st7735_cmd(ST7735_RASET); // Row addr set
  st7735_data(0x00);
  st7735_data(y0 + st7735_row_start); // YSTART
  st7735_data(0x00);
  st7735_data(y1 + st7735_row_start); // YEND

  st7735_cmd(ST7735_RAMWR); // write to RAM
}


void st7735_cmd(uint8_t cmd) {
  // DC = 0, CS = 0
  set_output(&lcd_dc, 0);

  set_output(&lcd_cs, 0);
  spi_transfer_byte(cmd);
  set_output(&lcd_cs, 1);
}

void st7735_data(uint8_t data) {
  // DC = 1, CS = 0
  set_output(&lcd_dc, 1);

  set_output(&lcd_cs, 0);
  spi_transfer_byte(data);
  set_output(&lcd_cs, 1);

}

void st7735_color(uint16_t color) {
  spi_transfer_byte(color >> 8);
  spi_transfer_byte(color);
}

uint16_t st7735_rgb_color(uint8_t r, uint8_t g, uint8_t b) {
  return (r & 0xf8)<<8 | (g & 0xfc)<<3 | b>>3;
}

void st7735_set_orientation(ST_Orientation orientation) {
  st7735_cmd(ST7735_MADCTL);

  switch (orientation) {
    case ST7735_PORTRAIT:
      st7735_data(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
      st7735_width = st7735_default_width;
      st7735_height = st7735_default_height;
      break;
    case ST7735_LANDSCAPE:
      st7735_data(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
      st7735_width = st7735_default_height;
      st7735_height = st7735_default_width;
      break;
    case ST7735_PORTRAIT_INV:
      st7735_data(MADCTL_RGB);
      st7735_width  = st7735_default_width;
      st7735_height = st7735_default_height;
      break;
    case ST7735_LANDSCAPE_INV:
      st7735_data(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
      st7735_width = st7735_default_height;
      st7735_height = st7735_default_width;
      break;
  }
}

void st7735_pixel(int16_t x, int16_t y, uint16_t color) {
  if(x < 0 || x >= st7735_width || y < 0 || y >= st7735_height) {
    return;
  }

  st7735_set_addr_win(x, y, x+1, y+1);

  set_output(&lcd_dc, 1);
  set_output(&lcd_cs, 0);
  st7735_color(color);
  set_output(&lcd_cs, 1);
}

void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
  if (x >= st7735_width || y >= st7735_height) {
    return;
  }

  if ((x + w - 1) >= st7735_width) {
    w = st7735_width  - x;
  }

  if ((y + h - 1) >= st7735_height) {
    h = st7735_height - y;
  }

  st7735_set_addr_win(x, y, x + w - 1, y + h - 1);

  set_output(&lcd_dc, 1);
  set_output(&lcd_cs, 0);

  for (uint8_t i = 0; i < h; i++) {
    for (uint8_t j = 0; j < w; j++) {
      st7735_color(color);
    }
  }

  set_output(&lcd_cs, 1);
}

void st7735_vline(uint8_t x, uint8_t y, uint8_t h, uint16_t color) {
  st7735_fill_rect(x, y, 1, h, color);
}

void st7735_hline(uint8_t x, uint8_t y, uint8_t w, uint16_t color) {
  st7735_fill_rect(x, y, w, 1, color);
}

void st7735_fill_screen(uint16_t color) {
  st7735_fill_rect(0, 0, st7735_width, st7735_height, color);
}

void st7735_circle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  st7735_pixel(x0, y0 + r, color);
  st7735_pixel(x0, y0 - r, color);
  st7735_pixel(x0 + r, y0, color);
  st7735_pixel(x0 - r, y0, color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    st7735_pixel(x0 + x, y0 + y, color);
    st7735_pixel(x0 - x, y0 + y, color);
    st7735_pixel(x0 + x, y0 - y, color);
    st7735_pixel(x0 - x, y0 - y, color);
    st7735_pixel(x0 + y, y0 + x, color);
    st7735_pixel(x0 - y, y0 + x, color);
    st7735_pixel(x0 + y, y0 - x, color);
    st7735_pixel(x0 - y, y0 - x, color);
  }
}

void st7735_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
  uint8_t steep_dir = abs(y1 - y0) > abs(x1 - x0);
  if (steep_dir) {
    _swap_uint8_t(x0, y0);
    _swap_uint8_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_uint8_t(x0, x1);
    _swap_uint8_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t y_step;

  if (y0 < y1) {
    y_step = 1;
  } else {
    y_step = -1;
  }

  uint8_t seg = x0;
  uint8_t cur_x;
  for(cur_x = x0; cur_x <= x1; cur_x++) {
    err -= dy;
    if (err < 0) {
      if (steep_dir) {
        st7735_vline(y0, seg, cur_x - seg + 1, color);
      } else {
        st7735_hline(seg, y0, cur_x - seg +1, color);
      }
      y0 += y_step;
      err += dx;
      seg = cur_x + 1;
    }
  }

  // x0 incremented
  if (steep_dir) {
    st7735_vline(y0, seg, cur_x - seg, color);
  } else {
    st7735_hline(seg, y0, cur_x - seg, color);
  }
}

/*
 * ASCII char 5x8 at x, y with bottom left corner
 * no space after char
 */
void st7735_char(char c, uint8_t x, uint8_t y, uint8_t scale, uint16_t fgcolor, uint16_t bgcolor) {
  uint16_t color = 0;

  st7735_set_addr_win(x-5*scale, y, x-1, y+8*scale-1);
  // st7735_set_addr_win(x-4, y, x+5*scale-1, y+8*scale-1);

  set_output(&lcd_dc, 1);
  set_output(&lcd_cs, 0);
  for (int8_t h=7; h>=0; h--) {
    for (uint16_t ii=0; ii<scale; ii++) {
      for (int8_t w=4; w>=0; w--) {
        uint8_t bits = pgm_read_byte(&char_map[c-32][w]);
        color = ((bits>>h) & 0x01) ? fgcolor : bgcolor;
        for (uint8_t i=0; i<scale; i++) {
          st7735_color(color);
        }
      }
    }
  }
  set_output(&lcd_cs, 1);
}

/*
 * text from bottom left corner
 */
void st7735_text(const char *text, uint8_t length, uint8_t x, uint8_t y, uint8_t scale, uint8_t space, uint16_t fgcolor, uint16_t bgcolor) {
  for (uint8_t i=0; i<length; i++) {
    st7735_char(text[i], x, y, scale, fgcolor, bgcolor);
    x -= 5*scale+space; // adds space between chars
  }
}

