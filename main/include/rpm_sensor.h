#ifndef RPM_SENSOR_H
#define RPM_SENSOR_H

#include <stdio.h>
#include "driver/gpio.h"
#include "esp_err.h"

#define HALL_SENSOR_PIN    GPIO_NUM_4   // Connect your HW-484 to GPIO4 (change if needed)
#define DEBOUNCE_TIME      10           // Debounce time in milliseconds
#define RPM_CALC_INTERVAL  1000         // Calculate RPM every 1000ms

/**
 * @brief Initialize the RPM sensor
 * 
 * @param gpio_pin GPIO pin number where the hall sensor is connected
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rpm_sensor_init(gpio_num_t gpio_pin);

/**
 * @brief Get the current RPM value
 * 
 * @return float Current RPM value
 */
float rpm_sensor_get_rpm(void);

/**
 * @brief Start the RPM measurement task
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rpm_sensor_start(void);

#endif /* RPM_SENSOR_H */