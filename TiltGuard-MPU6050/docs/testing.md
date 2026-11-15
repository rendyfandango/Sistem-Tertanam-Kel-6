# Verification record and hardware worksheet

## Software verification — 2026-11-15 (Asia/Jakarta)

| Check | Actual result | Status |
|---|---|---|
| SDK inspection | Installed Git tag `v6.1`, `idf.py --version`: ESP-IDF v6.1 | PASS |
| Compiler inspection | Xtensa GCC 15.2.0, esp-15.2.0_20251204 | PASS |
| `idf.py set-target esp32` | Configuration and generation completed | PASS |
| `idf.py build` | `Project build complete`; app/bootloader/partition binaries generated | PASS |
| Generated target | `CONFIG_IDF_TARGET="esp32"` | PASS |
| Generated tick | `CONFIG_FREERTOS_HZ=1000` | PASS |
| Generated hardware settings | SDA21, SCL22, 0x68, 400000 Hz, 100 ms | PASS |
| Host contract test | `bash tests/run_host_tests.sh` exited 0; strict C11 warnings enabled | PASS (mock I2C) |
| Physical MPU6050 transaction | No firmware flashed, no physical capture taken | NOT RUN |

The host tests compile the **actual driver source** against minimal ESP-IDF API
stubs. They verify invalid arguments, failed device registration, expected and
unexpected identities, unchanged caller output after timeout/transport failure,
0x69 addressing with 0x68 identity, and failed removal followed by a successful
retry. There is no mock bus deletion API, so an accidental driver call to delete
the externally owned bus would also fail to link. These tests do not validate
electrical timing, the real SDK implementation, or sensor behavior.

Reproduce from the project directory:

```bash
source tools/activate_local_idf.sh
idf.py build
bash tests/run_host_tests.sh
```

First-time target setup, flashing, and monitoring commands are in README.
Compiler/linker validation uses real ESP32 headers/toolchain; host tests use
host `cc` and synthetic return values only. No synthetic sensor experiments
or plots are provided.

Environment note: stock `export.sh` could not find the EIM tools using its normal
directory convention. We used a local environment helper, with no SDK/tool
installation changes. An initial CMake version check used a variable unavailable
in that scope; it was corrected to read SDK version metadata explicitly. The
successful target setup and build listed above were run after that correction.

## Phase 2 hardware procedure (pending)

1. With power off, check 3.3 V supply, common ground, AD0 low, GPIO21 SDA and
   GPIO22 SCL. Confirm pull-up resistors terminate at 3.3 V and INT is unused.
2. Manually flash after authorizing hardware use; open the serial monitor. Save
   the actual boot log. No port has been selected or opened by this delivery.
3. Confirm the configured address and full identity log, not just successful bus
   creation. If available, inspect a logic analyzer for 400 kHz SCL, address
   ACKs, register pointer 0x75, repeated START, byte 0x68 and final master NACK.
4. With safe power-off rewiring, test an absent module, swapped SDA/SCL, and AD0
   mismatch. Confirm errors and no false detection. Restore the wiring and
   power-cycle/reconnect; confirm detection on a later retry. Do not short live
   lines to force errors. A hard-stuck bus may require electrical recovery or a
   reboot; this phase retries reads and does not promise bus-reset recovery.
5. With AD0 high and configuration changed to 0x69, repeat. WHO_AM_I remains
   0x68. Do not use 0xD0/0xD1 as the configured ESP-IDF address.
6. A deliberate unexpected identity should be tested with a controlled mock or
   known alternate device, not by writing WHO_AM_I (a read-only register).

