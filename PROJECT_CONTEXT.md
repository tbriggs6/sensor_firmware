# Project Context

## Purpose
This is the firmware for a multi-mode water sensor.  The firmware will be installed
in a deeply embedded system that is built using the Contiki-NG OS (https://www.contiki-ng.org/).  
The sensors form a 6LOPAN mesh to manage communcations to an edge node.  There are three types of 
devices represented: waterNode, airborneNode, and spinet.  The waterNode is the firmware for the
water sensor, the airborneNode is the firmware for a sensor that is is used to sample the area but
also as a range extender for the mesh, and finally the spinet firmware is used to create an edge
node that can be attached to a device like a raspberry pi to bridge between the wireless 6LOPAN
mesh and the internet.

### Water Sensor
The water sensor includes the following sensors:
* MS5637 pressure sensor to measure air pressure inside the water sensor capsule
* SI7210 high-precision hall effect sensor
* TCS3472 - color sensor

There is a "daylight" equivalent LED that is driven to capture reflected water color 
and turbidity using the TCS3472.  The code takes two readings from the TCS3472: with the daylight led on (transflectance) and with it off for ambient.  The hardware has an analog
differential amplifier and wheatstone bridge to measure conductivity of the water which is ready by the ADC built into the microcontroller.  Finally, the most complex measure is the
derived from the hall sensor.  There is a spring loaded plunger and diaphragm that has water pressure on the outside.  Increased pressure pushes the diaphragm which pushes a magnetic plunger that is read from the Hall effect sensor to detect depth and pressure changes in the water.  The internal chamber pressure sensor is used to calibrate the relative pressure in the chamber versus the water pressure outside to establish the depth of the water above the sensor.

### Airborne Sensor
The airborne sensor includes a SI7020 to measure humidity, dew-point, and temperature.  

### Spinet
The spinet device does not use sensors, but is only an edge router.



## Domain Constraints
- Security: 6LOPAN mesh security
- Performance: long battery life, reliable run-time.

## Architecture
- Style: microservice under Contiki-NG
- Major modules: command service, config service, echo service, messenger service, and sensors

## Tech Stack
- Language: C++
- Frameworks: Contiki-NG
- Build: TI Code Composer Studio

## Quality Bar
- Test coverage target: 80%
- Performance targets: Idle time > 80%
- Latency/SLA: Responds to commands in less than 2 seconds
- Uptime: > 12 weeks

## Design Rules
- No static mutable state
- No reflection
- No blocking in async paths
- etc.

