# Project: TiltGuard — MPU6050 Embedded Driver and Signal Processing

## 1. Project Background

I am an Electrical Engineering student developing an embedded systems project called TiltGuard, a real-time tilt, motion, and environmental monitoring system.

The complete system consists of:

- MPU6050 — I2C
- BMP280 — SPI
- SSD1306 OLED — I2C
- MicroSD Card — SPI
- Wi-Fi SoftAP and UDP communication
- LED, buzzer, and push button
- FreeRTOS-based multitasking

My individual responsibility is the MPU6050 subsystem, including hardware driver development, register-level programming, sensor calibration, signal processing, and preparation for FreeRTOS integration.

The primary academic objective is to study an existing hardware library, identify its essential functions, remove unnecessary features, and develop a simplified embedded driver specifically tailored to the project's requirements.

## 2. Development Environment

Use the following specifications:

- Microcontroller: ESP32-WROOM-32U
- ESP-IDF target: esp32
- Framework: Native ESP-IDF
- Programming language: Embedded C
- Operating system: Ubuntu Linux
- IDE: Visual Studio Code with ESP-IDF extension
- Build system: CMake
- RTOS: FreeRTOS
- Communication protocol: I2C

Do not use the Arduino framework, Arduino libraries, Wire.h, PlatformIO, or C++.

Inspect the installed ESP-IDF version and use the compatible native ESP-IDF I2C master API. Prefer the modern `driver/i2c_master.h` API where supported.

## 3. Hardware Configuration

Use the following initial wiring configuration:

| MPU6050 | ESP32-WROOM-32U |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| AD0 | GND |
| INT | Not connected initially |

Configuration:

- I2C device address: 0x68
- I2C clock frequency: 400 kHz
- Accelerometer full-scale range: ±2g
- Gyroscope full-scale range: ±250 degrees/s
- Target sampling frequency: 100 Hz
- Target sampling period: 10 ms
- Digital Low-Pass Filter configuration: DLPF_CFG = 3

Make these parameters configurable rather than unnecessarily hardcoding them throughout the source code.

## 4. Library Analysis and Simplification

Study the official MPU6050 datasheet, register map, and existing open-source MPU6050 driver implementations, such as I2Cdevlib.

Identify which functions are necessary for TiltGuard and which functions can be excluded.

Retain or implement:

- Device initialization
- I2C communication
- Register read and write
- Device identification using WHO_AM_I
- Power management
- Accelerometer configuration
- Gyroscope configuration
- Sample rate configuration
- Digital Low-Pass Filter configuration
- Burst reading
- Raw data acquisition
- Physical unit conversion
- Sensor calibration
- Error handling

Exclude unnecessary features:

- Digital Motion Processor (DMP)
- FIFO management
- Auxiliary I2C master
- External sensor control
- Quaternion processing
- Advanced interrupt configurations
- Motion detection hardware features that are not required
- Unused register definitions and functions

Preserve attribution and comply with the license if any existing source code is reused. Do not claim that a library has been analyzed unless its source has actually been inspected.

Create documentation explaining which features were retained, removed, or rewritten and why.

## 5. Register-Level Programming

Implement MPU6050 configuration directly through its hardware registers using native ESP-IDF I2C functions.

Use the following essential registers:

| Register | Address |
|---|---|
| SMPLRT_DIV | 0x19 |
| CONFIG | 0x1A |
| GYRO_CONFIG | 0x1B |
| ACCEL_CONFIG | 0x1C |
| ACCEL_XOUT_H | 0x3B |
| TEMP_OUT_H | 0x41 |
| GYRO_XOUT_H | 0x43 |
| PWR_MGMT_1 | 0x6B |
| PWR_MGMT_2 | 0x6C |
| WHO_AM_I | 0x75 |

Initial register configurations:

- PWR_MGMT_1 = 0x01
- SMPLRT_DIV = 0x09
- CONFIG = 0x03
- GYRO_CONFIG = 0x00
- ACCEL_CONFIG = 0x00

Explain the purpose of each register and its relevant bit fields.

Demonstrate the correct use of:

