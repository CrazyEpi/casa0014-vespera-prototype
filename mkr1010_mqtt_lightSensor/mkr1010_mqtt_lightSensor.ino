#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <Adafruit_VEML7700.h>
#include "arduino_secrets.h" 
#include <utility/wifi_drv.h> 

Adafruit_VEML7700 veml = Adafruit_VEML7700();

/*
**** please enter your sensitive data in the Secret tab/arduino_secrets.h
**** using format below
#define SECRET_SSID "ssid name"
#define SECRET_PASS "ssid password"
#define SECRET_MQTTUSER "user name - eg student"
#define SECRET_MQTTPASS "password";
 */
const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server   = "mqtt.cetools.org";
const int mqtt_port       = 1884;

//joystick pins
const int PIN_VRX = A1;
const int PIN_VRY = A2;
const int PIN_SW  = 2;

const int deadzone = 300;    //joystick neutral deadzone
const int JSmid  = 3270;   //joystick neutral-point

//mode settings
bool manualMode = false;
int lightIndex = 0;

unsigned long lastToggleTime = 0;
int pixelID = 0; //index used in flow light

bool lastButtonPressed = false;

// create wifi object and mqtt object
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Make sure to update your lightid value below with the one you have been allocated
String lightId = "13"; // the topic id number or user number being used.

// Here we define the MQTT topic we will be publishing data to
String mqtt_topic = "student/CASA0014/luminaire/" + lightId;            
String clientId = ""; // will set once i have mac address so that it is unique

// NeoPixel Configuration - we need to know this to know how to send messages 
// to vespera 
const int num_leds = 72;
const int payload_size = num_leds * 3; // x3 for RGB

const float LUX_DARK   = 5.0f;
const float LUX_BRIGHT = 800.0f;

const int MIN_BRIGHT = 80;
const int MAX_BRIGHT = 255;

const uint8_t warm_R = 255, warm_G = 120, warm_B = 0;
const uint8_t cool_R = 0,   cool_G = 170, cool_B = 255;

// Create the byte array to send in MQTT payload this stores all the colours 
// in memory so that they can be accessed in for example the rainbow function
byte RGBpayload[payload_size];

void setup() {
  Serial.begin(115200);
  //while (!Serial); // Wait for serial port to connect (useful for debugging)
  Serial.println("Vespera");
  
  analogReadResolution(12);     // MKR1010 ADC -> 0..4095
  pinMode(PIN_SW, INPUT_PULLUP);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);

  Serial.print("This device is Vespera ");
  Serial.println(lightId);

  if (!veml.begin()) {
  Serial.println("Sensor not found");
  while (1);
  }
  Serial.println("Sensor found");

  // Connect to WiFi
  startWifi();

  // Connect to MQTT broker
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(2000);
  mqttClient.setCallback(callback);
  
  
  Serial.println("Set-up complete");
}

