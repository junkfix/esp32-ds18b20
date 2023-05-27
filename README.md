# Simple 1-Wire Arduino library for the ESP32
Arduino Library for ESP32 DS18B20 Maxim Integrated "1-Wire" protocol.

The original implementation of OneWire library used CPU delays to construct the 1-Wire read/write timeslots however this proved to be too unreliable.
It is based on the ESP's RMT peripheral. Whilst originally designed as an IR tranceiver it provides a good general purpose hardware bitstream encoder/decoder.

Using the ESP32's RMT peripheral, results in very accurate read/write timeslots and more reliable operation.

_1-Wire(R) is a trademark of Maxim Integrated Products, Inc._
