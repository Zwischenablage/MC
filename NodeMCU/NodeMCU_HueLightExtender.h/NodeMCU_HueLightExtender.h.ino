//#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"
#include <time.h>

//Wifi Manager
#include <DNSServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

//http server
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//RC switch
#include <RCSwitch.h>


#define SERIAL_BAUDRATE                 115200
#define LED                             2
#define DEBUG_FAUXMO_VERBOSE 1
#define DEBUG_FAUXMO 1

fauxmoESP fauxmo;
RCSwitch mySwitch = RCSwitch();
int doSwitch = 1;
char buffer[1024];
ESP8266WebServer server(80);
int timezone = 1;
int dst = 0;

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------
void wifiSetup() {

     //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("433Mhz Hue Gateway");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    
    //if you get here you have connected to the WiFi
    Serial.printf("[WIFI] connected to SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

}

void timeSetup()
{
  configTime(timezone * 3600, dst * 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
}

void http_handleRoot() {
  server.send(200, "text/plain", buffer);
}

void httpSetup()
{
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", http_handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void rcSwitchSetup()
{
  mySwitch.enableReceive(5);  // D1
  mySwitch.enableTransmit(4); //D2
  pinMode(2, OUTPUT); //Declare Pin mode
  digitalWrite(2, LOW);    //Turn the LED off 
}

void toggleLED()
{
  if (doSwitch > 0)
  {
    digitalWrite(2, HIGH);   //Turn the LED on
  }
  else
  {
    digitalWrite(2, LOW);    //Turn the LED off  
  }
  doSwitch *= -1;
  delay(500);
}

void processRCSwitch()
{
    if (mySwitch.available()) {
    
    int value = mySwitch.getReceivedValue();
    toggleLED();
    
    if (value == 0) {
      Serial.print("Unknown encoding");
    } else {
      Serial.print("Received ");
      Serial.print( mySwitch.getReceivedValue() );
      Serial.print(" / ");
      Serial.print( mySwitch.getReceivedBitlength() );
      Serial.print("bit ");
      Serial.print("Protocol: ");
      Serial.println( mySwitch.getReceivedProtocol() );

      char tmp[256];
      time_t now = time(nullptr); 
      sprintf(tmp, "[%s][RC] Received %i\n", ctime(&now), mySwitch.getReceivedValue());
      //sprintf(buffer, "[MAIN] Value %lu, bitlength %i, protocl %i\n", mySwitch.getReceivedValue() , mySwitch.getReceivedBitlength(), mySwitch.getReceivedProtocol());
      strcpy(buffer+strlen(buffer), tmp);
    
    }

    mySwitch.resetAvailable();
    }
}


void setup() {

    // Init serial port and clean garbage
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();

    // Wifi
    wifiSetup();

    // HTTP
    httpSetup();

    // RCSwitch
    rcSwitchSetup();

    // Time
    timeSetup();

     // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);
    fauxmo.enable(false);
    fauxmo.enable(true);

    // You can use different ways to invoke alexa to modify the devices state:
    // "Alexa, turn light one on" ("light one" is the name of the first device below)
    // "Alexa, turn on light one"
    // "Alexa, set light one to fifty" (50 means 50% of brightness)

    // Add virtual devices
    fauxmo.addDevice("Drucker");
	  fauxmo.addDevice("Gang");
    fauxmo.addDevice("Highboard");
    fauxmo.addDevice("Stern");
    fauxmo.addDevice("Steckdose 1");
    fauxmo.addDevice("Steckdose 2");
    fauxmo.addDevice("Steckdose 3");

    // fauxmoESP 2.0.0 has changed the callback signature to add the device_id,
    // this way it's easier to match devices to action without having to compare strings.
    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {

        
        time_t now = time(nullptr);        
        //Serial.printf("[%s][onSetState] Device #%d (%s) state: %s value: %d\n", ctime(&now), device_id, device_name, state ? "ON" : "OFF", value);
        //sprintf(buffer, "[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
        
        char tmp[256];
        sprintf(tmp, "[%s][Hue] Device #%d (%s) state: %s value: %d\n", ctime(&now), device_id, device_name, state ? "ON" : "OFF", value);

        if (state) //true == ON, false == OFF
        {
          switch (device_id)
          {
            case 0: mySwitch.send(1361, 24); break; //first light
            case 1: mySwitch.send(4433, 24); break; //second light
            case 2: mySwitch.send(5201, 24); break; //third light
            case 3: mySwitch.send(5393, 24); break; //fourth light
            case 4: mySwitch.switchOn('a', 1, 1); break; //fifth light
            case 5: mySwitch.switchOn('a', 1, 2); break; //sixth light
            case 6: mySwitch.switchOn('a', 1, 3); break; //seventh light
          }
        }
        else
        {
          switch (device_id)
          {
            case 0: mySwitch.send(1364, 24); break; //first light
            case 1: mySwitch.send(4436, 24); break; //second light
            case 2: mySwitch.send(5204, 24); break; //third light
            case 3: mySwitch.send(5396, 24); break; //fourth light
            case 4: mySwitch.switchOff('a', 1, 1); break; //fifth light
            case 5: mySwitch.switchOff('a', 1, 2); break; //sixth light
            case 6: mySwitch.switchOff('a', 1, 3); break; //seventh light
          }
        }
        strcpy(buffer+strlen(buffer), tmp);
    });

}

void loop() {

    // Since fauxmoESP 2.0 the library uses the "compatibility" mode by
    // default, this means that it uses WiFiUdp class instead of AsyncUDP.
    // The later requires the Arduino Core for ESP8266 staging version
    // whilst the former works fine with current stable 2.3.0 version.
    // But, since it's not "async" anymore we have to manually poll for UDP
    // packets
    fauxmo.handle();

    server.handleClient();

    processRCSwitch();
 

    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    }

    //delay(50);
}
