#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_dir="$(mktemp -d /tmp/tiltguard-host.XXXXXX)"
cc -std=c11 -Wall -Wextra -Werror -pedantic \
    -Itests/stubs -Icomponents/mpu6050/include \
    components/mpu6050/mpu6050.c tests/test_mpu6050.c -o "$test_dir/test_mpu6050"
"$test_dir/test_mpu6050"
