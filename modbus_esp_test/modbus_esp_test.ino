#include "IOManager.h"
#include "AstorinoCmdManager.h"
#include "MBServer.h"
#include "CommandManager.h"

MBServer mbserver;
AstorinoCmdManager acm;
CommandManager cmd;

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
void mbserverOnNewCmd(){
  Msg msg = mbserver.getFullMsg();
  Serial.printf("MSG %d %d %d", msg.parts[0], msg.parts[1], msg.parts[2]);
  ++receivedModbusCmds;
  acm.enqueueCommand(msg);
}

  
void setup() {
  ////////// SERIAL
  Serial.begin(115200);

  while(!Serial){
    delay(500);
  }
  Serial.println("\nSerial Ready.");

  ////////// WIFI
  #if USE_HOME_NETWORK == 0
  if (!WiFi.config(localIP, gateway, netmask, dns1, dns2)) {
      Serial.println("STA Failed to configure");
  }
  #endif
  
  WiFi.begin(SSID, PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(Serial.available()){
      if(Serial.read() == '!'){
        Serial.println("Bypassing WiFi Connection");
        break;
      }
    }
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ioManager.setupPins();

  ////////// Modbus

  mbserver.onNewCmd = &mbserverOnNewCmd;
  mbserver.init();  

  ////////// ACM
  acm.init();

  ///////// CMD
  cmd.mbserver = &mbserver;
  cmd.acm      = &acm;
  acm.init();
}
 
void loop() {

  ioManager.refreshOutState();
  cmd.loop();
  mbserver.loop();
  acm.loop();
  
  if(mbserver.requestedReset && acm.cmdQueue.isempty()){
    Serial.println("Resseting communication");
    mbserver.requestedReset = false;
    acm.sender.awaitForIdle = false;
    acm.sender.executedCmds = 0;
  }

  mbserver.setRobotIdle(acm.isRobotIdle);
  mbserver.setQueueEmpty(acm.cmdQueue.isempty());
  mbserver.setQueueFull(acm.cmdQueue.isfull());
  mbserver.setExecutedCmds(acm.sender.executedCmds);

    
  //delay(100);
}
