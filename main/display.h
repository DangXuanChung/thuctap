// display.h
#pragma once
#include "esp_err.h"

void display_init(void);
void display_aht20(float t_c, float h_pct, esp_err_t last_err);
void display_light(int mv, int pct);