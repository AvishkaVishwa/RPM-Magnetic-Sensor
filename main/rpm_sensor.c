#include "include/rpm_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#define TAG "RPM_SENSOR"

// Variables for RPM calculation
static volatile uint32_t pulse_count = 0;
static volatile int64_t last_time = 0;
static float rpm = 0.0;
static gpio_num_t sensor_pin = HALL_SENSOR_PIN;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

// ISR for Hall sensor detection
static void IRAM_ATTR hall_sensor_isr(void* arg)
{
    int64_t now = esp_timer_get_time() / 1000; // Convert to milliseconds
    
    // Simple debouncing - ignore pulses that come too quickly
    if ((now - last_time) > DEBOUNCE_TIME) {
        pulse_count++;
        last_time = now;
    }
}

// Task to calculate and display RPM
static void rpm_task(void *pvParameters)
{
    uint32_t last_pulse_count = 0;
    uint32_t pulses;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(RPM_CALC_INTERVAL));
        
        // Critical section to safely read pulse_count
        portENTER_CRITICAL(&spinlock);
        pulses = pulse_count - last_pulse_count;
        last_pulse_count = pulse_count;
        portEXIT_CRITICAL(&spinlock);
        
        // Calculate RPM - multiply by 60 (seconds per minute)
        // Divide by the number of magnets if you have multiple magnets on your wheel
        rpm = (pulses * 60.0 * 1000) / RPM_CALC_INTERVAL;
        
        ESP_LOGI(TAG, "Pulses: %lu, RPM: %.2f", pulses, rpm);
    }
}

esp_err_t rpm_sensor_init(gpio_num_t gpio_pin)
{
    // Store GPIO pin
    sensor_pin = gpio_pin;
    
    // Initialize Hall sensor GPIO as input
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << sensor_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,       // Enable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,         // Interrupt on falling edge
    };
    
    esp_err_t result = gpio_config(&io_conf);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO");
        return result;
    }
    
    // Install GPIO ISR service
    result = gpio_install_isr_service(0);
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install ISR service");
        return result;
    }
    
    // Add handler for Hall sensor GPIO
    result = gpio_isr_handler_add(sensor_pin, hall_sensor_isr, NULL);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler");
        return result;
    }
    
    ESP_LOGI(TAG, "RPM Sensor initialized on GPIO %d", sensor_pin);
    return ESP_OK;
}

float rpm_sensor_get_rpm(void)
{
    return rpm;
}

esp_err_t rpm_sensor_start(void)
{
    ESP_LOGI(TAG, "Starting RPM measurement");
    ESP_LOGI(TAG, "Waiting for pulses from Hall sensor...");
    
    // Create task to calculate and display RPM
    BaseType_t task_created = xTaskCreate(
        rpm_task,          // Function
        "rpm_task",        // Name
        2048,              // Stack size
        NULL,              // Parameters
        10,                // Priority
        NULL               // Task handle
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RPM task");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}