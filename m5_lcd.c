// m5_lcd.c
#include "sdkconfig.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG 
#include "esp_log.h"
#include "m5_lcd.h"
#include "font.h"   // phải có asc2_1206[95][12] (MSB-left 6x12)
#ifndef LCD_MADCTL_VAL
#define LCD_MADCTL_VAL 0x08   // BGR, không xoay (sẽ mirror bằng code)
#endif
#ifndef LCD_MIRROR_X
#define LCD_MIRROR_X   0      // bật mirror X bằng phần mềm
#endif
// Nếu font.h chỉ khai báo extern, bỏ dòng extern dưới.
// extern const unsigned char asc2_1206[95][12];

static const char *TAG = "LCD";
static spi_device_handle_t s_spi;

static inline void putc16be(uint8_t *dst, uint16_t c) { dst[0] = c >> 8; dst[1] = c & 0xFF; }

static inline void lcd_cmd(uint8_t cmd)
{
    spi_transaction_t t = {0};
    t.length = 8;
    t.tx_buffer = &cmd;
    gpio_set_level(LCD_PIN_DC, 0);
    ESP_ERROR_CHECK(spi_device_transmit(s_spi, &t));
}

static inline void lcd_data(const void *data, size_t len)
{
    if (!len) return;
    spi_transaction_t t = {0};
    t.length = len * 8;
    t.tx_buffer = data;
    gpio_set_level(LCD_PIN_DC, 1);
    ESP_ERROR_CHECK(spi_device_transmit(s_spi, &t));
}

static inline void lcd_window(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    // Áp offset trước
    xs += LCD_XSTART; xe += LCD_XSTART;
    ys += LCD_YSTART; ye += LCD_YSTART;

    uint8_t d[4];
    lcd_cmd(0x2A); d[0]=xs>>8; d[1]=xs; d[2]=xe>>8; d[3]=xe; lcd_data(d,4);
    lcd_cmd(0x2B); d[0]=ys>>8; d[1]=ys; d[2]=ye>>8; d[3]=ye; lcd_data(d,4);
    lcd_cmd(0x2C);
}

static void st7735s_init(void)
{
    lcd_cmd(0x11); vTaskDelay(pdMS_TO_TICKS(120));     // Sleep Out

    uint8_t colmod[] = {0x05};                         // 16-bit
    lcd_cmd(0x3A); lcd_data(colmod, 1);

    uint8_t madctl[] = { LCD_MADCTL_VAL };             // BGR, không xoay
    lcd_cmd(0x36); lcd_data(madctl, 1);

#if LCD_INVON
    lcd_cmd(0x21);                                     // Inversion ON
#else
    lcd_cmd(0x20);                                     // Inversion OFF
#endif

    lcd_cmd(0x13); vTaskDelay(pdMS_TO_TICKS(10));      // Normal display
    lcd_cmd(0x29); vTaskDelay(pdMS_TO_TICKS(20));      // Display ON
}

esp_err_t m5_lcd_init(void)
{
    ESP_LOGI(TAG, "pins SCLK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d",
             LCD_PIN_SCLK, LCD_PIN_MOSI, LCD_PIN_CS, LCD_PIN_DC, LCD_PIN_RST, LCD_PIN_BL);

    // Configure control pins
    int pins[] = { LCD_PIN_DC, LCD_PIN_RST, LCD_PIN_CS, LCD_PIN_BL };
    for (int i = 0; i < 4; ++i) {
        if (pins[i] < 0) continue;
        gpio_config_t io = { .pin_bit_mask = 1ULL << pins[i], .mode = GPIO_MODE_OUTPUT };
        ESP_ERROR_CHECK(gpio_config(&io));
    }

    // Backlight ON sớm để dễ quan sát
    m5_lcd_set_backlight(true);

    // SPI bus
    spi_bus_config_t bus = {
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 12*1000*1000,   // 10–20 MHz lành
        .mode = 0,
        .spics_io_num = LCD_PIN_CS,
        .queue_size = 7
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_SPI_HOST, &dev, &s_spi));

    // Reset
    if (LCD_PIN_RST >= 0) {
        gpio_set_level(LCD_PIN_RST, 0); vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(LCD_PIN_RST, 1); vTaskDelay(pdMS_TO_TICKS(120));
    }

    st7735s_init();

    m5_lcd_fill(C_BLACK);
    return ESP_OK;
}

