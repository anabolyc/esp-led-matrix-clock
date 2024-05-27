# ESP8266 LED Matrix Clock

![image](https://user-images.githubusercontent.com/5459747/208926467-28fd283d-614e-4183-b4f9-c0008e6ef104.png)

Are required:
- 4 or 6 MAX7219 incl 8x LED-Matrix8x8
- one ESP8266 or ESP32 Board

Define GPIOs, Brightness, and Timezones in the platformio.ini.

Wi-Fi credentials were recently replaced with [WiFiManager](https://github.com/tzapu/WiFiManager), so there are no hardcoded passwords anymore. When run for the first time (or no known networks are present), configure via captive portal, then settings are saved and ESP will connect automatically.

Added support for ESP32-C3, tested on [Lolin C3 mini](https://www.wemos.cc/en/latest/c3/c3_mini.html), and works perfectly fine.

Brief description:

During setup, the current time is obtained from an NTP via WLAN. If the clock is in operation, this process is repeated daily 

## Using with ESPHome / Home Assistant

Alternative to native firmware, the clock can be used together with Home Assistant via ESPHome. Although it is not a standalone solution, it has some additional features:
- It shows the weather along with the time and date
- It can show any text you like whenever it is pushed from the HA
- It allows easy remote firmware flashing

Configuration is [here](/firmware/esphome/led-matrix-clock.yaml). You'll need to upload 2 included font files into the HA `esphome/fonts` folder. 

Looks like this

|  Clock | Date | Weather
|--------|------|--------|
| ![image](https://github.com/anabolyc/esp-led-matrix-clock/assets/5459747/c1657cad-6daa-45b3-9455-61eb7d24e4b4) | ![image](https://github.com/anabolyc/esp-led-matrix-clock/assets/5459747/7dffc653-080b-4de9-b979-49b4e442ede8) | ![image](https://github.com/anabolyc/esp-led-matrix-clock/assets/5459747/08908d78-97e3-4c19-bd22-608cc8843c30)

## Where to buy

Available as Kit on [Tindie](https://www.tindie.com/products/sonocotta/esp8266-led-matrix-clock-diy-kit/). Assembly instructions can be found [here](https://hackaday.io/project/188093/instructions)

![image](https://user-images.githubusercontent.com/5459747/208926760-a6d0adaa-ce00-4d79-8fd4-97c3cfef58a9.png)

