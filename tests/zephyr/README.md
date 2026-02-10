# Zephyr tests

This directory documents how to validate the Zephyr migration (apps and modules).

## Build and run (manual)

From a Zephyr workspace that includes this repo (or with `ZEPHYR_BASE` set and this repo as the application):

```bash
# Build each app for CC13xx (e.g. ti_cc1352r1)
west build -b ti_cc1352r1 app/water_node
west build -b ti_cc1352r1 app/airborne_node
west build -b ti_cc1352r1 app/spinet_node
```

For build-only validation without hardware (e.g. QEMU or native_sim if the board is supported):

```bash
west build -b qemu_cortex_m3 app/water_node   # if supported
# or
west build -b native_sim app/water_node      # if supported
```

## Message/config compatibility (host test)

From the repo root (requires `gcc` and `make`):

```bash
make -C tests/baseline
./tests/baseline/message_struct_test
```

This checks that `message_z.h` struct layout (sizes, offsets, headers) matches expectations so that the existing backend can parse Zephyr-originated packets unchanged.

## Twister (optional)

A build-only test layout is in `tests/zephyr/build_apps/`. If this repo is used as a Zephyr application in a workspace with twister, configure the test to build the apps (e.g. set the sample path to `app/water_node` for your workspace root) and run:

```bash
west twister -T tests/zephyr/build_apps -i
```

The manual `west build` steps above are the primary way to validate.
