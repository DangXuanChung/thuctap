#include "sdkconfig.h"
#include <math.h>
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG 
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "ldr.h"
#include "sdkconfig.h"
#if !defined(LOG_LOCAL_LEVEL) && defined(CONFIG_LOG_MAXIMUM_LEVEL)
#define LOG_LOCAL_LEVEL CONFIG_LOG_MAXIMUM_LEVEL
#endif
// ==== PHẦN CỨNG (theo schematic bạn gửi) ====
#define LDR_GPIO     4                   // IO4
#define LDR_UNIT     ADC_UNIT_1
#define LDR_CHANNEL  ADC_CHANNEL_3       // ESP32-S3: IO4 = ADC1_CH3
#define LDR_ATTEN    ADC_ATTEN_DB_11     // đọc đến ~3.3V
#define LDR_BITW     ADC_BITWIDTH_12
#define VREF_MV      3300

static const char *TAG = "LDR";

static adc_oneshot_unit_handle_t s_adc = NULL;
static adc_cali_handle_t         s_cali = NULL;
static float s_ema = -1.0f;              // bộ lọc mượt EMA

void ldr_init(void)
{
    // ADC oneshot unit
    adc_oneshot_unit_init_cfg_t unit_cfg = {.unit_id = LDR_UNIT};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));

    // Kênh
    adc_oneshot_chan_cfg_t ch_cfg = {.bitwidth = LDR_BITW, .atten = LDR_ATTEN};
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, LDR_CHANNEL, &ch_cfg));

    // Calibration (nếu eFuse/IDF hỗ trợ)
    adc_cali_curve_fitting_config_t cal_cfg = {
        .unit_id = LDR_UNIT, .atten = LDR_ATTEN, .bitwidth = LDR_BITW
    };
    if (adc_cali_create_scheme_curve_fitting(&cal_cfg, &s_cali) == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration: curve fitting");
    } else {
        s_cali = NULL;
        ESP_LOGW(TAG, "ADC calibration not available, using Vref=%dmV", VREF_MV);
    }
}

int ldr_read_mv(void)
{
    int raw = 0, mv = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc, LDR_CHANNEL, &raw));
    if (s_cali) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(s_cali, raw, &mv));
    } else {
        mv = (raw * VREF_MV) / 4095;
    }
    return mv;
}

int ldr_read_mv_filtered(void)
{
    const float alpha = 0.4f;         // 0..1 (cao hơn = phản ứng nhanh hơn)
    int mv = ldr_read_mv();
    if (s_ema < 0) s_ema = (float)mv;
    else           s_ema = (1.0f - alpha) * s_ema + alpha * mv;
    return (int)(s_ema + 0.5f);
}

// Map về % theo cảm nhận mắt người (gamma ≈ 2.2).
// Chỉnh 2 mốc dưới theo thực tế của bạn để % hiển thị "đẹp".
static inline int _clip(int v, int lo, int hi){ if(v<lo) return lo; if(v>hi) return hi; return v; }

int ldr_percent_from_mv(int mv)
{
    const int mv_min = 150;   // điện áp khi tối (đo thực tế rồi chỉnh)
    const int mv_max = 3000;  // điện áp khi rất sáng
    mv = _clip(mv, mv_min, mv_max);
    float x = (float)(mv - mv_min) / (float)(mv_max - mv_min); // 0..1
    float pct = powf(x, 1.0f/2.2f) * 100.0f;                   // gamma
    if (pct < 0) {pct = 0;} 
    if (pct > 100) {pct = 100;}
    return (int)(pct + 0.5f);
}

int ldr_read_percent(void)
{
    int mv = ldr_read_mv_filtered();
    return ldr_percent_from_mv(mv);
}
