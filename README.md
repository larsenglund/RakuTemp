# RakuTemp
WiFi enabled (ESP8266) thermocouple (MAX31855) (for Raku ovens or anything else). Draws around 60mA so it can easily be powered from a cheap usb power bank for the duration of a Raku burn.

## Software
Uses the CaptivePortalAdvanced and httpUpdater example that comes with the Arduino core for the ESP8266 for handling WiFi and OTA firmware updates. Websockets and Chart.js is used to display a live graph of the temperature read from a K-type thermocouple (-200 to 1350 degrees celsius) with the Adafruit MAX31855 library.

Mobile screenshot:

<a href="http://i66.tinypic.com/2815t1t.png"><img src="http://i66.tinypic.com/2815t1t.png" width="200" ></a>


## Hardware
+ WeMos D1 mini: https://www.aliexpress.com/item/D1-mini-V2-Mini-NodeMcu-4M-bytes-Lua-WIFI-Internet-of-Things-development-board-based-ESP8266/32681374223.html
+ MAX31855 breakout: https://www.aliexpress.com/item/Hot-Sale-High-Quality-MAX31855-K-Type-Thermocouple-Breakout-Board-Temperature-1350-Celsius-for-3V-5V/32647767866.html
+ K-type thermocouple: https://www.aliexpress.com/item/Newest-Style-Best-Price-High-Temperature-100-To-1250-Degree-Thermocouple-K-Type-100mm-Stainless-Steel/32635321141.html
+ Enslosure: https://www.aliexpress.com/item/J34-Free-Shipping-Waterproof-Plastic-Electronic-Project-Box-Case-Enclosure-2-56-L-x-2-28/32579201101.html
+ Some wires & hot glue :)