- Bitwise AND
- Bitwise OR
- Bitwise NOT
- Bit shifting
- Bit masks
- Fixed-width integer types
- Pointers
- Structures
- Static functions
- Function prototypes
- Header guards

Use read-modify-write operations where appropriate to preserve unrelated register bits.

## 6. Raw Data Acquisition

Implement a 14-byte burst read starting from register 0x3B.

Extract:

- Accelerometer X, Y, Z
- Gyroscope X, Y, Z
- Internal sensor temperature

Correctly combine the high and low bytes into signed 16-bit integers.

Handle negative two's complement values correctly.

Create a structure named `mpu6050_raw_t` containing the raw measurements.

## 7. Physical Unit Conversion

Convert the raw measurements into physical quantities.

For accelerometer at ±2g:

- Sensitivity: 16384 LSB/g
- Acceleration (g) = raw value / 16384.0
- Acceleration (m/s²) = acceleration (g) × 9.80665

For gyroscope at ±250 degrees/s:

- Sensitivity: 131 LSB/(degrees/s)
- Angular velocity = raw value / 131.0

For internal temperature:

- Temperature (°C) = raw temperature / 340.0 + 36.53

Create an `mpu6050_data_t` structure containing converted measurements.

Ensure that the conversion sensitivity remains consistent with the configured accelerometer and gyroscope ranges.

The MPU6050 internal temperature is optional diagnostic data and should not be treated as the primary ambient temperature measurement because the complete TiltGuard system uses BMP280.

## 8. Sensor Calibration

Implement accelerometer and gyroscope calibration.

### Gyroscope Calibration

- Collect approximately 1000 samples while the sensor remains stationary.
- Calculate the average offset for each axis.
- Subtract the calculated offset from subsequent measurements.
- Detect excessive motion during calibration and report calibration failure if appropriate.

### Accelerometer Calibration

Initially implement stationary flat-position offset calibration with positive Z pointing upward.

Expected measurements:

- X = 0g
- Y = 0g
- Z = 1g

Preserve gravitational acceleration rather than subtracting it completely.

Explain the limitations of single-position calibration and how six-position calibration could improve accuracy.

Store calibration parameters in a dedicated structure.

## 9. Signal Processing and Sensor Fusion

Implement tilt estimation using accelerometer and gyroscope measurements.

### Accelerometer-Based Tilt

Roll:

roll_acc = atan2(ay, az)

Pitch:

pitch_acc = atan2(-ax, sqrt(ay² + az²))

Convert the resulting angles from radians to degrees.

### Gyroscope-Based Tilt

Estimate angular displacement through discrete-time integration:

angle[n] = angle[n-1] + angular_velocity[n] × dt

Use actual elapsed time between valid samples rather than assuming that every sample arrives at exactly 10 ms.

### Complementary Filter

Implement:

angle[n] = alpha × (angle[n-1] + angular_velocity[n] × dt) + (1 - alpha) × angle_acc[n]

Initial configuration:

- Alpha = 0.98
- Nominal sampling period = 0.01 seconds

Initialize roll and pitch using accelerometer measurements to avoid unnecessary startup transients.

Provide both raw accelerometer-based angles and filtered angles for comparison.

Document the following:

- Accelerometer noise
- Gyroscope drift
- Digital low-pass filtering
- Sensor fusion
- Complementary filter transfer function
- Relationship between alpha, sampling period, and cutoff frequency
- Limitations during translational acceleration
- Limitations of simplified roll and pitch integration during complex three-dimensional rotations

Do not implement absolute yaw estimation because MPU6050 does not have a magnetometer.

Keep the signal processing implementation separate from the low-level hardware driver.

## 10. Modular Software Architecture

Create the following ESP-IDF project structure:

```text
TiltGuard-MPU6050/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
├── main/
│   ├── CMakeLists.txt
│   └── main.c
├── components/
│   ├── mpu6050/
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── mpu6050.h
│   │   ├── mpu6050.c
│   │   └── mpu6050_regs.h
│   └── tilt_filter/
│       ├── CMakeLists.txt
│       ├── include/
│       │   └── tilt_filter.h
│       └── tilt_filter.c
├── docs/
│   ├── library_analysis.md
│   ├── register_map.md
│   ├── software_architecture.md
│   └── testing.md
└── tools/
    ├── serial_logger.py
    └── plot_sensor_data.py
```