| Test | Expected | Actual measurement/log | Error | Validation |
|---|---|---|---|---|
| Device detection | Successful WHO_AM_I transaction at 0x68 | — | — | NOT RUN |
| Identity | Full byte 0x68 from 0x75 | — | — | NOT RUN |
| Bus waveform | Approx. 400 kHz, correct repeated START | — | — | NOT RUN |
| Absent sensor | Read error, no valid detection | — | — | NOT RUN |
| Unexpected identity | ESP_ERR_INVALID_RESPONSE | Host mock returns this error | N/A | MOCK PASS; hardware NOT RUN |
| Reconnect | Successful identity on later retry | — | — | NOT RUN |
| AD0 high, address 0x69 | Identity remains 0x68 | — | — | NOT RUN |

## Later phase validation plan

All rows below are future work. Do not fill actual values from expectations.
Record board/module ID, firmware revision, mounting, supply voltage, pull-ups,
reference tool accuracy, calibration state and units alongside each capture.

| Test | Expected target/procedure | Actual | Error | Status |
|---|---|---|---|---|
| Register readback | PWR1=0x01, divider=0x09, CONFIG=0x03, gyro/accel=0x00 after controlled initialization | — | — | NOT RUN |
| Raw burst | 14 ordered bytes; signed-boundary host vectors plus real stationary readings | — | — | NOT RUN |
| Gyro stationary offsets | Average ~1000 samples; calibrated axes near zero; reject motion | — | — | NOT RUN |
| Accel stationary | Flat +Z up: approximately (0,0,1)g; magnitude near 1g | — | — | NOT RUN |
| Filter response | Compare accelerometer and fused angles after controlled tilt steps | — | — | NOT RUN |
| Long-duration drift | Static recording ≥10 minutes; report drift versus elapsed time | — | — | NOT RUN |
| Sample rate | Nominal 100 Hz; frequency=(N−1)/(last−first timestamp) | — | — | NOT RUN |
| Sampling jitter | Plot intervals; report SD, min/max, missed deadlines versus 10 ms | — | — | NOT RUN |
| Disconnection during acquisition | Error/invalid state; no invalid data published as valid; safe recovery | — | — | NOT RUN |
| Latest-value queue | Length one, overwrite and delayed consumer behavior; verify fanout design | — | — | NOT RUN |

Define tolerances before judging pass/fail; account for MPU6050 specifications,
reference-fixture uncertainty, calibration, noise, and timing load. For stationary
windows report mean, standard deviation, RMS noise relative to the window mean,
and angle error relative to the known fixture angle. Keep transient and static
analysis windows separate. Report raw counts separately from calibrated units.

## Reference angle worksheet (Phases 6–10)

Use the chip/module's documented right-handed axes, not an assumed breakout
silkscreen. Begin flat with +Z vertically upward and X/Y horizontal. Test roll
by rotating around +X with +Y rising toward vertical; test pitch separately
around +Y with +X tipping downward. These positive rotations correspond to the
specified accelerometer formulas. Hold each position motionless after settling.
Use a measured jig/inclinometer, keep the other nominal tilt at zero, and record
the independent reference angle. At pitch +90°, roll is undefined/unreliable;
do not score it as a zero-roll test.

| Rotation and orientation | Reference (deg) | Measured accel angle | Measured filtered angle | Filtered minus reference | Status |
|---|---:|---|---|---|---|
| +X roll, +Z up | 0 | — | — | — | NOT RUN |
| +X roll, +Y rises | 15 | — | — | — | NOT RUN |
| +X roll, +Y rises | 30 | — | — | — | NOT RUN |
| +X roll, +Y rises | 45 | — | — | — | NOT RUN |
| +X roll, +Y rises | 60 | — | — | — | NOT RUN |
| +X roll, +Y vertical up | 90 | — | — | — | NOT RUN |
| +Y pitch, +Z up | 0 | — | — | — | NOT RUN |
| +Y pitch, +X tips down | 15 | — | — | — | NOT RUN |
| +Y pitch, +X tips down | 30 | — | — | — | NOT RUN |
| +Y pitch, +X tips down | 45 | — | — | — | NOT RUN |
| +Y pitch, +X tips down | 60 | — | — | — | NOT RUN |
| +Y pitch, +X vertical down | 90 | — | — | — | NOT RUN |
