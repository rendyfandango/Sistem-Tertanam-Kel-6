# Program flow and Embedded C notes

## Phase 1 — project setup

Created top-level `CMakeLists.txt`, `sdkconfig.defaults`, `main/CMakeLists.txt`,
`main/Kconfig.projbuild`, and a private generated-file `.gitignore`.
The target is `esp32`, suitable for ESP32-WROOM-32U. A 1000 Hz RTOS tick permits
10 ms scheduling later; it does not make this phase acquire data at 100 Hz.

ESP-IDF's bootloader loads the application from flash. SDK startup initializes
runtime services and starts FreeRTOS, which calls `app_main()` in the main task.
Unlike Arduino's `setup()`/`loop()`, `app_main()` is the C application entry
point and can start tasks or perform blocking work itself. Here it runs a small
diagnostic loop. `vTaskDelay()` blocks this task so other tasks and idle tasks
can execute; there is no busy wait.

CMake's `project.cmake` imports the SDK build system. `idf_component_register`
lists each component's `.c` files, include paths, and dependencies.
`REQUIRES esp_driver_i2c` exposes the I2C header needed by the driver's public
header. The application requires `mpu6050` and `esp_driver_gpio`. Setting
`COMPONENTS main` builds the dependency closure rather than unrelated subsystems.
The compiler turns each `.c` file into object code, component archives collect
objects, and the linker resolves symbols into an ELF firmware image. ESP-IDF
then creates flashable binaries. SDK internal C++ runtime components can appear
in the build even though all our application sources are C.

Hardware interaction in Phase 1 is only prepared through configuration. The
build itself does not access an ESP32. Commands for this phase:

```bash
source tools/activate_local_idf.sh
idf.py set-target esp32
idf.py build
# Manual hardware steps only, with the correct port:
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

## Phase 2 — identity read

Created `main/main.c`, `components/mpu6050/CMakeLists.txt`,
`components/mpu6050/include/mpu6050.h`, `mpu6050.c`, and `mpu6050_regs.h`.

Flow:

1. Validate distinct output-capable SDA and SCL pins.
2. Create I2C bus 0 in `app_main()`. Failure is logged and startup stops.
3. Attach one MPU6050 device with a 7-bit address and per-device clock speed.
   Attachment allocates software resources; it does not establish presence.
4. Repeatedly read WHO_AM_I at 0x75 with a 100 ms maximum transaction wait.
5. Compare the returned byte with 0x68. Log presence, unexpected identity,
   transfer failure, and transitions between connected and disconnected.
6. Delay one second and retry. This interval is diagnostic, not a sample rate.

The driver leaves the chip's power and measurement configuration untouched.
WHO_AM_I can be accessed without waking the sensing engines. Phase 3 will
explicitly establish power, clock, ranges, DLPF, and sample rate.

`mpu6050.h` is the public contract: function prototypes tell the compiler each
function's argument and return types. Include guards prevent duplicate type
declarations. Private register definitions remain in `mpu6050_regs.h`.
`static` on the register reader limits visibility to one translation unit;
`static const` on the log tag provides stable read-only shared text.

`mpu6050_config_t` groups settings in a structure. A pointer passed to a
function refers to caller-owned memory rather than copying a whole object.
`const mpu6050_config_t *` prohibits modifying settings through that pointer.
`mpu6050_handle_t` hides the definition of the device structure from callers.
`mpu6050_handle_t *out` lets initialization write the new handle into the
caller's variable; initialize that variable to NULL first. `uint8_t` represents
the one-byte register address and value with a known width.

One small device context is allocated by `calloc()` at startup, not per read.
The allocation and I2C device registration can fail; both errors propagate as
`esp_err_t`. `mpu6050_deinit()` removes the device and frees its context only
after successful removal. The application owns the bus, and only the application
may delete it after **all** attached devices have been removed. No global device
singleton is used. The diagnostic application intentionally retains its bus and
device for the lifetime of its infinite loop.

The register reader uses a temporary byte and publishes it only on transfer
success. Callers must check the return code before interpreting the output.
An unexpected but successfully received identity is returned for diagnosis with
`ESP_ERR_INVALID_RESPONSE`; a timeout/NACK is propagated from ESP-IDF unchanged.
The application controls log policy, avoiding driver/application duplicate logs.

## Concurrency and future integration

Currently one task owns the MPU6050. No mutex is needed for this use. ESP-IDF's
bus transaction synchronization permits other devices to share the bus, but
device lifetime and future read-modify-write sequences still require application
serialization. Do not call deinit while another task uses the handle. Future
configuration and calibration should run under the acquisition task's ownership
or a mutex protecting the entire sequence, not individual bytes only.

Phase 8 will use a dedicated task, `vTaskDelayUntil()`, actual elapsed time from
`esp_timer_get_time()`, and a length-one latest-value queue. A single queue with
several receivers distributes entries; it does not broadcast them. A dispatcher
with one queue per subscriber, or a protected latest-value snapshot with explicit
notification, is required for OLED, SD, UDP, and alarm consumers.

Build/flash/monitor commands for Phase 2 are the same as Phase 1 (`set-target`
need not be repeated). See `testing.md` for verification status and outstanding
hardware checks. Bit masks, signed burst extraction, physical conversion, and
fusion will be taught alongside their implementations in subsequent phases.
