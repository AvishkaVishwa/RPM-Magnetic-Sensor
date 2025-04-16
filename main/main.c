#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "include/rpm_sensor.h"

#define TAG "MAIN"

void app_main(void)
{
    // Initialize sensor on the default GPIO pin
    esp_err_t result = rpm_sensor_init(HALL_SENSOR_PIN);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RPM sensor");
        return;
    }
    
    // Start RPM measurement
    result = rpm_sensor_start();
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start RPM measurement");
        return;
    }
    
    // Main app can continue with other tasks
    // For example, setting up WiFi, display, etc.
}