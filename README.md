# pressure_logger

Use a low-cost pressure sensor to measure pressure in a drainfield lateral and log the data to a SD card.

## Designs

* [Version 1](docs/v1.md)
* [Version 2](docs/v2.md)
* [Version 3](docs/v3.md)

## Setup (Ubuntu 24.04)

Run the following commands to install the required tools and dependencies:

1. Install `arduino-cli`:
   ```bash
   sudo snap install arduino-cli
   ```

2. Install the AVR core (required for compiling for the Adafruit Metro 328 / Arduino Uno):
   ```bash
   arduino-cli core install arduino:avr
   ```

3. Install the required libraries used by the sketch:
   ```bash
   arduino-cli lib install "RTClib" "SD" "Low-Power"
   ```

## Build Instructions

To compile the sketch after making any changes, run:
```bash
arduino-cli compile --fqbn arduino:avr:uno src/pressure_logger/pressure_logger.ino
```
