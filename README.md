# ESP32_ESP-IDF_VL53L0

Started with the template <https://github.com/HankB/ESP32_create-project_start> which provides suport for:

* WiFi
* SNTP
* MQTT
* DS18B20 temperature sensor

Build on this using the ESP-IDF VL53L0 driver <https://components.espressif.com/components/pkolt/vl53l0x/versions/1.0.0/readme>

## Motivation

Provide an ESP32 based solution to measuring the sump depth in my pasement.

## Status

* 2026-08-16 OTA incorporated and built, not yet tested. (VL53L0 not connected and proj_init_vl53l0x() not called.)
* 2026-08-06 WIP

## Plans

* Add OTA support as was done with <https://github.com/HankB/ESP32_create-project_start>
* Application logic.
  * Sample with a short enough period to detect changes in the water level due to sump pump operation
  * Report when level changes sufficiently or on a longer timed interval.
* Review initialization choices
* Store calibration in NVS

## Build

This effort uses the V6.0.2 version of ESP-IDF.

```text
source ~/.espressif/tools/activate_idf_v6.0.2.sh
cd ~/some/convenient/directory
git clone git@github.com:HankB/ESP32_ESP-IDF_VL53L0X.git # (For me or substitute your fork)
cd start
idf.py set-target esp32c3 # or idf.py set-target esp32
idf.py menuconfig
cp components/proj_wifi/secrets.h.example components/proj_wifi/secrets.h # fill in creds
idf.py build flash monitor # builds the project and flashes it. 
```

## Wiring

### ESP32 DevKit V1

Colors are for the DuPont jumpers on the breadboard. The ESP32 in use is an "ESP32 DevKit V1" to the best of my knowledge. The VL53L0X has pin header soldered amd protruding from the bottom edge of the board.

|ESP|color|VL53L0X|notes|
|---|---|---|---|
|3V3|orange|VIN||
|GND|yellow|GND||
|D22|green|SCL||
|D21|blue|SDA||
||purple|GPIO1|unused|
||gray|XSHUT|unused|

### D1 Mini ESP32 ESP-WROOM

Alternate H/W preparing for "deployment." This prototyping board has two rows of pins on each side and all connections (DS18B20 and VL53L0X) can be accomplished with the 1-wire interface moved to GPIO-16.

## Sensor description

* Purchased from Amazon <https://www.amazon.com/dp/B0F28MFW6X?th=1> and references the datasheet from ST which is for the sensor itself. There is some additional information on the Amazon page and copied here should that page go away:

---

### About this item

* The VL53L0X from ST Microelectronics is a time-of-flight ranging system integrated into a compact module. This board is a carrier for the VL53L0X, so we recommend careful reading of the VL53L0X datasheet (1MB pdf) before using this product.
* The VL53L0 uses ST's FlightSense technology to precisely measure how long it takes for emitted pulses of infrared laser light to reach the nearest object and be reflected back to a detector, so it can be considered a tiny, self-contained lidar system.
* Ranging measurements are available through the sensor's I⊃2;C (TWI) interface, which is also used to configure sensor settings, and the sensor provides two additional pins: a shutdown input and an interrupt output.
* The VL53L0X is a great IC, but its small, leadless, LGA package makes it difficult for the typical student or hobbyist to use. It also operates at a recommended voltage of 2.8 V, which can make interfacing difficult for microcontrollers operating at 3.3 V or 5 V. Our breakout board addresses these issues, making it easier to get started using the sensor, while keeping the overall size as small as possible.
* A time-of-flight ranging system integrated into a compact module

### Product Description

Description:

The VL53L0X from ST Microelectronics is a time-of-flight ranging system integrated into a compact module. This board is a carrier for the VL53L0X, so we recommend careful reading of the VL53L0X datasheet (1MB pdf) before using this product.

Ranging measurements are available through the sensor’s I²C (TWI) interface, which is also used to configure sensor settings, and the sensor provides two additional pins: a shutdown input and an interrupt output.

PIN Description:

VDD Regulated 2.8 V output. Almost 150 mA is available to power external components. (If you want to bypass the internal regulator, you can instead use this pin as a 2.8 V input with VIN disconnected.)

VIN This is the main 2.6 V to 5.5 V power supply connection. The SCL and SDA level shifters pull the I2C lines high to this level.

GND The ground (0 V) connection for your power supply. Your I2C control source must also share a common ground with this board.

SDA Level-shifted I²C data line: HIGH is VIN, LOW is 0 V

SCL Level-shifted I²C clock line: HIGH is VIN, LOW is 0 V

XSHUT This pin is an active-low shutdown input; the board pulls it up to VDD to enable the sensor by default. Driving this pin low puts the sensor into hardware standby. This input is not level-shifted. 

---

## Errata

* The output sense for the built in LED is reversed between the ESP32 and the ESP32-C3. That doesn't matter for simple on/off sequencing but for more involved signaling it will need to be addressed (and has been:LED_ACTIVE_LOW in start.c)
* The ESP32-C3 will occasionally have trouble associating with the AP and communicating over WiFi in general. Locating it where it has better "line of sight" to the AP seems to help.
* I have been working with an ESP32-C3 which connects to Linux as `/dev/ttyACM0` and an ESP32 DevKit V1 which connects as `/dev/ttyUSB0`
* Whenever the target is changed (`idf.py set-target [esp32|esp32c3]`) the MQTT broker URI reverts to the default.
* Apparently the way that the one wire bus (RMT) driver used for the DS18B20 works, it produces the following harmless warning:

```text
W (5798) rmt: GPIO 4 is not usable, maybe conflict with others
```
