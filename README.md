# ESP32 RPM Magnetic Sensor

An ESP-IDF project for measuring rotational speed (RPM) using a magnetic sensor (e.g., Hall effect sensor) connected to an ESP32.

## Project Overview

This project utilizes an ESP32 microcontroller to read pulses from a magnetic sensor triggered by magnets attached to a rotating object. It calculates the Revolutions Per Minute (RPM) based on the pulse count over time and the number of magnets used per revolution.

## Features

- Non-contact RPM measurement.
- Configurable number of magnets per revolution.
- Real-time RPM calculation.
- Debouncing for sensor input signal.
- Uses FreeRTOS tasks for non-blocking operation.
- Outputs RPM readings to the serial monitor.

## Hardware Requirements

- ESP32 Development Board
- Magnetic Sensor (e.g., Hall Effect Sensor)
- Magnets (1 or more, evenly spaced if multiple)
- Connecting Wires

## Software Requirements

- ESP-IDF (Espressif IoT Development Framework) v4.x or later
- A compatible toolchain (GCC, CMake, Ninja)
- Serial terminal program (e.g., PuTTY, `idf.py monitor`)

## Configuration

Key parameters can be adjusted in `main/include/rpm_sensor.h`:

- `HALL_SENSOR_PIN`: The GPIO pin connected to the sensor's output.
- `DEBOUNCE_TIME`: The minimum time (in milliseconds) between valid pulses to filter noise.
- `RPM_CALC_INTERVAL`: The interval (in milliseconds) at which RPM is calculated and logged.

The number of magnets must be set in `main/rpm_sensor.c`:

- `NUM_MAGNETS`: Define this macro with the number of magnets attached to the rotating object (e.g., `#define NUM_MAGNETS 4`).

```c
// filepath: e:\RPM Magnetic Sensor\main\rpm_sensor.c
// ...
#define TAG "RPM_SENSOR"

#define NUM_MAGNETS 4 // <-- SET THIS VALUE

// ...
```

## Firmware Logic Explained

The firmware operates using interrupts and a dedicated FreeRTOS task.

1.  **Initialization (`rpm_sensor_init`)**:
    *   Configures the specified GPIO pin (`HALL_SENSOR_PIN`) as a digital input with an internal pull-up resistor.
    *   Sets up an interrupt to trigger on the falling edge of the signal from the sensor.
    *   Installs the global GPIO ISR service (if not already installed).
    *   Registers the specific ISR handler (`hall_sensor_isr`) for the sensor pin.

2.  **Interrupt Service Routine (`hall_sensor_isr`)**:
    *   **`IRAM_ATTR`**: This function is placed in the ESP32's fast Internal RAM (IRAM) for quick execution, crucial for handling interrupts reliably.
    *   **Pulse Detection**: Triggered automatically by the hardware whenever the sensor pin goes from HIGH to LOW (falling edge).
    *   **Debouncing**: It checks the time elapsed since the last *valid* pulse (`last_time`). If the current pulse arrived sooner than `DEBOUNCE_TIME` milliseconds after the last one, it's ignored as noise.
    *   **Counting**: If the pulse is considered valid (passes the debounce check), a global counter (`pulse_count`) is incremented. This counter is declared `volatile` because it's modified in an ISR and read elsewhere. The timestamp of this valid pulse is recorded in `last_time`.

3.  **RPM Calculation Task (`rpm_task`)**:
    *   **Dedicated Task**: Runs as a separate FreeRTOS task created by `rpm_sensor_start`. This allows RPM calculation to happen periodically without blocking other potential operations.
    *   **Periodic Execution**: The task sleeps for `RPM_CALC_INTERVAL` milliseconds using `vTaskDelay`.
    *   **Pulse Reading**: After waking up, it safely reads the number of pulses counted since its last execution. It enters a critical section (using the spinlock `portENTER_CRITICAL`/`portEXIT_CRITICAL`) to read `pulse_count`, calculate the difference (`pulses`) from the previous reading (`last_pulse_count`), and update `last_pulse_count`. It then exits the critical section. This prevents race conditions where the ISR might modify the count while the task is reading it.
    *   **RPM Calculation**: Calculates the RPM using the formula:
        `RPM = (pulses * 60.0 * 1000) / (RPM_CALC_INTERVAL * NUM_MAGNETS)`
        - `pulses`: Pulses counted during the interval.
        - `* 60.0 * 1000`: Converts pulses per millisecond interval to pulses per minute.
        - `/ RPM_CALC_INTERVAL`: Normalizes the count based on the interval duration.
        - `/ NUM_MAGNETS`: Adjusts the count to reflect full revolutions based on the number of magnets used.
    *   **Logging**: Prints the raw pulse count for the interval and the calculated RPM to the serial monitor using `ESP_LOGI`.

4.  **Main Application (`app_main` in `main.c`)**:
    *   Initializes the RPM sensor module by calling `rpm_sensor_init`.
    *   Starts the RPM measurement process by calling `rpm_sensor_start`, which creates the `rpm_task`.
    *   The `app_main` function can then exit or proceed with other application tasks (like Wi-Fi, display updates, etc.), while the `rpm_task` continues running in the background.

## How to Build and Flash

1.  Ensure ESP-IDF is installed and the environment is activated.
2.  Navigate to the project directory (`e:\RPM Magnetic Sensor`).
3.  Configure the project (optional): `idf.py menuconfig`
4.  Build the project: `idf.py build`
5.  Flash the firmware (replace `COMx` with your ESP32's serial port): `idf.py -p COMx flash`
6.  Monitor the output: `idf.py -p COMx monitor`

## Troubleshooting

- **No Readings:** Check sensor wiring, GPIO pin configuration (`HALL_SENSOR_PIN`), and magnet polarity/distance. Ensure the sensor is actually outputting pulses. Verify the interrupt edge (`GPIO_INTR_NEGEDGE` or `GPIO_INTR_POSEDGE`) matches the sensor's output behavior. Check pull-up/pull-down configuration.
- **Inaccurate RPM:** Verify `NUM_MAGNETS` is set correctly. Adjust `DEBOUNCE_TIME` if noise is suspected or if very high RPMs are causing valid pulses to be missed. Ensure magnets are evenly spaced. Check the accuracy of `RPM_CALC_INTERVAL`.
- **Task Creation Failed:** Increase stack size for `rpm_task` in `rpm_sensor_start` if necessary (check ESP-IDF logs for memory issues).
- **ISR Issues:** Ensure the ISR (`hall_sensor_isr`) is short and fast. Avoid complex calculations or blocking calls inside the ISR. Use `ESP_LOGI` sparingly or outside the ISR for debugging.