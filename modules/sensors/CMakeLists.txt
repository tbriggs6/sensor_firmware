# Zephyr sensor drivers (_z). Used when building app with west; Contiki uses Makefile.sensors.

zephyr_library()
zephyr_library_sources(
  ms5637_z.c
  si7210_z.c
  tcs3472_z.c
  si7020_z.c
  analog_z.c
  vaux_z.c
  daylight_z.c
)
zephyr_library_include_directories(.)
