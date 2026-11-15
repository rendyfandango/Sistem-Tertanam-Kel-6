# Source inspection and simplification — initial increment

Reviewed 2026-11-15. This is a scoped analysis, not a claim of a full library audit.

## Sources actually inspected

- InvenSense **RM-MPU-6000A-00**, revision 4.2, register table and section 4.32
  (WHO_AM_I), from a [mirror of the manufacturer's document](https://uelectronics.com/wp-content/uploads/2018/02/MPU-6000-Register-Map1.pdf).
- Jeff Rowberg's [I2Cdevlib MPU6050.cpp](https://github.com/jrowberg/i2cdevlib/blob/master/Arduino/MPU6050/MPU6050.cpp):
  license header, `initialize()`, `testConnection()`, sample rate documentation,
  `getDeviceID()`, and FIFO/DMP-related sections. The linked `master` is mutable;
  this report records the inspected function names, not a pinned upstream version.
- Installed ESP-IDF v6.1 `components/esp_driver_i2c/include/driver/i2c_master.h`
  and the [Espressif I2C documentation](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/peripherals/i2c.html)
  for the modern bus/device and combined-transfer model. Installed headers and
  actual v6.1 compilation determine API compatibility for this project.

The manufacturer's product-specification PDF links were attempted but could not
be retrieved. A full product-specification review and detailed measurement
register analysis remain prerequisites for later phases. The register map was
retrieved successfully; these are separate documents.

## Findings relevant to this increment

I2Cdevlib's connection test compares an extracted six-bit identity with 0x34.
Its getter reads bits 6:1, whereas our new C driver compares the full register
byte with 0x68. Confusing these representations would incorrectly reject a
working device. Changing AD0 affects the slave address, not WHO_AM_I.

I2Cdevlib initialization selects the X gyro PLL, sensitive ranges and wakes the
device. Those measurement operations are reserved for Phase 3 here. FIFO and
DMP support add APIs and state that the direct polling TiltGuard design does
not require. Our transport is ESP-IDF native, with explicit error propagation
and an externally owned bus rather than Arduino/I2Cdev objects.

## Feature decisions

| Feature | Decision | Reason/status |
|---|---|---|
| I2C transport, identity, device lifetime | Rewritten in native C, implemented | Shared ESP-IDF bus and bounded errors |
| Register read | Implemented privately | Minimal operation needed for identity |
| Register write and masked update | Retain in Phase 3 | Preserve unrelated control bits |
| Wake/clock, ranges, DLPF, divider | Retain in Phase 3 | Required sensing setup |
| Burst read and unit conversion | Retain in Phase 4 | Coherent raw sample, range-aware scaling |
| Stationary offsets and motion rejection | Retain in Phase 5 | Project-specific calibration |
| Complementary filter | New separate C component in Phase 6 | Keep signal processing out of transport |
| DMP, FIFO, quaternion, auxiliary master | Exclude from planned driver | Unnecessary for 100 Hz polling roll/pitch |
| External sensor control, advanced interrupts, hardware motion features | Exclude | Other subsystems or unused features |
| Unused register constants | Exclude | Only WHO_AM_I defined in this increment |

These exclusions describe our new driver scope; no existing library files were
deleted or bundled. Future retained features are not yet implemented.

## Attribution and license

I2Cdevlib is credited to Jeff Rowberg, copyright 2012, under MIT in the inspected
source. It was studied as a reference; no source code was copied or linked into
this project. The C implementation here was written for the ESP-IDF API. If a
later increment copies substantial upstream code, retain its copyright and full
MIT permission notice with that code. The register map belongs to InvenSense;
the PDF itself is linked, not redistributed.
