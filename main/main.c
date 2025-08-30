#include <stdio.h>
#include "sdkconfig.h" 
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <aht.h>
#include <string.h>
#include <esp_err.h>
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG 
#include <esp_log.h>
#include "display.h"
#include "ldr.h"
#include "esp_mac.h"
#define AHT_TYPE AHT_TYPE_AHT20


#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

static const char *TAG = "aht-example";

void task(void *pvParameters)
{
    aht_t dev = { 0 };
    dev.mode = AHT_MODE_NORMAL;
    dev.type = AHT_TYPE;

    ESP_ERROR_CHECK(aht_init_desc(&dev, 0x38, 0, 37, 36));
    ESP_ERROR_CHECK(aht_init(&dev));

    bool calibrated;
    ESP_ERROR_CHECK(aht_get_status(&dev, NULL, &calibrated));
    if (calibrated)
        ESP_LOGI(TAG, "Sensor calibrated");
    else
        ESP_LOGW(TAG, "Sensor not calibrated!");

    float temperature, humidity;

    while (1)
    {
        esp_err_t res = aht_get_data(&dev, &temperature, &humidity);
        if (res == ESP_OK)
            ESP_LOGI(TAG, "Temperature: %.1f°C, Humidity: %.2f%%", temperature, humidity);
        else
            ESP_LOGE(TAG, "Error reading data: %d (%s)", res, esp_err_to_name(res));
        display_aht20(temperature, humidity, res);   // NEW: đẩy dữ liệu lên LCD
        int mv  = ldr_read_mv_filtered();
        int pct = ldr_percent_from_mv(mv);
        display_light(mv, pct);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main()
{
    ESP_LOGI("APP", "app_main start");

   // blink_bl_once(); // test BL sớm
    ESP_ERROR_CHECK(i2cdev_init());

    ESP_LOGI("APP", "calling display_init");
    display_init();
    ldr_init();  
    ESP_LOGI("APP", "after display_init");

    xTaskCreatePinnedToCore(task, TAG,
        configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);

    ESP_LOGI("APP", "leaving app_main");  
}

