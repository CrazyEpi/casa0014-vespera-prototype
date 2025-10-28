# casa0014-vespera-prototype
A prototype program to control lights with a light sensor and a joystick.

# Introduction

The design is inspired by a video game - Splatoon
<img width="1600" height="800" alt="image" src="https://github.com/user-attachments/assets/95e366d0-c163-44cd-89b4-6175559a1503" />
This is a TPS(Third Person Shooter) game, performing a 5v5 match, where victory is not achieved by regular kills or scores, but painting the largest area of the map. Thus, this prototype is designed to creat a comforting ambient light, determined with the light sensor, while users can press down the joystick to "paint" the Vespera light.

Specifically, there are two modes of this prototype, auto and manual, the color for both modes are determined by the enviroment lighting. 
Auto mode is the default mode, which will be engaged after the arduino is powered. It would light two LEDs at a time, perform a flowing light through Vespera. 
<<<<<Auto Mode Figure>>>>>>

When the joystick is pressed, the manual mode will be engaged. The current light will stop flowing, and user will be able to control one of the LED pixel with the joystick. The color of the selected LED is still determined by the ambient light. Thus, if the ambient light is changing, the user will be able to record the ambient light at the moment to creat a light map.
<<<<paint figure>>>>>

# Methodology
## Device Setup
<img width="1036" height="703" alt="QQ_1761614025490" src="https://github.com/user-attachments/assets/6a901b52-60c1-4ecb-85f0-67b703361cf4" />
An Arduino MRK WiFi 1010, a VEML7700 light sensor, and a joystick module is used in this prototype. The schematic diagram is shown as the figure above. 
