# esp32-usound-sensor
Arduino sketch for an ultrasound distance detector on an ESP32 board via a simple web get http://device-ip-address

Getting started

Ensure you have the Arduino ESP32 library downloaded and added via the zip file from here; https://github.com/espressif/arduino-esp32/tree/master


* Clone this repo
* Open this ino file in Arduino IDE or VS Code
* change the constants for sssid and password
* Setup your ESP32 board with the 
 * Device - Sensor
 * VIn to VCC 
 * D5 to Trigger
 * D18 to Echo
 * Gnd to Gnd

* Select your board and port
* Control U. 

Observe the Device IP Address printed in the serial monitor obtained from your DHCP server

In a web browser enter http://device-ip-address and go!

Test your distances as required