void send_RGB_to_pixel(int r, int g, int b) {
  // Check if the mqttClient is connected before publishing
  if (mqttClient.connected()) {
    //update colors
    // for (int i = 0; i < num_leds; i++) {
    //   RGBpayload[i * 3 + 0] = 0;
    //   RGBpayload[i * 3 + 1] = 0;
    //   RGBpayload[i * 3 + 2] = 0;
    // } 

    RGBpayload[lightIndex * 3 + 0] = (byte)r;
    RGBpayload[lightIndex * 3 + 1] = (byte)g;
    RGBpayload[lightIndex * 3 + 2] = (byte)b;

    // Publish the byte array
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
    
  } else {
    Serial.println("MQTT mqttClient not connected, cannot publish from *send_RGB_to_pixel*.");
  }
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

void calColor(float t, int &rOut, int &gOut, int &bOut) {
  // interpolate color temperature between warm and cool thresholds
  float newR = interpo(warm_R, cool_R, t);
  float newG = interpo(warm_G, cool_G, t);
  float newB = interpo(warm_B, cool_B, t);

  // scale brightness between min and max brightness
  float scale = interpo(MIN_BRIGHT, MAX_BRIGHT, t) / 255.0f;

  float rr = scale * newR;
  float gg = scale * newG;
  float bb = scale * newB;

  if (rr < 0) rr = 0; if (rr > 255) rr = 255;
  if (gg < 0) gg = 0; if (gg > 255) gg = 255;
  if (bb < 0) bb = 0; if (bb > 255) bb = 255;

  rOut = (int)(rr + 0.5f);
  gOut = (int)(gg + 0.5f);
  bOut = (int)(bb + 0.5f);
}

//calculate rgb value with interpolation
float interpo(float a, float b, float x){
  float interpo = a + (b - a)*x;
  return interpo;
}

//log filter
float luxLog(float lux, float darkLux, float brightLux) {
  if (lux < 0) lux = 0;
  if (darkLux < 0.001f) darkLux = 0.001f;

  float num   = log10f(lux + 1.0f) - log10f(darkLux + 1.0f);
  float denom = log10f(brightLux + 1.0f) - log10f(darkLux + 1.0f);
  float t = (denom > 0) ? (num / denom) : 0.0f;
  return t;
}

//make light flow in vespera
void flowLight(){
  //setup
  float lux = veml.readLux();
  if (!isfinite(lux) || lux < 0) lux = 0;

  float t = luxLog(lux, LUX_DARK, LUX_BRIGHT);

  int r,g,b;
  calColor(t, r, g, b);

  if (mqttClient.connected()) {
    for (int i = 0; i < num_leds; i++) {
      RGBpayload[i*3+0] = 0;
      RGBpayload[i*3+1] = 0;
      RGBpayload[i*3+2] = 0;

    }

    RGBpayload[pixelID*3+0] = (byte)r;
    RGBpayload[pixelID*3+1] = (byte)g;
    RGBpayload[pixelID*3+2] = (byte)b;

    int nextPixel = pixelID + 1;
    if (nextPixel < num_leds) {
      RGBpayload[nextPixel*3+0] = (byte)r;
      RGBpayload[nextPixel*3+1] = (byte)g;
      RGBpayload[nextPixel*3+2] = (byte)b;
    }

    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
  }
  pixelID = pixelID + 1;
  if (pixelID >= num_leds) {
    pixelID = 0;
  }
}

void readJoystick(int &xDir, int &yDir, bool &pressed) {
  int xRaw = analogRead(PIN_VRX);
  int yRaw = analogRead(PIN_VRY);

  // define center point, and directions of joystick
  int xC = xRaw - JSmid;
  int yC = JSmid - yRaw;

  // deadzone filter
  if (abs(xC) < deadzone) xC = 0;
  if (abs(yC) < deadzone) yC = 0;

  // convert to direction
  if (xC > 0)      xDir = +1;
  else if (xC < 0) xDir = -1;
  else             xDir = 0;

  if (yC > 0)      yDir = +1;
  else if (yC < 0) yDir = -1;
  else             yDir = 0;

  // button: pulled up normally, LOW when pressed
  pressed = (digitalRead(PIN_SW) == LOW);
}

void joystickUpdate(int xDir, int yDir) {
  int col = lightIndex / 6;
  int row = lightIndex % 6;

  // vertical: up/down = +/-1
  if (yDir == +1) {         // up
    if (row > 0) {
      row -= 1;
    }
  } else if (yDir == -1) {  // down
    if (row < 5) {
      row += 1;
    }
  }

  // horizontal: left/right = +/-6
  if (xDir == +1) {         // right
    if (col < 11) {
      col += 1;
    }
  } else if (xDir == -1) {  // left
    if (col > 0) {
      col -= 1;
    }
  }

  lightIndex = col * 6 + row;
  //Serial.print("light index is updated ");
  //Serial.print(lightIndex);
}

void loop(){
  // Reconnect if necessary
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  
  if (WiFi.status() != WL_CONNECTED){
    startWifi();
  }
  // keep mqtt alive
  mqttClient.loop();

  //read joystick
  int xDir, yDir;
  bool pressed;
  readJoystick(xDir, yDir, pressed);

  //mode selection setup
  unsigned long nowTime = millis();
  //single click
  bool buttonDetection = (!lastButtonPressed && pressed);
    lastButtonPressed = pressed;

  if (buttonDetection && (nowTime - lastToggleTime > 200)) {
  manualMode = !manualMode;
  lastToggleTime = nowTime;

  //pass index number to manual mode
  if (manualMode) {
    lightIndex = pixelID;
  }

  Serial.print("[MODE] manualMode = ");
    if (manualMode) {
      Serial.println("Manual");
    } else {
      Serial.println("Auto");
    }
  }

  //perform control
  if (manualMode) {
    joystickUpdate(xDir, yDir);

    float lux = veml.readLux();
    if (!isfinite(lux) || lux < 0) lux = 0;
    float t = luxLog(lux, LUX_DARK, LUX_BRIGHT);

    int r,g,b;
    calColor(t, r, g, b);
    Serial.print(lightIndex);

    send_RGB_to_pixel(r, g, b);
    delay(50);

    Serial.print("[MANUAL] pixel=");
    Serial.print(lightIndex);
    Serial.print(" xDir=");
    Serial.print(xDir);
    Serial.print(" yDir=");
    Serial.print(yDir);
    Serial.print(" lux=");
    Serial.println(lux,1);

  } else {
    flowLight();
    Serial.println(lightIndex);
    Serial.println(pixelID);

    Serial.println("[AUTO] running flow light................");
  }
}

