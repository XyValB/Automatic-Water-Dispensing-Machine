# Automatic Water Dispensing Machine

An ESP32-based automatic water dispensing machine developed as an electronics technology project.

## Overview

The Automatic Water Dispensing Machine is designed to dispense water through a peristaltic pump controlled by an ESP32 microcontroller. It uses an OLED display and push buttons to provide a simple interface for selecting dispensing options and operating the system.

The system also includes an ultrasonic sensor for level sensing and LED indicators to display its operating status.

## Features

* ESP32 microcontroller
* OLED display menu
* Push-button controls
* Preset dispensing mode
* Ultrasonic level-sensing mode
* Peristaltic pump control
* LED status indicators

## Hardware Requirements

* ESP32 DevKit development board
* 1.3-inch 128×64 I2C OLED display (SH1106)
* INTLLAB 12V DC peristaltic pump
* L298N motor driver module
* HC-SR04 ultrasonic sensor
* Four push buttons
* Two LEDs
* 12V power supply
* Jumper wires and other necessary connections

## Software Requirements

* Arduino IDE
* ESP32 board package
* Wire library
* U8g2 graphics library

## Installation

1. Install the Arduino IDE.
2. Install ESP32 board support through the Arduino Boards Manager.
3. Install the U8g2 library through the Arduino Library Manager.
4. Download or clone this repository.
5. Open `automatic_water_dispensing_machine.ino` in the Arduino IDE.
6. Select the appropriate ESP32 board and COM port.
7. Connect the components according to the wiring documentation.
8. Upload the sketch to the ESP32.

## Operation

1. Power on the system.
2. Use the push buttons to navigate the OLED menu.
3. Select the desired dispensing mode.
4. Follow the instructions displayed on the OLED.
5. Observe the LED indicators and system status during operation.

## Project Files

* `automatic_water_dispensing_machine.ino` — main Arduino source code
* `COMPONENTS.md` — component list and pin assignments
* `TESTING.md` — testing and calibration documentation

## Safety Notice

Use an appropriately rated power supply for the pump and motor driver. Verify all wiring before powering on the system. This project is intended for educational prototyping.

## License

This project is intended for educational purposes.