Separate the following responsibilities:

- `main.c`: Application initialization and FreeRTOS task management.
- `mpu6050.h`: Public API and data structures.
- `mpu6050.c`: Hardware driver implementation.
- `mpu6050_regs.h`: Private register definitions and bit masks.
- `tilt_filter.h`: Public signal processing API.
- `tilt_filter.c`: Tilt estimation and complementary filter.
- `CMakeLists.txt`: Component dependencies and build configuration.

Follow good embedded software engineering practices:

- Encapsulation
- Separation of concerns
- Modular programming
- Hardware abstraction
- Meaningful variable names
- Configurable parameters
- Minimal global variables
- Appropriate use of static and const
- Error propagation using `esp_err_t`

Avoid unnecessary architectural complexity.

## 11. Required Driver API

Design a clean public API with functions equivalent to:

```c
mpu6050_init();
mpu6050_deinit();
mpu6050_check_connection();
mpu6050_read_raw();
mpu6050_read_data();
mpu6050_calibrate();
mpu6050_set_accel_range();
mpu6050_set_gyro_range();
mpu6050_set_dlpf();
mpu6050_set_sample_rate();
```

Internal functions should include:

```c
mpu6050_write_reg();
mpu6050_read_reg();
mpu6050_read_bytes();
mpu6050_update_bits();
```

Signal processing functions should include:

```c
tilt_filter_init();
tilt_filter_update();
tilt_filter_reset();
```

These are proposed function names, not mandatory function signatures. Design appropriate arguments, return types, and device context structures.

The MPU6050 driver must accept an externally initialized I2C bus handle so that the SSD1306 OLED can share the same bus during final integration.

The MPU6050 driver must not delete or reinitialize an externally owned I2C bus.

## 12. FreeRTOS Implementation

Create a dedicated MPU6050 acquisition task.

Requirements:

- Target sampling frequency: 100 Hz
- Periodic scheduling using `vTaskDelayUntil()` or the appropriate equivalent.
- Configure sufficient FreeRTOS tick resolution.
- Use `esp_timer_get_time()` to measure actual sampling intervals.
- Read sensor measurements.
- Apply calibration.
- Calculate roll and pitch.
- Apply complementary filtering.
- Publish processed measurements through a FreeRTOS queue.
- Maintain predictable timing.
- Handle I2C communication failures without publishing invalid measurements as valid data.

Create a measurement structure containing:

- Timestamp
- Acceleration X, Y, Z
- Angular velocity X, Y, Z
- Accelerometer-based roll
- Accelerometer-based pitch
- Filtered roll
- Filtered pitch
- Actual sampling interval
- Measurement validity status

Provide a simple latest-value queue or clearly documented data-sharing interface for future integration with OLED, MicroSD, Wi-Fi, and alarm tasks.

Explain that a single FreeRTOS queue with multiple consumers does not automatically broadcast measurements.

Do not implement other team members' hardware subsystems.

## 13. Error Handling and Reliability

Implement error handling for:

- MPU6050 not detected
- Invalid WHO_AM_I value
- I2C initialization failure
- I2C communication timeout
- Register read/write failure
- Invalid configuration parameters
- Calibration failure
- Invalid sampling intervals
- Sensor disconnection

Use `ESP_LOGI`, `ESP_LOGW`, and `ESP_LOGE` appropriately.

Avoid excessive logging inside the 100 Hz sampling task because it can affect timing.

Do not silently ignore hardware communication errors.

## 14. Serial Monitor Output

Provide human-readable diagnostic output and a configurable CSV streaming mode.

Expected diagnostic output example:

```text
I MPU6050: Device detected at address 0x68
I MPU6050: WHO_AM_I = 0x68
I MPU6050: Accelerometer range = ±2g
I MPU6050: Gyroscope range = ±250 dps
I MPU6050: Target sampling rate = 100 Hz
I MPU6050: Starting calibration
I MPU6050: Calibration completed

Roll: 12.35 deg
Pitch: -3.20 deg
```

CSV format:

