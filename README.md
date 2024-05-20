# SIT210-Task5.1PGUI
 S306 SIT210 Task 3.3D, detect a wave and a pat using an ultrasonic sensor

## Description
A simple sensor with some (basic and probably janky) signal processing to detect and react to waves and pats.

A wave is considered as from zero to many swipes through the sensor range.
A pat is considered as the distance decreasing and then increasing multiple times.
A breakdown is considered as my existance

When an action is detected (either a wave or a pat), a message is sent to the MQTT broker.
When an MQTT message is recieved, the LED blinks, a message is sent to the serial console.
The LED blink pattern varies based on the message.

## How it Works
Due to my ultrasonic sensor being not the greatest quality, I have utilised an ESP8266 based D1 mini clone for this task.

The rest of the How It Works section will be fixed once MQTT is fully functional.

## Usage
Requires the following libraries:
* ESP8266WiFi
* PubSubClient

## MQTT Implementation Help
A fantastic reference which I used to help develop the MQTT component of this task: [Using MQTT on ESP8266: A Quick Start Guide | EMQ](https://www.emqx.com/en/blog/esp8266-connects-to-the-public-mqtt-broker)