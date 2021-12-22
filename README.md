# Attendance Machine Project

## Brief Introduction

An Arduino project for attendance machine.

The attendance machine has a motion sensor (or any other movement-related sensors) for detecting object in front of the device. If there is an object, the camera will then turn on and try to use face detection to detect the object whether it is a human. If the camera detects a face, it will capture the image of the human in JPEG format.

There are 2 versions for the recognition:

1. Images will be sent to the server for face recognition. 

2. Images will be face-recognized locally in the machine.

For now, the image recognition can be done locally in the machine. The device is designed as shown in the illustration below. The button is connected to `GND` and `GPIO13`. As of `GPIO0` will be connected to `GND` to get to flashing mode, this connection must be disconnected in order to run the ESP32 normally.

<img src="assets/images/esp32.jpg" title="" alt="" data-align="center">

As for the TTL, the port can be described as such:

| Index | TTL | ESP32 |
| ----- | --- | ----- |
| 1     | 5V  | 5V    |
| 2     | GND | GND   |
| 3     | TX  | U0R   |
| 4     | RX  | U0T   |

This work is originated from ESP32-Cam example from ESP32 board version 1.0.4. However, the face detection and face recognition provided in the example can not run without establishing any local server. Thus, with the help of the tutorial from [ESP32 camera: face detection - techtutorialsx](https://techtutorialsx.com/2020/06/13/esp32-camera-face-detection/), the machine can now detect and recognize faces offline.

*This project started in December 2021.*

## Specification

| Specification    | Detail                      | Version |
| ---------------- | --------------------------- | ------- |
| Device           | ESP32-Cam FOCE (AI-Thinker) |         |
| Camera           | OV2640                      |         |
| ESP32 Board      | (*For ArduinoIDE*)          | 1.0.4   |
| Platform IO Core |                             | 5.2.4   |
| Platform IO Home |                             | 3.4.0   |

## Main Sources

- [GitHub - espressif/esp32-camera](https://github.com/espressif/esp32-camera)

- [ESP32 camera: face detection - techtutorialsx](https://techtutorialsx.com/2020/06/13/esp32-camera-face-detection/)

## Other Helpful Sources

- [face recognition without web browser](https://rntlab.com/question/face-recognition-without-web-browser/)

- [Editing Camera Web Server HTML Source Code for the ESP32-CAM - YouTube](https://www.youtube.com/watch?v=bIJoVyjTf7g)

- [Accessing ESP32-CAM Video Streaming from anywhere in the ...](https://www.elementzonline.com/blog/Accessing-ESP32-CAM-Video-Streaming-from-anywhere-in-the-world)

- [ESP32-CAM Face Recognition for Home Automation &ndash; Robot Zero OneRobot Zero One](https://robotzero.one/esp32-face-door-entry/)
