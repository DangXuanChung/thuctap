#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo ADC cho LDR (IO4, ADC1 CH3, 11 dB)
void ldr_init(void);

// Đọc một lần (mV) & đọc kèm lọc EMA (mượt)
int  ldr_read_mv(void);
int  ldr_read_mv_filtered(void);

// Quy đổi về % sáng (0..100). Có sẵn mapping gamma.
int  ldr_percent_from_mv(int mv);
int  ldr_read_percent(void);

#ifdef __cplusplus
}
#endif
