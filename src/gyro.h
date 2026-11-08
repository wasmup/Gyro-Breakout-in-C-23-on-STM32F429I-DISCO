#pragma once
#include <stdint.h>
/* Calibrate while stationary; rates are filtered integer degrees per second. */
bool gyro_start(void);
bool gyro_rates(int rates[3]);
/* Select a moving-average window from 1 to 10 samples; clears its history. */
void gyro_set_filter_window(unsigned samples);

/* Sensor readings used to vary non-security game choices. */
uint32_t gyro_calibration_seed(void);