```text
timestamp_us,ax,ay,az,gx,gy,gz,roll_acc,pitch_acc,roll_filtered,pitch_filtered,dt,valid
```

Do not fabricate sensor measurements. The values shown above are examples of the expected output format.

## 15. Python Data Analysis

Create a Python script that reads serial data and saves it into a CSV file.

Create another Python script using NumPy and Matplotlib to analyze actual recorded measurements.

Generate plots for:

1. Raw accelerometer measurements.
2. Raw gyroscope measurements.
3. Calibrated sensor measurements where recorded.
4. Accelerometer-based roll and pitch.
5. Complementary-filtered roll and pitch.
6. Accelerometer angle versus complementary-filtered angle.
7. Sampling interval and sampling frequency.
8. Static noise and angular drift.

Calculate relevant performance metrics:

- Mean
- Standard deviation
- RMS noise
- Angular error against a known reference
- Actual average sampling frequency
- Sampling jitter

Do not generate artificial experimental results unless explicitly requested and clearly labeled as simulated data.

## 16. Testing and Validation

Prepare a testing procedure for:

- I2C device detection
- WHO_AM_I verification
- Register configuration verification
- Raw data acquisition
- Stationary gyroscope offset
- Accelerometer magnitude near 1g
- Tilt measurements at 0°, 15°, 30°, 45°, 60°, and 90°
- Complementary filter response
- Long-duration drift
- Sampling frequency accuracy
- Sampling jitter
- Sensor disconnection and recovery
- FreeRTOS queue operation

At each reference angle, clearly identify the axis of rotation and the physical sensor orientation.

Include a testing table that distinguishes expected values, actual measurements, errors, and validation status.

## 17. Development Workflow

Develop the project incrementally in the following phases:

Phase 1 — Create ESP-IDF project and verify target configuration.

Phase 2 — Initialize I2C and detect MPU6050 using WHO_AM_I.

Phase 3 — Develop register read/write functions and configure sensor registers.

Phase 4 — Implement 14-byte burst reading and data conversion.

Phase 5 — Implement sensor calibration.

Phase 6 — Implement tilt estimation and complementary filtering.

Phase 7 — Refactor the driver and signal processing into modular ESP-IDF components.

Phase 8 — Implement FreeRTOS acquisition task and data-sharing interface.

Phase 9 — Implement Python logging and plotting tools.

Phase 10 — Complete documentation, build verification, and hardware testing procedure.

After each phase:

- Explain what has been implemented.
- Explain the relevant Embedded C concepts.
- Identify files that were created or modified.
- Explain how the code interacts with the physical hardware.
- Provide commands for building, flashing, and monitoring.
- Identify any remaining issues.
- Verify compilation whenever the installed environment permits it.

## 18. Educational Requirements

I have experience with Arduino-based embedded hardware development but am currently learning native Embedded C, ESP-IDF, register-level programming, and embedded software architecture.

Therefore, do not simply generate unexplained code.

Explain the implementation from an Electrical Engineering and Embedded Systems perspective, including:

- Program flow of control
- ESP-IDF application startup
- app_main()
- Header and source files
- Compilation and linking
- CMake component registration
- Memory allocation
- Pointers and structures
- Hardware registers
- I2C transaction sequences
- Device drivers
- Hardware abstraction
- FreeRTOS tasks
- Scheduling
- Queues
- Mutexes
- Signal processing

Add meaningful comments explaining why important operations are performed.

## 19. Execution Instructions

Act as a senior Embedded Systems Firmware Engineer and technical mentor.

First inspect the existing workspace, installed ESP-IDF version, available toolchain, and existing project files.

Create the project files directly in the workspace. Do not merely provide code snippets in the chat.

Use:

```bash
idf.py set-target esp32
idf.py build
```

Do not overwrite unrelated files or modify the global ESP-IDF installation without permission.

If compilation fails, inspect the error messages, identify the root cause, and fix the implementation.

Do not flash the connected hardware automatically without permission.

Do not claim successful hardware testing unless actual hardware measurements are available.

Begin with Phase 1 and Phase 2. Complete and verify the initial project setup and MPU6050 communication implementation before proceeding to the remaining phases. Keep the full project requirements as the development roadmap.