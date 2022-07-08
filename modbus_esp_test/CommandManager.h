#pragma once
#include "IOManager.h"
#include "MBServer.h"
#include "AstorinoCmdManager.h"
#include "MQTT.h"

const char* HELP_STR =
"==== HELP ==== \n"
"help                  - prints help\n"
"conf                  - show current config\n"
"add x1 x2             - adds two numbers\n"
"mbr (r|h|i|c) addr    - reads from modbus\n"
"mbw (r|h|i|c) addr val- writes to modbus\n"
"mbrlog (0|1)          - logging mb reads\n"
"mbwlog (0|1)          - logging mb writes\n"
"cmd (w1) [w2] [w3]    - send cmd to arduino\n"
"cmdpins (count)       - set cmd pins count\n"
"invout (0|1)          - set inverting output\n"
"invin  (0|1)          - set inverting input\n"
"mode (single|adv|alt) - set cmd sending mode\n"
"word size             - set cmd word size (in bits)\n"
"usepcf (0|1)          - set wheather using PCF\n"
"haltpcf (0|1)         - halt PCF communication\n"
"r                     - reads all pcf pins\n"
"w pin (0|1)           - sets output pcf pin\n"
"dr                    - read dedicated i/o pins\n"
"dw name (0|1)         - sets dedicated output pin\n"
"da name pin out? inv? - assigns a pin (1-8, 1001-1008, D0-D8) to a name\n"
"demo (0|1)            - set DEMO mode\n"
"stopsend              - aborts sending cmd\n"
"mqttupd ms            - set Mqtt update interval (in ms)\n"
"mqttrec ms            - set Mqtt reconnect timeout (in ms)\n"
"mqttaddr addr         - set Mqtt address\n"
"=====\n";


struct CommandManager
{

  MBServer* mbserver;
  AstorinoCmdManager* acm;
  MQTT* mqtt;

  constexpr static int MAX_ARGS = 4;
  String argsBuffer[MAX_ARGS];

