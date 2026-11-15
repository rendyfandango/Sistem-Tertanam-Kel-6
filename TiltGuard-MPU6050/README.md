# TiltGuard — MPU6050 (Phases 1–2)

Native ESP-IDF Embedded C firmware for ESP32-WROOM-32U. This increment creates
the ESP32 project and verifies MPU6050 identity over I2C. No Arduino, PlatformIO,
Wire, or application C++ is used.

**Scope:** device attachment, synchronous register read, WHO_AM_I validation,
bounded error handling, and periodic connection diagnostics. Measurement register
configuration, calibration, filtering, CSV, and the 100 Hz acquisition task are
future phases. `mpu6050_init()` currently attaches the device; it does not wake or
configure it. The full request is preserved in `docs/project_requirements.md`.

## Wiring

| MPU6050 | ESP32 |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| AD0 | GND (address 0x68) |
| INT | Unconnected |

Use short wires and external SDA/SCL pull-ups to **3.3 V** (check the breakout's
existing pull-ups first). Internal pull-ups are enabled but are too weak to rely
on at 400 kHz. A typical starting point is 2.2–4.7 kΩ; verify rise times on the
actual bus. Keep GPIOs used for flash, boot strapping, and other peripherals in
mind when changing pins. GPIO6–11 are normally reserved for module flash.

## Build on this workstation

Detected SDK: `/home/sayed/.espressif/v6.1/esp-idf`, Git tag `v6.1`.
Detected compiler: Xtensa GCC 15.2.0 (`esp-15.2.0_20251204`).
Python: 3.14.4 from the existing EIM virtual environment.

The normal SDK `export.sh` did not recognize this installation's EIM tool
layout. The following helper sets process-local variables and paths for the
already installed tools; it does not install tools or edit the global SDK:

```bash
cd /home/sayed/Documents/UNPAD/Sayed_MPU6050_driver/Sistem-Tertanam-Kel-6/TiltGuard-MPU6050
source tools/activate_local_idf.sh
idf.py --version
idf.py set-target esp32
idf.py build
```

`set-target` resets generated configuration; use it for initial setup or a target
change. Subsequent edits need only `idf.py build`. On another computer, activate
that computer's ESP-IDF environment instead of this machine-specific helper.
The component requires ESP-IDF >= 5.3; only installed v6.1 has been build-verified.

Optional settings: `idf.py menuconfig` → **TiltGuard MPU6050**. Defaults are
GPIO21/22, unshifted address 0x68, 400000 Hz, 100 ms timeout. The FreeRTOS tick
rate is 1000 Hz in `sdkconfig.defaults`, ready for a future 10 ms period.

In VS Code, open this directory as the ESP-IDF project and select the installed
v6.1 environment. Use the commands above in a terminal if extension activation
does not pick up the EIM layout.

## Flash and monitor (manual hardware step)

No hardware was flashed during development. After approving firmware use on your
board, choose the real serial port and run these commands yourself:

```bash
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

The port is an example, not a detected device. Exit the monitor with Ctrl+].
Expected messages after a successful **real** transaction include
`Device detected at address 0x68` and `WHO_AM_I = 0x68`. They are printed only
on initial success or reconnection. No sensor measurements or tilt estimates
are emitted in this phase. On errors the application retries once per second,
reports the first failure and every tenth failure, and reports connection loss.
An address ACK alone is insufficient; the identity byte must also match.

## Learning and next steps

- `docs/software_architecture.md`: program flow, ownership, C concepts, build.
- `docs/register_map.md`: identity transaction and planned register settings.
- `docs/library_analysis.md`: inspected sources and simplification decisions.
- `docs/testing.md`: build evidence and unfilled hardware validation worksheet.
- `docs/roadmap.md`: all ten phases, scope and acceptance gates.

Finish the Phase 2 hardware checks before implementing Phase 3. Build success
does not establish successful I2C communication with a physical sensor.
