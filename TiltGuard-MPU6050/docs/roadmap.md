# Incremental development roadmap

The original full specification is preserved in `project_requirements.md`.
This delivery covers Phases 1 and 2 in software. Physical verification of Phase 2
is still required; do not infer sensor success from a successful build.

| Phase | Scope/files | Acceptance gate | Current status |
|---|---|---|---|
| 1 | Project CMake, target, tick configuration, environment | `idf.py set-target esp32`, `idf.py build` | See build record in testing.md |
| 2 | Bus creation, driver attach/deinit, register read, WHO_AM_I | Build plus real 0x68 identity and error-path checks | Software implemented; hardware pending |
| 3 | Private register helpers, masked writes, setters | Read back power, range, DLPF and divider; no unrelated bit changes | Planned |
| 4 | 14-byte burst, raw/data structs, range-aware conversion | Signed boundary tests and actual stationary readings | Planned |
| 5 | Dedicated calibration structure, ~1000 stationary samples | Motion rejection; gyro offsets reduced, +Z gravity preserved | Planned |
| 6 | `tilt_filter` component: accel tilt, elapsed-time integration, complementary filter | Initialize from accel, validate dt, compare response | Planned |
| 7 | Review/refine modular component boundaries | Hardware, processing and task concerns remain separate | Planned; driver split established early |
| 8 | 100 Hz task, timestamps, validity, latest-value queue | Timing, no invalid samples treated as valid, queue semantics | Planned |
| 9 | Serial logger, actual-data plotting and metrics | Correct CSV parsing and no fabricated experiments | Planned |
| 10 | Educational docs, final build and hardware validation | Complete actual/expected/error/status worksheets | Planned |

After each phase, update the implementation notes with changed files, C concepts,
physical transactions, commands, verification evidence, and outstanding issues.
The build/flash/monitor sequence in README applies throughout; flashing requires
explicit hardware authorization and must not run automatically.

## Requirements to carry into the next phases

- Defaults: ±2g, ±250 dps, DLPF_CFG=3, SMPLRT_DIV=9, 100 Hz; centralize settings.
- Public APIs: raw/data read, calibration, range/DLPF/rate setters. Add private
  write/read-bytes/update-bits helpers when used; propagate every transfer error.
- Raw order: ax, ay, az, temperature, gx, gy, gz from 0x3B. Combine bytes safely
  into signed 16-bit values, including negative two's-complement boundaries.
- At default ranges: 16384 LSB/g, 9.80665 m/s² per g, 131 LSB/(deg/s),
  temperature raw/340 + 36.53 °C. Scale must follow the active ranges. Internal
  die temperature is diagnostic; ambient temperature belongs to BMP280.
- Flat +Z calibration expects (0,0,1)g, not zero on every axis. Single-position
  offsets do not identify scale, cross-axis error or all mounting errors; explain
  the improvement from six-position calibration. Reject motion before commit.
- Use atan2(ay,az) for roll and atan2(-ax,sqrt(ay²+az²)) for pitch, in degrees.
  Start the filter at accel angles, integrate valid elapsed time, then fuse with
  nominal alpha=0.98 at dt=0.01 s. Document noise, drift, DLPF delay, complementary
  transfer functions, alpha/cutoff relation, translation and 3D rotation limits.
  No absolute yaw without a heading reference.
- Measure dt between valid samples with `esp_timer_get_time()` and bound it;
  decide how gaps reset/reinitialize integration. Use periodic scheduling and
  restrained logs. Do not equate register polling with perfectly synchronized
  fresh hardware samples; consider checking data-ready when appropriate.
- Measurement payload: timestamp, calibrated acceleration/angular velocity,
  accel angles, filtered angles, dt and validity. Queue length one supports
  overwrite/latest data. Multiple consumers of one queue do not get a broadcast.
- CSV baseline: `timestamp_us,ax,ay,az,gx,gy,gz,roll_acc,pitch_acc,roll_filtered,pitch_filtered,dt,valid`.
  Explicitly define units. Record additional raw and calibration metadata for
  raw-vs-calibrated plots rather than falsely calling converted data raw counts.
- NumPy/Matplotlib tools must use actual captures: plot raw and calibrated
  channels where present, accel and filtered angles, comparisons, intervals,
  frequency, noise and drift. Report means, SD, RMS noise, reference angle errors,
  average sampling frequency and jitter; label static recording windows.
- Validate at 0°,15°,30°,45°,60°,90° with declared rotation axis and orientation.
  Test drift, response, disconnection/recovery and queue behavior. Record actual
  values separately from expected ones. Do not manufacture experimental results.
- Retain shared-bus ownership, avoid other team members' hardware, and exclude
  DMP/FIFO/auxiliary-master/quaternion/unused interrupt and motion features.
