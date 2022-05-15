#pragma once
#include <Wire.h>

template<typename T>
static void setbit(T& var, int pin, bool val){
    if(val){
      var |=  (1 << pin);
    }else{
      var &= ~(1 << pin);
    }
}

struct IOManager{

    constexpr static int RobotToPcfPin(int robotPinNumber){
      return 8 - (robotPinNumber % 1000);
    }
    constexpr static int PcfToRobotPin(int pcfPinNumber, bool isPcfOutput){
      return (8 - pcfPinNumber) + isPcfOutput * 1000;
    }
    static constexpr int CmdPinsCountMax = 2;
    int CmdPinsCount = 2;
    bool usePcf         = true;
    bool invertedOutput = true;
    bool invertedInput  = true;
    
// GPIO pins
    int PinsCmd[CmdPinsCountMax] {D1, D2};
    int PinSend = D0;
    int PinRobotIdle = D5;
    int PinRobotAck  = D6;

// PCF pins

    // Output
    uint8_t outputAddress = 0x38;
    uint8_t PcfSend = RobotToPcfPin(1008);
    uint8_t PcfCmd[CmdPinsCountMax] = {
        RobotToPcfPin(1001),
        RobotToPcfPin(1002),
    };

    uint8_t outState = 0xFF;
    
    // Input
    uint8_t inputAddress = 0x39;
    uint8_t PcfRobotIdle =  RobotToPcfPin(1);
    uint8_t PcfRobotAck  =  RobotToPcfPin(2);

    auto writeInv(uint8_t val){
      if(invertedOutput)
        return Wire.write(~val);
      return Wire.write(val);
    }
    auto writeInvDigital(uint8_t pin, bool val){
      if(invertedOutput)
          return digitalWrite(pin, !val);
      return digitalWrite(pin, val);
    }
    uint8_t readInv(){
      if(invertedInput)
        return ~Wire.read();
      return Wire.read();
    }
    bool readInvDigital(uint8_t pin){
      return digitalRead(pin) != invertedInput;
    }

    bool setCmd(uint8_t cmd){
      if(usePcf){
        for(int i=0; i<CmdPinsCount; i++){
          setbit(outState, PcfCmd[i], cmd & (1 << i));
        }
        Wire.beginTransmission(outputAddress);
        writeInv(outState);
        auto res = Wire.endTransmission();
        if(res){
          Serial.print("ERROR PCF setCmd: ");
          Serial.println(res);
          return false;
        }
      }
      else{
        for(int i=0; i<CmdPinsCount; i++){
          writeInvDigital(PinsCmd[i], cmd & (1 << i));
        }
      }
      return true;
    }

    int writePcfPin(uint8_t pin, bool val, bool raw = false){
        bool toSet = (raw && invertedOutput) ? !val : val;
        setbit(outState, pin, toSet);
        Wire.beginTransmission(outputAddress);
        writeInv(outState);
        return Wire.endTransmission();
    }

    bool setSend(bool val){
      if(usePcf){
        int res = writePcfPin(PcfSend, val);
        if(res){
          Serial.print("ERROR PCF PinSend: ");
          Serial.println(res);
          return false;
        }
        return true;
      }
      else{
        writeInvDigital(PinSend, val);
      }
      return true;
    }

    uint8_t getRawOutState(){
      return invertedOutput ? ~outState : outState;
    }

    uint8_t readAllPcfRaw(){
      auto res = Wire.requestFrom(inputAddress, uint8_t(1));
      if(res != 1){
        Serial.print("ERROR PCF read raw: ");
        Serial.println(res);
        return 0;
      }
      return Wire.read();
    }

    bool readPcfBit(uint8_t pos){
      static unsigned int errorDelay = 0;
       auto res = Wire.requestFrom(inputAddress, uint8_t(1));
        if(res != 1 && errorDelay < millis()){
          errorDelay = millis() + 5000;
          Serial.print("ERROR PCF read: ");
          Serial.println(res);
          return false;
        }
        return readInv() & (1 << pos);
    }

    bool readIdle(){
      if(usePcf){
       return readPcfBit(PcfRobotIdle);
      }
      else{
        return readInvDigital(PinRobotIdle);
      }
    }
    
    bool readAck(){
      if(usePcf){
       return readPcfBit(PcfRobotAck);
      }
      else{
        return readInvDigital(PinRobotAck);
      }
    }

    void printPcfPins(){
      Serial.println("Pcf -> Robot");
      for(int i=0; i<CmdPinsCount; i++){
        Serial.printf("[cmd%d] Cmd%d  -  %d (%d)\n", i, i, PcfCmd[i], PcfToRobotPin(PcfCmd[i], true));
      }
      Serial.printf("[send] Send  -  %d (%d)\n", PcfSend, PcfToRobotPin(PcfSend, true));
      Serial.println("Robot -> Pcf");
      Serial.printf("[idle] Idle  -  %d (%d)\n", PcfRobotIdle, PcfToRobotPin(PcfRobotIdle, false));
      Serial.printf("[ack]  Ack   -  %d (%d)\n", PcfRobotAck, PcfToRobotPin(PcfRobotAck, false));
    }
    
    void assignPcfPin(String name, int pcfPinNumber){
      if (name.startsWith("cmd"))  PcfCmd[name[3] - '0'] = pcfPinNumber;
      else if(name == "send") PcfSend      = pcfPinNumber;
      else if(name == "idle") PcfRobotIdle = pcfPinNumber;
      else if(name == "ack")  PcfRobotAck  = pcfPinNumber;
      else Serial.println("Unknown port name");
      setupPins();
    }

    bool setupPins(){
      if(usePcf){
        Wire.begin(SDA, SCL);
        outState = 0;
        Wire.beginTransmission(outputAddress);
        writeInv(outState);
        auto resOut = Wire.endTransmission();
        Wire.beginTransmission(inputAddress);
        Wire.write(0xFF);
        auto resIn  = Wire.endTransmission();
        if(resOut){
          Serial.print("ERROR PCF setup Out: ");
          Serial.println(resOut);
        }
        if(resIn){
          Serial.print("ERROR PCF setup In:  ");
          Serial.println(resIn);
        }
        return !(resOut || resIn);
      }else{
        //Wire.end();
        pinMode(PinRobotIdle, INPUT);
        pinMode(PinSend, OUTPUT);
        writeInvDigital(PinSend, 0);
        for(int i=0; i<CmdPinsCount; i++){
            pinMode(PinsCmd[i], OUTPUT);
            writeInvDigital(PinsCmd[i], 0);
        }
        if(PinRobotAck >= 0){
            pinMode(PinRobotAck, INPUT);
        }
      }
      return true;
   }
  
};

inline IOManager ioManager;
