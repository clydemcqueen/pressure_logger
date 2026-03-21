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
   curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
   ```

2. Install the AVR core (required for compiling for the Adafruit Metro 328 / Arduino Uno):
   ```bash
   arduino-cli core install arduino:avr
   ```

3. Install the required libraries used by the sketch:
   ```bash
   arduino-cli lib install "RTClib" "SD" "Low-Power"
   ```

## Build and Upload

To compile and upload the sketch after making any changes, run:
```bash
cd src/v1_field
arduino-cli compile --upload -p /dev/ttyUSB0
```

To listen to the serial port:
```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```
