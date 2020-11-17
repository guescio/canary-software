# Canary software

This is the software to operate a Canary.

## scripts
- **PCPost.py**: use a PC to read the serial output from the Canary and post the data to an MQTT server. This is useful when the Canary cannot access a wi-fi connection.

## sleepMeasurePostRepeat
This is the Arduino sketch to be flashed on the Canary. To configure the Canary edit `config.h`. Do not edit the rest of the code.
