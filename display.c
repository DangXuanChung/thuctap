// display.c
#include <stdio.h>
#include "sdkconfig.h"           // để các CONFIG_* có giá trị
#include "freertos/FreeRTOS.h"   // định nghĩa pdMS_TO_TICKS, TickType_t, ...
#include "freertos/task.h"       // vTaskDelay, Task APIs
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG 
#include "esp_log.h"
#include "display.h"
#include "m5_lcd.h"
//static void blink_bl_once(void) {
  //  gpio_config_t io = {
  //      .pin_bit_mask = 1ULL << 17,   // IO17 = LCD_LED_ON theo sơ đồ
   //     .mode = GPIO_MODE_OUTPUT,
  //  };
  //  gpio_config(&io);

 //  // Thử cả 2 mức để xem cái nào làm sáng:
//   gpio_set_level(17, 0); vTaskDelay(pdMS_TO_TICKS(500));
//    gpio_set_level(17, 1); vTaskDelay(pdMS_TO_TICKS(500));
    // Giữ ở mức 1 để quan sát
///    gpio_set_level(17, 1);
//}
void display_init(void) {
    m5_lcd_init();

    // 1) Bật đèn nền
    m5_lcd_set_backlight(true);

    m5_lcd_fill(C_RED);   vTaskDelay(pdMS_TO_TICKS(200));
    m5_lcd_fill(C_GREEN); vTaskDelay(pdMS_TO_TICKS(200));
    m5_lcd_fill(C_BLUE);  vTaskDelay(pdMS_TO_TICKS(200));
    m5_lcd_fill(C_BLACK);
    m5_lcd_draw_string(4, 2, "AHT20 DEMO", C_WHITE, C_BLACK, 1, false);
}

void display_aht20(float t, float h, esp_err_t err) {
    char line[48];
    if (err == ESP_OK) {
        snprintf(line, sizeof line, "Temp: %.1f C", t);
        m5_lcd_draw_string(4, 18, line, C_GREEN, C_BLACK, 1, false);
        snprintf(line, sizeof line, "Humi: %.2f %%", h);
        m5_lcd_draw_string(4, 34, line, C_CYAN,  C_BLACK, 1, false);
        // Xóa dòng lỗi: in khoảng trắng đè
        m5_lcd_draw_string(4, 50, "                    ", C_BLACK, C_BLACK, 1, false);
    } else {
        snprintf(line, sizeof line, "AHT20 ERROR: %s", esp_err_to_name(err));
        m5_lcd_draw_string(4, 50, line, C_RED, C_BLACK, 1, false);
    }
}
void display_light(int mv, int pct)
{
    const int x = 4, y = 50;          // dưới dòng Humi
    const int slot_chars = 10;        // vùng xóa đủ dài
    m5_lcd_fill_rect(x, y, 6*slot_chars, 12, C_BLACK);

    char buf[48];
    snprintf(buf, sizeof buf, "Light: %d mV (%2d%%)", mv, pct);
    m5_lcd_draw_string(x, y, buf, C_YELLOW, C_BLACK, 1, false);
}