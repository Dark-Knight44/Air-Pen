# ESP32 Air Pen

A wireless, motion-controlled input device built using an ESP32 and an MPU motion sensor. The Air Pen allows users to control a computer cursor through hand movements without using a traditional mouse.

The device uses gyroscope data to detect rotational movements, processes the readings on the ESP32, and translates them into cursor movements. Mouse commands are transmitted to a connected device using Bluetooth HID.

## Overview

The ESP32 Air Pen is designed as a compact and low-cost alternative to a traditional computer mouse. Instead of relying on a physical surface, it uses hand movements to control cursor navigation.

A gyroscope tracks rotational movement along different axes, while a physical button provides mouse-click and drag functionality.

## Hardware Requirements

| Component | Description |
|---|---|
| ESP32 DevKit (ESP32-WROOM) | Main microcontroller |
| MPU6050 / MPU6500 | Motion sensing |
| Push Button | Mouse click and drag control |
| 18650 Li-ion Battery | Portable power supply (planned) |
| Charging Module | Battery charging (planned) |
| Voltage Converter | Regulated power supply (planned) |

## Software and Technologies

- C++
- Arduino Framework
- PlatformIO
- Visual Studio Code
- ESP32 BLE Mouse Library
- Arduino Wire Library (I2C)

## Working Principle

1. The MPU motion sensor measures rotational movement using its gyroscope.
2. The ESP32 reads gyroscope data through the I2C interface.
3. Sensor readings are calibrated to compensate for the gyroscope's resting offset.
4. A filtering algorithm reduces unwanted sensor noise.
5. Rotational movement is converted into horizontal and vertical cursor movement.
6. Bluetooth HID transmits mouse commands to the connected device.
7. A physical push button provides left-click and click-and-drag functionality.

## Hardware Connections

### MPU Motion Sensor

| Sensor Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Push Button

| Button Connection | ESP32 Pin |
|---|---|
| Terminal 1 | GPIO 19 |
| Terminal 2 | GND |

The push button uses the ESP32's internal pull-up resistor.

## Project Structure

```text
ESP32-Air-Pen/
│
├── include/
│   └── README.md
│
├── lib/
│   └── README.md
│
├── src/
│   └── main.cpp
│
├── test/
│   └── README.md
│
├── platformio.ini
├── .gitignore
└── README.md
```

## Getting Started

### Prerequisites

- Visual Studio Code
- PlatformIO IDE extension
- ESP32 development board
- MPU6050 or compatible MPU motion sensor
- USB data cable

### Installation

Clone the repository:

```bash
git clone https://github.com/Dark-Knight44/Air-Pen.git
```

Open the project directory in Visual Studio Code.

Build the firmware:

```bash
pio run
```

Upload the firmware:

```bash
pio run --target upload
```

Open the serial monitor:

```bash
pio device monitor
```

Make sure the ESP32 and motion sensor are connected correctly before uploading.

## Configuration

The main configuration parameters are defined in `src/main.cpp`.

| Parameter | Description |
|---|---|
| SENSITIVITY | Controls cursor movement speed |
| DEADZONE | Filters small unwanted movements |
| FILTER_ALPHA | Controls motion filtering |
| X_DIRECTION | Adjusts horizontal direction |
| Y_DIRECTION | Adjusts vertical direction |
| BUTTON_PIN | Defines the mouse button GPIO |

These values can be modified to adjust cursor behaviour according to the sensor orientation and user preference.

## Bluetooth Connectivity

The ESP32 operates as a Bluetooth HID mouse.

**Bluetooth device name:** `ESP32 Air Pen`

Pair the device with a Bluetooth-compatible computer or operating system that supports Bluetooth HID mouse devices.

## Power Supply

The device currently receives power through the ESP32 USB connection.

A rechargeable Li-ion battery-based power supply is intended for portable operation. The battery circuit will use a suitable charging and protection module along with a regulated voltage converter.

## Testing

Hardware testing involves checking:

- I2C communication with the motion sensor.
- Gyroscope calibration and readings.
- Cursor movement and direction.
- Bluetooth connectivity.
- Physical button operation.

Automated tests can be added through the PlatformIO testing framework.