  void handleCommand(String args[]){
//    Serial.print("Handling command: ");
//    for(int i=0; i<MAX_ARGS; i++){
//      Serial.print(args[i]);
//      Serial.print(", ");
//    }
//    Serial.println();

    if(args[0] == "help"){
      printHelp();
    }
    else if(args[0] == "conf"){
      Serial.println("===== CONF =====");
      Serial.printf("Ip:              %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("MBLog Read:      %d\n", mbserver->logReads);
      Serial.printf("MBLog Write:     %d\n", mbserver->logWrites);
      Serial.printf("Cmd Pins Count:  %d\n", ioManager.CmdPinsCount);
      Serial.printf("Invert Output:   %d\n", ioManager.invertedOutput);
      Serial.printf("Invert Input:    %d\n", ioManager.invertedInput);
      Serial.printf("Mode:            %d\n", acm->sendingMode);
      Serial.printf("Word Size:       %d\n", acm->sender.WordSize);
      Serial.println("========");
    }
    else if(args[0] == "add"){
      int arg1 = args[1].toInt();
      int arg2 = args[2].toInt();
      Serial.printf("%d + %d = %d", arg1, arg2, arg1 + arg2);
    }
    else if(args[0] == "mbr"){
      uint16_t addr = args[2].toInt();
      if(args[1] == "r"){
        Serial.printf("Ireg %d = %d", addr, mbserver->mb.Ireg(addr));
      }
      else if(args[1] == "h"){
        Serial.printf("Hreg %d = %d", addr, mbserver->mb.Hreg(addr));
      }
      else if(args[1] == "i"){
        Serial.printf("Ists %d = %d", addr, mbserver->mb.Ists(addr));
      }
      else if(args[1] == "c"){
        Serial.printf("Coil %d = %d", addr, mbserver->mb.Coil(addr));
      }
      else{
        Serial.print("Invalid args.");
      }
      Serial.println();
    }
    else if(args[0] == "mbw"){
      uint16_t addr = args[2].toInt();
      uint16_t val = args[3].toInt();
      if(args[1] == "r"){
        Serial.printf("Ireg %d -> %d", addr, val);
        mbserver->mb.Ireg(addr, val);
      }
      else if(args[1] == "h"){
        Serial.printf("Hreg %d -> %d", addr, val);
        mbserver->mb.Hreg(addr, val);
      }
      else if(args[1] == "i"){
        Serial.printf("Ists %d -> %d", addr, val);
        mbserver->mb.Ists(addr, val);
      }
      else if(args[1] == "c"){
        Serial.printf("Coil %d -> %d", addr, val);
        mbserver->mb.Coil(addr, val);
      }
      else{
        Serial.print("Invalid args.");
      }
      Serial.println();
    }
    else if(args[0] == "mbrlog"){
      mbserver->logReads = args[1][0] == '1';
      Serial.printf("Setting mb Reads logs to %d\n", mbserver->logReads);
    }
    else if(args[0] == "mbwlog"){
      mbserver->logWrites = args[1][0] == '1';
      Serial.printf("Setting mb Writes logs to %d\n", mbserver->logWrites);
    }
    else if(args[0] == "cmd"){
      Msg msg;
      msg.parts[0] = args[1].toInt();
      msg.parts[1] = args[2].toInt();
      msg.parts[2] = args[3].toInt();
      Serial.printf("Premaring command: %d %d %d", msg.parts[0], msg.parts[1], msg.parts[2]);
      acm->enqueueCommand(msg, true);
    }
    else if(args[0] == "cmdpins"){
      int val = args[1].toInt();
      if(val > IOManager::CmdPinsCountMax)
        val = IOManager::CmdPinsCountMax;
      Serial.printf("Setting Cmd Pins Count to %d.\n", val);
      ioManager.CmdPinsCount = val;
      ioManager.setupPins();
    }
    else if(args[0] == "invout"){
      ioManager.invertedOutput = args[1][0] == '1';
      Serial.printf("Setting Inverting Output to: ");
      Serial.println(ioManager.invertedOutput);
      auto& mod = ioManager.dio.getModule(DIO::Type::RobotIn);
      for(int i=0; i<mod.PinsCount; i++) mod.isInverted[i] = ioManager.invertedOutput;
      ioManager.setupPins();
    }
    else if(args[0] == "invin"){
      ioManager.invertedInput = args[1][0] == '1';
      Serial.printf("Setting Inverting Input to: ");
      Serial.println(ioManager.invertedInput);
      auto& mod = ioManager.dio.getModule(DIO::Type::RobotOut);
      for(int i=0; i<mod.PinsCount; i++) mod.isInverted[i] = ioManager.invertedInput;
      ioManager.setupPins();
    }
    else if(args[0] == "mode"){
      acm->abort();
      if(args[1] == "single"){
        acm->sendingMode = AstorinoCmdManager::Mode::Single;
      }
      else if(args[1] == "adv"){
        acm->sendingMode = AstorinoCmdManager::Mode::Advanced;
      }
      else if(args[1] == "alt"){
        acm->sendingMode = AstorinoCmdManager::Mode::Alternating;
      }
      else{
        Serial.printf("Unknown mode %s\n", args[1].c_str());
        return;
      }
      Serial.printf("Set mode to %s\n", args[1].c_str());
    }
    else if(args[0] == "word"){
      int val = args[1].toInt();
      if(val < 0) val = 0;
      if(val > 16) val = 16;
      Serial.printf("Setting word size to %d\n", val);
      acm->sender.WordSize = val;
    }
    else if(args[0] == "haltpcf"){
      bool val = args[1][0] == '1';
      Serial.printf("Halting PCF communication: %d\n", val);
      for(auto& state : ioManager.pcfStates){
        state.isHalted = val;
      }
      if(val){
        digitalWrite(SDA, LOW);
        digitalWrite(SCL, LOW);
      }
    }
    else if(args[0] == "r"){
      ioManager.dio.getModule(DIO::Type::RobotOut).readall();
      Serial.print("IN:  ");
      printBinaryUint8(ioManager.dio.getModule(DIO::Type::RobotOut).lastInState);
      Serial.print("\nOUT: ");
      printBinaryUint8(ioManager.dio.getModule(DIO::Type::RobotIn).outState);
      Serial.println();
    }
    else if(args[0] == "dr"){
      ioManager.printPcfPins();
    }
    else if(args[0] == "dw"){
      auto function = DIO::getFunctionFromName(args[1]);
      if(function == DIO::Function::NONE){
        Serial.printf("Function named %s does not exist\n", args[1].c_str());
      }else{
        auto& mapping = ioManager.dio.getMapping(function);
        if(mapping.type == DIO::Type::NONE){
            Serial.printf("Function %s has no assigned pin\n", args[1].c_str());
        }else{
            bool value = args[2][0] == '1';
            if(mapping.type == DIO::Type::RobotIn){
              Serial.printf("Setting %s (pcf %d (%d)) to %d\n", args[1].c_str(), mapping.pin, IOManager::PcfToRobotPin(mapping.pin, true), value);
              ioManager.dio.write(function, value);
            }
            else if(mapping.type == DIO::Type::GPIO){
              Serial.printf("Setting %s (gpio D%d (%d)) to %d\n", args[1].c_str(), mapping.pin, IOManager::GPIOPinsMap[mapping.pin], value);
              ioManager.dio.write(function, value);
            }else{
              Serial.printf("ERROR: Pin assigned to unsuported module.");
            }
        }
      }
    }
    else if(args[0] == "da"){
      auto function = DIO::getFunctionFromName(args[1]);
      if(function == DIO::Function::NONE){
        Serial.printf("Function named %s does not exist\n", args[1].c_str());
      }else{
        bool isInput = args[3] != "out";
        if(args[2][0] == 'D'){ // GPIO
          int pin = args[2][1]- '1';
          bool inv = args[4] == "inv";
          int dpin = IOManager::GPIOPinsMap[pin];
          Serial.printf("Assigning pin D%d (%d) [Input: %d, Inv: %d] to function %s\n", pin, dpin, isInput, inv, args[1].c_str());
          ioManager.dio.assignFunctionToPin(function,
             isInput ? DIO::PinMode::Input : DIO::PinMode::Output,
             false,
             DIO::Type::GPIO,
             pin);
        }else{ // PCF
          int pin = args[2].toInt();
          int  pcfPin  = IOManager::RobotToPcfPin(pin);
          bool inv = isInput ? ioManager.invertedInput : ioManager.invertedOutput;
          Serial.printf("Assigning pin %d (%d) [Input: %d, Inv: %d] to function %s\n", pcfPin, pin, isInput, inv, args[1].c_str());
          ioManager.dio.assignFunctionToPin(function,
             isInput ? DIO::PinMode::Input : DIO::PinMode::Output,
             inv,
             isInput ? DIO::Type::RobotOut : DIO::Type::RobotIn,
             pcfPin);
        }
      }
    }
    else if(args[0] == "demo"){
      bool val = args[1][0] == '1';
      Serial.printf("Setting DEMO mode: %d\n", val);
      acm->sender.performDemo = val;
    }
    else if(args[0] == "stopsend"){
      Serial.println("Aborting transfer\n");
      acm->abort();
      ioManager.setupPins();
    }
    else if(args[0] == "mqttupd"){
      int val = args[1].toInt();
      Serial.printf("Setting MQTT Update Interval to %dms\n", val);
      mqtt->updateInterval = val;      
    }
    else if(args[0] == "mqttrec"){
      int val = args[1].toInt();
      Serial.printf("Setting MQTT Reconnection Timeout to %dms\n", val);
      mqtt->reconnectionTimeout = val;      
    }
    else if(args[0] == "mqttaddr"){
      Serial.printf("Setting MQTT Address to %s\n", args[1].c_str());
      mqtt->address = args[1];
      mqtt->end();
      mqtt->begin();      
    }
    else{
      Serial.println("Unknown command");
    }
  }

  void parseCommandString(String cmdStr, String args[]){
    //Serial.printf("Parsing command: \"%s\"\n", cmdStr.c_str());
    int lastIndex = 0;
    int i=0;
    for(i=0; i<MAX_ARGS; i++){
      int nextIndex = cmdStr.indexOf(' ', lastIndex);
      args[i] = cmdStr.substring(lastIndex, nextIndex);
      ;
      if((lastIndex = nextIndex+1) == 0){
        break;
      }
    }
    while(++i < MAX_ARGS){
      args[i] = "";
    }
  }

  String readCommandString(Stream& stream = Serial){
    String cmdStr = stream.readStringUntil('\n');
    Serial.read();
    return cmdStr;
  }

  void init();

  void loop(){
    if(Serial.available()){
      auto cmdStr = readCommandString();
      parseCommandString(cmdStr, argsBuffer);
      handleCommand(argsBuffer);
    }
  }

  void printHelp(){
    Serial.print(HELP_STR);
  }

  
};
