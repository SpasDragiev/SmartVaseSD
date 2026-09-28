# SmartVase (Arduino + SD card version)

A plant monitoring system built on an Arduino. It measures five things about the plant's environment, shows friendly messages on a 16x2 LCD, and logs every reading to an SD card as CSV.

This is the first version of the project. A newer ESP32 version with a live web dashboard also exists (see [Related](#related)).

## Features

- Reads temperature, humidity, CO₂, soil moisture and soil pH
- Cycles through 5 LCD screens, each with a short message about the plant's condition (for example "Plant is happy!" or "Water me NOW!!!")
- Logs all readings to `data.txt` on the SD card every 5 seconds
- Prints readings to the Serial Monitor at 9600 baud
- pH filtering: 10 samples are sorted and the middle 6 are averaged
- Powers the soil sensor only while measuring, which reduces corrosion

## Hardware

- Arduino Uno (or compatible)
- SCD40 CO₂ / temperature / humidity sensor
- DHT11 temperature and humidity sensor (initialised in the code, not used for readings)
- Capacitive soil moisture sensor
- Analog pH sensor with signal board
- 16x2 LCD with I2C backpack (address `0x27`)
- SD card module
- Jumper wires and breadboard

## Wiring

| Component | Component pin | Arduino pin |
|---|---|---|
| DHT11 | Data | **D2** |
| DHT11 | VCC / GND | 5V / GND |
| Soil moisture sensor | Signal (AOUT) | **A0** |
| Soil moisture sensor | VCC | **D7** (powered from a pin, so it is only on while measuring) |
| Soil moisture sensor | GND | GND |
| pH sensor | Signal (PO) | **A1** |
| pH sensor | VCC / GND | 5V / GND |
| SD card module | CS | **D10** |
| SD card module | MOSI | D11 |
| SD card module | MISO | D12 |
| SD card module | SCK | D13 |
| SD card module | VCC / GND | 5V / GND |
| SCD40 | SDA | A4 (SDA) |
| SCD40 | SCL | A5 (SCL) |
| SCD40 | VCC / GND | 3.3V / GND |
| LCD (I2C) | SDA | A4 (SDA) |
| LCD (I2C) | SCL | A5 (SCL) |
| LCD (I2C) | VCC / GND | 5V / GND |

The LCD (`0x27`) and the SCD40 (`0x62`) share the same two I2C wires without conflict.

> [!NOTE]
> Check that your SD card module accepts 5V. Most modules with a voltage regulator do; bare 3.3V modules need a level shifter.

## Required libraries

Install these from **Sketch → Include Library → Manage Libraries** in the Arduino IDE. Search for the exact names below.

| Library | Author | Notes |
|---|---|---|
| `7semi_SCD40` | 7semi | SCD40 sensor driver |
| `DHT sensor library` | Adafruit | DHT11 driver |
| `Adafruit Unified Sensor` | Adafruit | Required by the DHT library (the IDE will offer to install it) |
| `LiquidCrystal I2C` | Frank de Brabander | LCD driver |
| `Wire` | Built in | I2C |
| `SPI` | Built in | SD card bus |
| `SD` | Built in | SD card logging |

Includes used in the sketch:

```cpp
#include <Wire.h>
#include <7semi_SCD40.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
```

## Setup

1. Wire the components as in the table above.
2. Install the libraries.
3. Open the `.ino` file in the Arduino IDE.
4. Select your board and port, then upload.
5. Insert a FAT-formatted SD card **before** powering on. If it isn't found, the LCD shows `SD ERROR` and the program stops.

## Calibration

Edit these values at the top of the sketch.

**Soil moisture**

```cpp
#define DRY_VAL 465   // raw reading with the sensor in dry air
#define WET_VAL 280   // raw reading with the sensor in water
```

Open the Serial Monitor, read the `Soil raw` value in each condition, and put your numbers in.

**pH**

```cpp
#define PH_NEUTRAL_VOLTAGE 2.50   // voltage in pH 7.0 buffer solution
#define PH_ACID_VOLTAGE    2.03   // voltage in pH 4.0 buffer solution
```

Use buffer solutions, note the sensor voltage in each, and update the two constants.

## Data format

`data.txt` is a CSV file with this header:

```
TempC,Humidity,CO2,Soil,SoilPct,pH
```

It can be opened directly in Excel, Google Sheets or Python (pandas).

## Notes

- The LCD messages are intentionally playful, and the thresholds can be changed in `showScreen()`.
- Screen change speed and reading interval are set by `SCREEN_DURATION` and `READ_INTERVAL`.

## Related

An improved ESP32 version with WiFi and a live web dashboard is available in a separate repository.

## License

MIT
