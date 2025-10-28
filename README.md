# casa0014-vespera-prototype
A prototype program to control lights with a light sensor and a joystick.

# Introduction and Inspiration

The design is inspired by a video game - Splatoon, hopefully, it would creat a cozy vibe, helping people understand, and interact with the environment.

<img width="800" height="400" alt="image" src="https://github.com/user-attachments/assets/95e366d0-c163-44cd-89b4-6175559a1503" />

Splatoon is a TPS(Third Person Shooter) game, performing a 5v5 match, where victory is not achieved by regular kills or scores, but painting the largest area of the map. Originally it is only a ambient light controlled by enviroment lighting. After exchanging our ideas in L6, I think it would be great to allow users to interact with the Vespera, and the LED pixels in Vespera are just like a canvas. Thus, a paintable Vespera is made. Specifically, there are two modes of this prototype, auto and manual, the color for both modes are determined by the enviroment lighting. 

# Methodology
## Device Setup
<img width="1036" height="703" alt="QQ_1761614025490" src="https://github.com/user-attachments/assets/6a901b52-60c1-4ecb-85f0-67b703361cf4" />

Board: Arduino MKR WiFi 1010

Ambient Light Sensor: VEML7700

·VCC → 3V3

·GND → GND

·SDA → SDA

·SCL → SCL

Joystick module:

·VRx → A1

·VRy → A2

·SW → D2

·5V Input

## Overall Design
A VEML7700 ambient light sensor will measure surrounding brightness. The lux values are converted through a logarithmic mapping in order to make the color changes similar how human eyes sense light. Specifically, the light will be dark orange in dark environment, and bright blue in bright environment.

<img src="https://github.com/user-attachments/assets/89598fe2-b574-4bb3-a192-71747cdaa207" width="600">

As mentioned, the system operates in two modes, auto and manual. Auto mode is the default mode, which will be engaged after the arduino is powered. It would light two LEDs at a time, perform a flowing light through Vespera. Originally, only one LED will be lighted, which is, in my mind, more intuitive as the following manual mode. However, after testing with multiple classmates and friends, the feedback is that one single LED is kinda too hard to find during daytime. So it now will light two pixels: current LED pixel, and the next one.

<img width="200" height="200" alt="IMG_2062(20251028-151133)" src="https://github.com/user-attachments/assets/b1eff9fe-e54d-4794-acb2-bf993d8a2aff" />

<img width="600" height="300" alt="QQ_1761664089065" src="https://github.com/user-attachments/assets/baf79618-d18c-4fa3-8297-1d4d8109bf52" />

When the joystick is pressed, the manual mode will be engaged. The current light will stop flowing, and user will be able to control one of the LED pixel with the joystick. The color of the selected LED is still determined by the ambient light. Thus, if the ambient light is changing, the user will be able to record the ambient light at the moment to creat a light map.

The Arduino connects to WiFi (arduino_secrets.h) and then connects to an MQTT broker (mqtt.cetools.org on port 1884). 

Then it will publish to an MQTT topic of the form:

student/CASA0014/luminaire/<lightId>

where <lightId> is our assigned ID (for this prototype: "13").

# Limitation
Joystick midpoint needs more adjustment, it's not very sensitive.

# Work in Progress
1. Better Joystick holder
2. Light color adjustment
3. Manual Mode Light Indicator, showing the current selected pixel (blink?)
