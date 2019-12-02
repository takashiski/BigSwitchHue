#include <M5StickC.h>

#include <FastLED.h>


#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif
#include "fauxmoESP.h"



// Rename the credentials.sample.h file to credentials.h and 
// edit it according to your router configuration
#include "credentials.h"

fauxmoESP fauxmo;

// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE     115200

#define ID_LED "BigSwitch"
#define LED 10

#define LED_NUM 12
#define LED_PIN 33
#define LED_BRIGHTNESS 100
CRGB leds[LED_NUM];

#define SWITCH_PIN 32

bool this_state=false;
bool last_switch_state=false;
unsigned char last_value=255;

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    

    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

}

void setup() {

    // Init serial port and clean garbage
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();
    Serial.println();

    // LEDs
    pinMode(SWITCH_PIN,INPUT_PULLUP);
    
    FastLED.addLeds<WS2812B,LED_PIN,GRB>(leds,LED_NUM).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(LED_BRIGHTNESS);
    setColorToAllLEDs(CRGB::Black,0);
    FastLED.show();
    pinMode(LED,OUTPUT);
    digitalWrite(LED,LOW);

    // Wifi
    wifiSetup();

    // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);
    
    fauxmo.addDevice(ID_LED);

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        
        // Callback when a command from Alexa is received. 
        // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
        // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
        // Just remember not to delay too much here, this is a callback, exit as soon as possible.
        // If you have to do something more involved here set a flag and process it in your main loop.
        
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
        // Otherwise comparing the device_name is safer.

        this_state=state;
        last_value=value;
        if(strcmp(device_name, ID_LED)==0)
        {
          setBrightnessToAllLEDs(state,value);
          digitalWrite(LED,state?LOW:HIGH);
        }
    });
    digitalWrite(LED,HIGH);
}

void loop() {

    // fauxmoESP uses an async TCP server but a sync UDP server
    // Therefore, we have to manually poll for UDP packets
    fauxmo.handle();
    checkSwitch();

    // This is a sample code to output free heap every 5 seconds
    // This is a cheap way to detect memory leaks
    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }

    // If your device state is changed by any other means (MQTT, physical button,...)
    // you can instruct the library to report the new state to Alexa on next request:
    // fauxmo.setState(ID_YELLOW, true, 255);

}

void checkSwitch()
{

  bool pin_state=digitalRead(SWITCH_PIN);
  if(last_switch_state!=pin_state&&pin_state==LOW)
  {
    this_state=!this_state;
    fauxmo.setState(ID_LED,this_state,last_value);
    setBrightnessToAllLEDs(this_state,last_value);
  }
    last_switch_state=pin_state;
  Serial.print(digitalRead(SWITCH_PIN));
  Serial.print(this_state);
  Serial.print(last_value);
  Serial.println();
  
}

void setBrightnessToAllLEDs(bool state,int value)
{
  if(state==false)value=0;
  
  for(int i=0;i<LED_NUM;i+=1)
  {
    leds[i]=CRGB(value,value,value);
  }
  FastLED.show();
}

void setColorToAllLEDs(CRGB color,int value)
{
  for(int i=0;i<LED_NUM;i+=1)
  {
    leds[i]=color*value;
  }
  FastLED.show();
 
}
