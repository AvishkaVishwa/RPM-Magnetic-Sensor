# RPM Magnetic Sensor

An ESP-IDF project for measuring rotational speed (RPM) using a magnetic sensor.

## Project Overview

This project uses a Hall effect or similar magnetic sensor to detect rotations of a wheel, shaft, or other rotating component and calculate the RPM. The ESP32 microcontroller processes the signals from the sensor and can display or transmit the RPM readings.

## Features

- Non-contact RPM measurement using magnetic sensors
- Real-time RPM calculation and display
- Configurable sampling rate and filtering options
- Low power operation for battery-powered applications

## Hardware Requirements

- ESP32 development board
- Magnetic sensor (Hall effect sensor, magnetic switch, etc.)
- Magnet for rotation detection
- Optional display (OLED, LCD) for showing readings

## How to Use

1. Mount the magnetic sensor in a fixed position near the rotating component
2. Attach a magnet to the rotating component
3. Connect the sensor to the ESP32 as specified in the pin configuration
4. Power on the system and observe RPM readings

## How It Works

### Signal Flow: From Magnetic Pulse to Digital Signal

When a magnet passes by the Hall effect sensor:

1. **Physical Detection**: The Hall sensor detects the magnetic field
2. **Signal Generation**: The sensor output changes state (HIGH→LOW)
3. **Digital Input**: The ESP32 reads this as a falling edge on the GPIO pin
4. **Interrupt Trigger**: This falling edge triggers our ISR (Interrupt Service Routine)

The sensor input is configured with:
```c
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << sensor_pin),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,       // Enable pull-up
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_NEGEDGE,         // Interrupt on falling edge
};
```

Key configuration points:
- Pull-up resistor enabled (sensor pulls LOW when activated)
- Interrupt triggered on negative (falling) edge
- No debouncing in hardware (handled in software)

### Sensor Data Reading Logic

The system counts pulses through an interrupt service routine:

```c
static void IRAM_ATTR hall_sensor_isr(void* arg)
{
    int64_t now = esp_timer_get_time() / 1000; // Convert to milliseconds
    
    // Simple debouncing - ignore pulses that come too quickly
    if ((now - last_time) > DEBOUNCE_TIME) {
        pulse_count++;
        last_time = now;
    }
}
```

This code:
- Runs immediately when the magnet triggers the sensor
- Uses software debouncing to reject noise and false triggers
- Increments a counter each time a legitimate pulse is detected
- Is marked as `IRAM_ATTR` to run from ESP32's fast instruction memory

### RPM Calculation

RPM is calculated periodically in a dedicated FreeRTOS task:

```c
// Calculate RPM - multiply by 60 (seconds per minute)
// Divide by the number of magnets if you have multiple magnets on your wheel
rpm = (pulses * 60.0 * 1000) / RPM_CALC_INTERVAL;
```

Explanation of the formula:
- `pulses`: Number of sensor triggers detected in the last interval
- `* 60.0`: Converts from pulses-per-interval to pulses-per-minute
- `* 1000`: Accounts for the RPM_CALC_INTERVAL being in milliseconds
- `/ RPM_CALC_INTERVAL`: Normalizes to the exact measurement period

The code safely retrieves the pulse count using a critical section to avoid race conditions:
```c
portENTER_CRITICAL(&spinlock);
pulses = pulse_count - last_pulse_count;
last_pulse_count = pulse_count;
portEXIT_CRITICAL(&spinlock);
```

## Project Structure

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   ├── main.c             Main application code
│   ├── rpm_sensor.c       Sensor handling and RPM calculation
│   └── include/rpm_sensor.h  Header file with configurations
└── README.md              This is the file you are currently reading
```

## Building and Flashing

This project uses the ESP-IDF build system. To build and flash:

```
idf.py build
idf.py -p (PORT) flash
```

## FAQ

### What is the purpose of the IRAM_ATTR attribute in the code?

`IRAM_ATTR` is a special attribute in ESP-IDF that instructs the compiler to place a function in the ESP32's Instruction RAM (IRAM) rather than in Flash memory.

When a function is marked with `IRAM_ATTR`:

1. **Execution Speed**: The function runs from internal SRAM, which is significantly faster than executing from external Flash memory
2. **Interrupt Reliability**: Critical for Interrupt Service Routines (ISRs) like the Hall sensor handler in this project
3. **Flash Cache Independence**: The function remains accessible even when the Flash cache is disabled (which happens during Flash write operations)

In the RPM sensor application, `IRAM_ATTR` is applied to the hall sensor ISR to ensure that when a magnet triggers the Hall effect sensor and generates an interrupt, the handler responds immediately without any potential delay from Flash memory access. For precise RPM measurements, this timing reliability is crucial.

### How does the software debouncing work in this project?

The debouncing mechanism is implemented in the Hall sensor interrupt service routine:

```c
static void IRAM_ATTR hall_sensor_isr(void* arg)
{
    int64_t now = esp_timer_get_time() / 1000; // Convert to milliseconds
    
    // Simple debouncing - ignore pulses that come too quickly
    if ((now - last_time) > DEBOUNCE_TIME) {
        pulse_count++;
        last_time = now;
    }
}
```

**How it works:**

1. **Time-based filtering**: The code implements debouncing by checking if enough time has passed since the last valid pulse.
   
2. **DEBOUNCE_TIME constant**: This predefined value represents the minimum allowed time between valid pulse detections, typically in milliseconds.

3. **Rejection mechanism**: When the Hall sensor triggers the interrupt, the system:
   - Checks how much time has passed since the last valid pulse
   - Only accepts the new pulse if enough time has elapsed (greater than DEBOUNCE_TIME)
   - Otherwise ignores it as noise or bounce

This approach prevents electrical noise, mechanical vibration, or magnetic fluctuations from causing false readings that would artificially inflate the RPM count. It's particularly important in environments with electrical interference.

The debouncing logic introduces a trade-off: if DEBOUNCE_TIME is set too high, legitimate pulses might be missed at very high rotation speeds.
