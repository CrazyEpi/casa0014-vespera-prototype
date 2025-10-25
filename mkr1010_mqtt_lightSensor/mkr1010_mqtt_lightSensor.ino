// Duncan Wilson Oct 2025 - v1 - MQTT messager to vespera

// works with MKR1010

#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <Adafruit_VEML7700.h>
#include "arduino_secrets.h" 
#include <utility/wifi_drv.h>   // library to drive to RGB LED on the MKR1010

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

const int MIN_BRIGHT = 90;
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
 
void loop() {
  // Reconnect if necessary
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  
  if (WiFi.status() != WL_CONNECTED){
    startWifi();
  }
  // keep mqtt alive
  mqttClient.loop();

  for(int n=0; n<num_leds; n++){
    float lux = veml.readLux(); 
    if (!isfinite(lux) || lux < 0) lux = 0; //safety
    float t = luxLog(lux, LUX_DARK, LUX_BRIGHT);  //log filter
    if (t < 0) t = 0; if (t > 1) t = 1; //safety

    float newR = interpo(warm_R, cool_R, t);  //calculate new rgb values
    float newG = interpo(warm_G, cool_G, t);
    float newB = interpo(warm_B, cool_B, t);

    float newRbright = (interpo(MIN_BRIGHT, MAX_BRIGHT, t)/255.0f)*newR;  //calculate bright level
    float newGbright = (interpo(MIN_BRIGHT, MAX_BRIGHT, t)/255.0f)*newG;
    float newBbright = (interpo(MIN_BRIGHT, MAX_BRIGHT, t)/255.0f)*newB;

    send_all_off();
    delay(50);
    send_RGB_to_pixel(newRbright,newGbright,newBbright,n);
    Serial.print("  Color Factor=(");
    Serial.print(t); Serial.println(")");
    delay(120);
  }
}

// Function to update the R, G, B values of a single LED pixel
// RGB can a value between 0-254, pixel is 0-71 for a 72 neopixel strip
void send_RGB_to_pixel(int r, int g, int b, int pixel) {
  // Check if the mqttClient is connected before publishing
  if (mqttClient.connected()) {
    // Update the byte array with the specified RGB color pattern
    RGBpayload[pixel * 3 + 0] = (byte)r; // Red
    RGBpayload[pixel * 3 + 1] = (byte)g; // Green
    RGBpayload[pixel * 3 + 2] = (byte)b; // Blue

    // Publish the byte array
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
    
  } else {
    Serial.println("MQTT mqttClient not connected, cannot publish from *send_RGB_to_pixel*.");
  }
}

void send_all_off() {
  // Check if the mqttClient is connected before publishing
  if (mqttClient.connected()) {
    // Fill the byte array with the specified RGB color pattern
    for(int pixel=0; pixel < num_leds; pixel++){
      RGBpayload[pixel * 3 + 0] = (byte)0; // Red
      RGBpayload[pixel * 3 + 1] = (byte)0; // Green
      RGBpayload[pixel * 3 + 2] = (byte)0; // Blue
    }
    // Publish the byte array
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
    
  } else {
    Serial.println("MQTT mqttClient not connected, cannot publish from *send_all_off*.");
  }
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

void send_all_random() {
  // Check if the mqttClient is connected before publishing
  if (mqttClient.connected()) {
    // Fill the byte array with the specified RGB color pattern
    for(int pixel=0; pixel < num_leds; pixel++){
      RGBpayload[pixel * 3 + 0] = (byte)random(50,256); // Red - 256 is exclusive, so it goes up to 255
      RGBpayload[pixel * 3 + 1] = (byte)random(50,256); // Green
      RGBpayload[pixel * 3 + 2] = (byte)random(50,256); // Blue
    }
    // Publish the byte array
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
    
    Serial.println("Published an all random byte array.");
  } else {
    Serial.println("MQTT mqttClient not connected, cannot publish from *send_all_random*.");
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



