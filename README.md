# Use Lilygo T-Display to show ThermoBeacon measurements

```
Copyright (c) 2024 Eugene Crosser

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must
    not claim that you wrote the original software. If you use this
    software in a product, an acknowledgment in the product documentation
    would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must
    not be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
```

------------------------------------------------------------------------

![Photo of the display in action](photo.jpg)


T-Display is an inexpensive dev module based on ESP32 LX6 with a 135x240
TFT LCD display. It has built-in radio (WiFi & Bluetooth, incl. BLE),
can be powered from 3.3 or 5 V, and programmed over USB using Arduino
tools and libraries.

This project turns it into an autonomous temperature and humidity display
that collects data from up to two ThermoBeacon BLE sensors. BLE scanner
is passive, it only listens to the advertisement frames sent by the sensors.
If more than two sensors are present, measurements from the first two,
based on the RSSI value, are displayed, (that means, presumably, the two
sensos in the closest proximity to the display module). Supported sensors
are detected by the first two octets of the MAC address, no configuration
should be necessary.

One of the buttons (GPIO 35) toggles between graphic display of battery
charge and signal strength, and display of the last three octets of the
MAC address.

Depnedencies: `ArduinoBLE` library and `Bodmer/TFT_eSPI`.

## Homepage and source

Home page is [http://www.average.org/ThermoBeaconDisplay/](http://www.average.org/ThermoBeaconDisplay/)
Get the source from the origin `git://git.average.org/ThermoBeaconDisplay.git`
or from [Github mirror](https://github.com/crosser/ThermoBeaconDisplay).
