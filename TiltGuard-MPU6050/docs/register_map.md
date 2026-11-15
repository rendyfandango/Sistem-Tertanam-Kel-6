# Registers and transaction sequence

Phase 2 implements only `WHO_AM_I`. The remaining rows are the agreed plan for
later increments; values below are targets, not hardware readbacks.

| Register | Address | Relevant fields and planned use |
|---|---|---|
| WHO_AM_I | 0x75 | Read-only identity bits 6:1; full byte expected 0x68 |
| PWR_MGMT_1 | 0x6B | Bit7 reset, bit6 sleep, bit5 cycle, bit3 temperature disable, bits2:0 clock; target 0x01 |
| PWR_MGMT_2 | 0x6C | Bits7:6 low-power wake rate; bits5:3 accel standby, bits2:0 gyro standby; target active axes |
| SMPLRT_DIV | 0x19 | Bits7:0 sample divider; target 0x09 |
| CONFIG | 0x1A | Bits5:3 external sync, bits2:0 DLPF; target 0x03 |
| GYRO_CONFIG | 0x1B | Bits7:5 self-test, bits4:3 FS_SEL; target 0x00 (±250 dps) |
| ACCEL_CONFIG | 0x1C | Bits7:5 self-test, bits4:3 AFS_SEL; target 0x00 (±2g) |
| ACCEL_XOUT_H | 0x3B | First of six accel bytes and start of planned 14-byte burst |
| TEMP_OUT_H | 0x41 | Two signed temperature bytes in the burst |
| GYRO_XOUT_H | 0x43 | First of six gyro bytes in the burst |

For the planned DLPF setting 3, use a 1 kHz gyro base rate, so divider 9 gives
1000/(1+9) = 100 Hz. This sensor rate is separate from I2C's 400 kHz bit rate,
the RTOS's 1000 Hz tick, and this increment's one-second identity retry.

The full identity byte 0x68 has reserved bits7 and 0 equal to zero. Extracting
bits6:1 would produce `(value & 0x7eU) >> 1` = 0x34. The current implementation
intentionally compares the complete byte instead. AD0 high changes the device
address to 0x69, but the expected identity stays 0x68.

## Actual Phase 2 I2C operation

`i2c_master_transmit_receive()` sends the register-pointer byte and receives one
byte without an intervening STOP:

```text
START → 0x68 + write → ACK → 0x75 → ACK
      → repeated START → 0x68 + read → ACK → identity → master NACK → STOP
```

On the wire, the address/RW bytes are 0xD0 and 0xD1 for address 0x68; pass the
unshifted 0x68 to ESP-IDF. ACK is the receiver pulling SDA low on the ninth
clock. The master's final NACK terminates the one-byte read normally; it is
different from a missing slave ACK. No writes to the contents of WHO_AM_I occur.
Writing 0x75 sets the chip's register pointer, which is part of reading it.

## Planned read-modify-write lesson (Phase 3, not implemented yet)

To change the range field while preserving other bits, read the register and
form `new = (old & (uint8_t)~mask) | ((range << shift) & mask)`, then write and
verify. AND clears/selects bits, NOT inverts the mask, shifting positions the
field, and OR inserts its new value. For FS_SEL/AFS_SEL, shift=3 and mask=0x18.
Explicit unsigned fixed-width types and casts keep byte operations clear after
C integer promotion. Validate `range` before shifting. Serialize the whole
read-modify-write if multiple tasks can configure this sensor.

Reference: InvenSense [register map revision 4.2](https://uelectronics.com/wp-content/uploads/2018/02/MPU-6000-Register-Map1.pdf).
