// m5_lcd.h
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

// ==== GPIO mapping ====
#define LCD_PIN_SCLK   10   // IO10
#define LCD_PIN_MOSI    9   // IO9
#define LCD_PIN_CS     11   // IO11
#define LCD_PIN_DC      8   // IO8 (AO)
#define LCD_PIN_RST    18   // IO18
#define LCD_PIN_BL     17   // IO17

#define LCD_SPI_HOST   SPI2_HOST

// ==== Kích thước & offset panel ====
#define LCD_WIDTH     128
#define LCD_HEIGHT    160
#define LCD_XSTART      0
#define LCD_YSTART      0

// ==== Tùy chọn hiển thị ====
#define LCD_INVON        1      // 1: inversion ON (thường đúng với ST7735S)
#define LCD_MADCTL_VAL   0x68   // BGR, không xoay
#define LCD_MIRROR_X     0      // 1: sửa lỗi soi gương trái–phải bằng phần mềm

// ==== Màu RGB565 ====
#define C_RGB(r,g,b)  (((r&0xF8)<<8)|((g&0xFC)<<3)|((b)>>3))
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_RED     0xF800
#define C_GREEN   0x07E0
#define C_BLUE    0x001F
#define C_CYAN    0x07FF
#define C_MAGENTA 0xF81F
#define C_YELLOW  0xFFE0

// API
esp_err_t m5_lcd_init(void);
void      m5_lcd_set_backlight(bool on);
void      m5_lcd_fill(uint16_t color);
void      m5_lcd_fill_rect(int x, int y, int w, int h, uint16_t color);
void      m5_lcd_draw_string(int x, int y, const char *text,
                             uint16_t fg, uint16_t bg, int size12, bool transparent);

