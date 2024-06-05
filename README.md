# Simple 1-Wire Arduino library for the ESP32
Arduino Library for ESP32 DS18B20 Maxim Integrated "1-Wire" protocol.

The original implementation of OneWire library used CPU delays to construct the 1-Wire read/write timeslots however this proved to be too unreliable.
It is based on the ESP's RMT peripheral. Whilst originally designed as an IR tranceiver it provides a good general purpose hardware bitstream encoder/decoder.

Using the ESP32's RMT peripheral, results in very accurate read/write timeslots and more reliable operation.

Version 1.0.7 is for Arduino 2.x based on ESP-IDF 4.X
Version 2.0.x is For Arduino 3.0.0 based on ESP-IDF 5.x

The example is provided and tested using Arduino 2.0.6 based on ESP-IDF 4.4.3

_1-Wire(R) is a trademark of Maxim Integrated Products, Inc._
---

<a href="https://www.buymeacoffee.com/htmltiger"><img src="https://www.buymeacoffee.com/assets/img/custom_images/white_img.png" alt="Buy Me A Coffee"></a>
