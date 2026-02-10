# Firmware tests

- **baseline/** – Host tests for message/config compatibility (e.g. struct layout so the backend can parse Zephyr packets). Run: `make -C baseline` then `./baseline/message_struct_test`.
- **zephyr/** – Zephyr build and validation; see [zephyr/README.md](zephyr/README.md).
- **ip6addr/** – Legacy IPv6 address helper test.
- **server/** – Legacy server test.
