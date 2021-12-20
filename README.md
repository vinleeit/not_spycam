# Attendance Machine Project

## Brief Introduction

An Arduino project for attendance machine. The attendance machine runs face detection locally (not face recognition) which will be done after it detects a person, who is standing in front of the machine, by using sensors. The machine will then capture a JPEG image after a successful detection. Finally, the image will be sent to the server for image recognition.

This work is originated from ESP32-Cam example from ESP32 board version 1.0.4. However, the face detection and face recognition provided in the example can not run without establishing any local server. Thus, with the help of the tutorial from [ESP32 camera: face detection - techtutorialsx](https://techtutorialsx.com/2020/06/13/esp32-camera-face-detection/), the machine can now detect faces offline.

*This project started in December 2021.*

## Specification

| Specification    | Detail                      | Version |
| ---------------- | --------------------------- | ------- |
| Device           | ESP32-Cam FOCE (AI-Thinker) |         |
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