void m5_lcd_set_backlight(bool on)
{
    if (LCD_PIN_BL < 0) return;
    gpio_set_level(LCD_PIN_BL, on ? 1 : 0); // mạch NPN low-side → 1 là sáng
}

void m5_lcd_fill(uint16_t color)
{
    lcd_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    uint8_t buf[64*2];
    for (int i = 0; i < 64; ++i) putc16be(&buf[i*2], color);

    int total = LCD_WIDTH * LCD_HEIGHT;
    while (total > 0) {
        int n = (total > 64) ? 64 : total;
        lcd_data(buf, n * 2);
        total -= n;
    }
}

void m5_lcd_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;
    lcd_window(x, y, x + w - 1, y + h - 1);

    uint8_t buf[64*2];
    for (int i = 0; i < 64; ++i) putc16be(&buf[i*2], color);

    int total = w * h;
    while (total > 0) {
        int n = (total > 64) ? 64 : total;
        lcd_data(buf, n * 2);
        total -= n;
    }
}

// Vẽ 1 ký tự 6x12 (MSB-left) với màu 16-bit big-endian
static void draw_char_6x12(int x, int y, char ch, uint16_t fg, uint16_t bg, bool transparent)
{
    //if (ch < 32 || ch > 126) ch = '?';
    //const unsigned char *glyph = asc2_1206[(int)ch - 32];

    // Cửa sổ 6x12 tại (x,y) theo toạ độ logic
    //lcd_window(x, y, x + 5, y + 11);

    //uint8_t line[6*2];
    //for (int row = 0; row < 12; ++row) {
     //   int rr = 11 - row;  
     //   uint8_t bits = glyph[rr];                // 6 bit cao là 6 cột từ trái→phải
        
     //   for (int col = 0; col < 6; ++col) {
     //      bool on = (bits >> (5 - col)) & 1;    // MSB-left
     //       int cc = 5 - col;  
     //       uint16_t c = on ? fg : (transparent ? 0 : bg);
     //       putc16be(&line[col*2], c);            // BE
      //  }
     //   lcd_data(line, sizeof line);
   // }
   if (ch<32||ch>126) ch='?';
    const unsigned char *glyph = asc2_1206[(int)ch-32];

    lcd_window(x, y, x+5, y+11);

    uint8_t line[6*2];
    for (int row=0; row<12; ++row) {
       // int rr = 11 - row;                 // LẬT DỌC (trên↔dưới)
      //  uint8_t bits = glyph[rr];          // font MSB-left (6 bit cao)
      uint8_t bits = glyph[row];
        
        for (int col=0; col<6; ++col) {
            bool on = (bits >> (5 - col)) & 1;   // đọc trái→phải theo font
            int cc = 5 - col;                    // LẬT NGANG (trái↔phải)
            uint16_t c = on ? fg : (transparent ? 0 : bg);
            // gửi màu big-endian
            line[cc*2 + 0] = c >> 8;
            line[cc*2 + 1] = c & 0xFF;
        }
        lcd_data(line, sizeof line);
    }
}

void m5_lcd_draw_string(int x, int y, const char *text, uint16_t fg, uint16_t bg, int size12, bool transparent){
    if (size12 < 1) size12 = 1;
    int ox = x, cx = x, cy = y;

    for (const char *p = text; *p; ++p) {
        if (*p == '\n') { cx = ox; cy += 12*size12; continue; }
        // hiện tại chỉ hỗ trợ scale = 1 (6x12)
        draw_char_6x12(cx, cy, *p, fg, bg, transparent);
        cx += 6*size12;
    }
}
