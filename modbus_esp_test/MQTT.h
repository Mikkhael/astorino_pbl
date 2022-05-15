#pragma once
#include "ESP8266WiFi.h"
#include "Adafruit_MQTT_Client.h"



struct MQTT : public Adafruit_MQTT_Client{
  WiFiClient wifiClient;
  
  // Subscribes
  Adafruit_MQTT_Subscribe sub1(&client, "sub1");
  Adafruit_MQTT_Subscribe echo_in(&client, "echo/in");

  // Publishes
  Adafruit_MQTT_Subscribe pub1(&client, "pub1");
  Adafruit_MQTT_Subscribe echo_out(&client, "echo/out");

  MQTT(const char* server, int port, const char* user = "", const char* pass = "")
    : Adafruit_MQTT_Client(&wifiClient, server, port, user, pass)
  {}

  void wait_for_connect(){
    if (this->connected())
      return;
    Serial.print("Connecting to MQTT... ");
    int ret = 0;
    while ((ret = this->connect()) != 0) {
         Serial.println(this->connectErrorString(ret));
         Serial.println("Retrying MQTT connection in 2 seconds...");
         this->disconnect();
         delay(2000);
    }
    Serial.println("MQTT Connected!");
  }

  bool try_reconnect(){
    if (this->connected())
      return;
    Serial.print("Reconnecting to MQTT... ");
    int ret = 0;
    if ((ret = this->connect()) != 0) {
         Serial.println(this->connectErrorString(ret));
         this->disconnect();
         return false;
    }
    Serial.println("MQTT Connected!");
    return true;
  }

  void init(){
    wait_for_connect();
  }
  
  void loop(){
    if(!try_reconnect()){
      return;
    }
    if(!wifiClient.available()){
      return;
    }
    //TODO
  }
    
};
