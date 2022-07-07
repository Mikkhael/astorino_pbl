#pragma once
#include <PubSubClient.h>
#include "ESP8266WiFi.h"
#include "IOManager.h"
#include "AstorinoCmdManager.h"


struct MQTT{
  
  WiFiClient clientWiFi;
  PubSubClient client;

  String id = "ESP-RobotController";
  String user = "user";
  String pass = "password";
  String address;
  uint16_t port = 1883;
  unsigned long reconnectionTimeout = 5000;
  unsigned long updateInterval = 500;

  AstorinoCmdManager* acm;

  MQTT() : client(clientWiFi) {}

  struct Payload{
    // , 0 = Unconnected, 1 = OFF, 2 = ON
    uint8_t dio[DIO::FunctionsCount]{};
    uint8_t queueFull = 0;
    uint8_t queueEmpty = 0;
    uint8_t idle = 0;
    //uint8_t reserved;

    // Numbers
    uint16_t executedCmds = 0;
    uint16_t executedDebugCmds = 0;
    uint16_t currentExecutingCmd = 255; // 255 - none command

    Payload(){
      for(int i=0; i<DIO::FunctionsCount; i++){
        dio[i] = 0;
      }
    }

    void populateDio(){
      for(int i=0; i<DIO::FunctionsCount; i++){
        auto state = ioManager.dio.readState(DIO::getFunctionFromIndex(i));
        dio[i] = uint8_t(state);
      }
    }
  } payload;

  static_assert(sizeof(Payload) == DIO::FunctionsCount + 3 + 3*2);

  unsigned long lastUpdate = 0;
  bool trySendUpdate(){
    if(client.connected() && millis() - lastUpdate > updateInterval){
      payload.populateDio();
      //Serial.println("MQTT Sent Update");
      //Serial.printf("Executed cmd: %d, size: %d\n", payload.currentExecutingCmd, sizeof(payload));
      client.publish("robotstate", (uint8_t*)(&payload), sizeof(payload), false);
      lastUpdate = millis();
      return true;
    }
    return false;
  }
  
  unsigned long reconnectionWaitStart = 0;
  bool tryReconnect(){
    if(client.connected()){
      return true;
    }
    Serial.println("Attempting MQTT reconnection...");
    if(client.connect(id.c_str(), user.c_str(), pass.c_str())){
      onConnect();
      return true;
    }
    Serial.print("MQTT Connection Error: ");
    Serial.println(client.state());
    reconnectionWaitStart = millis();
    return false;
  }

  void onConnect(){
    Serial.print("MQTT Connected to ");
    Serial.println(address);
    client.subscribe("customCommand");
  }

  void handleRequest(char* topic, uint8_t* payload, unsigned int length){
    if(strcmp(topic, "customCommand")){
      Serial.printf("Received %d custom commands via MQTT\n", length);
      for(unsigned int i=0; i<length; i++){
        Msg msg;
        msg.parts[0] = payload[i];
        acm->enqueueCommand(msg, true);
      }
    }
  }

  void begin(){
    client.disconnect();
    client.setServer(address.c_str(), port);
    client.setCallback([this](char* topic, uint8_t* payload, unsigned int length){this->handleRequest(topic, payload, length);});
    tryReconnect();
  }

  void end(){
    client.disconnect();
  }

  void loop(){
    if(!client.connected()){
      if(millis() - reconnectionWaitStart < reconnectionTimeout){
        return;
      }
      if(reconnectionTimeout > 0 && !tryReconnect()){
        return;
      }
    }

    client.loop();
    trySendUpdate();
  }
  
};
