#include "IOManager.h"
#include "AstorinoCmdManager.h"
#include "MBServer.h"
#include "CommandManager.h"
#include "MQTT.h"

MBServer mbserver;
AstorinoCmdManager acm;
CommandManager cmd;
MQTT mqtt;

// Network
#define USE_HOME_NETWORK 0
#if USE_HOME_NETWORK
const char SSID[] = "TP-LINK_FD2F53";
const char PASS[] = "42936961";
#else
const char SSID[] = "Lab408";
const char PASS[] = "Laborat408";
const IPAddress localIP(192,168,0,101);
const IPAddress netmask(255,255,255,0);
const IPAddress gateway(192,168,0,222);
const IPAddress dns1(157,158,3,1);
const IPAddress dns2(157,158,3,2);
#endif

bool usingAdvanced = false;
unsigned int receivedModbusCmds = 0;

  
void setup() {
  ////////// SERIAL
  Serial.begin(115200);

  while(!Serial){
    delay(500);
  }
  Serial.println("\nSerial Ready.");

  ioManager.setupPins();
  
  ////////// WIFI
  #if USE_HOME_NETWORK == 0
  if (!WiFi.config(localIP, gateway, netmask, dns1, dns2)) {
      Serial.println("STA Failed to configure");
  }
  #endif
  
  WiFi.begin(SSID, PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    ioManager.refreshOutState(false);
    Serial.print(".");
    if(Serial.available()){
      if(Serial.read() == '!'){
        Serial.println("Bypassing WiFi Connection");
        break;
      }
    }
  }
  ioManager.refreshOutState(false);
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ////////// MQTT
  mqtt.address = "192.168.0.104";
  mqtt.user    = "user";
  mqtt.pass    = "password";
  mqtt.begin();

  ////////// Modbus
  mbserver.init();  

  ////////// ACM
  acm.init();

  ///////// CMD
  cmd.mbserver = &mbserver;
  cmd.acm      = &acm;
  cmd.mqtt     = &mqtt;
  acm.init();
}
 
void loop() {

  ioManager.loop();
  cmd.loop();
  mbserver.loop();
  acm.loop();
  mqtt.loop();
  
  if(mbserver.requestedReset && acm.cmdQueue.isempty()){
    Serial.println("Resseting communication");
    mbserver.requestedReset = false;
    acm.sender.awaitForIdle = false;
    acm.sender.executedCmds = 0;
  }

  if(mbserver.newCmd){
    mbserver.newCmd = false;
    Msg msg = mbserver.getFullMsg();
    Serial.printf("MSG %d %d %d", msg.parts[0], msg.parts[1], msg.parts[2]);
    ++receivedModbusCmds;
    acm.enqueueCommand(msg);
  }

  mbserver.set(MBServer::RegName::RobotIdle,    acm.isRobotIdle);
  mbserver.set(MBServer::RegName::QueueEmpty,   acm.cmdQueue.isempty());
  mbserver.set(MBServer::RegName::QueueFull,    acm.cmdQueue.isfull());
  mbserver.set(MBServer::RegName::ExecutedCmds, (uint16_t)acm.sender.executedCmds);

  mbserver.set(MBServer::RegName::GrabberClosed,  ioManager.dio.readVal(DIO::Function::GGrabbed, false));
  mbserver.set(MBServer::RegName::ElementGrabbed, ioManager.dio.readVal(DIO::Function::GFar1,    false));
  mbserver.set(MBServer::RegName::ButtonPressed,  ioManager.dio.readVal(DIO::Function::GTens,    false));

  ioManager.dio.write(DIO::Function::GTestButt, mbserver.isTestButton());

  mqtt.payload.idle       = acm.isRobotIdle        ? 2 : 1;
  mqtt.payload.queueEmpty = acm.cmdQueue.isempty() ? 2 : 1;
  mqtt.payload.queueFull  = acm.cmdQueue.isfull()  ? 2 : 1;
  mqtt.payload.executedCmds       = acm.sender.executedCmds;
  mqtt.payload.executedDebugCmds  = acm.sender.executedDebugCmds;

    
  //delay(100);
}
