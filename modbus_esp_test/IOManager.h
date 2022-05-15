#pragma once
#include <Wire.h>

struct IOManager{

    static constexpr int CmdPinsCountMax = 4;
    int CmdPinsCount = 2;
    bool usePcf         = true;
    bool invertedOutput = true;
    bool invertedInput  = true;
    
// GPIO pins
    int PinsCmd[CmdPinsCountMax] {D1, D2, D3, D4};
    int PinSend = D0;
    int PinRobotIdle = D5;
    int PinRobotAck  = D6;

// PCF pins

    // Output
    uint8_t outputAddress = 0x38;
    //uint8_t PcfCmd[CmdPinsCountMax] {0, 1, 2, 3};
    uint8_t PcfSend = 5;

    uint8_t outState = 0xFF;
    uint8_t cmdMask  = 0;
    
    // Input
    uint8_t inputAddress = 0x39;
    uint8_t PcfRobotIdle = 0;
    uint8_t PcfRobotAck  = 1;

    auto writeInv(uint8_t val){
      if(invertedOutput)
        return Wire.write(~outState);
      return Wire.write(outState);
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
          outState = (outState & (~cmdMask)) | (cmd & cmdMask);
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

    bool setSend(bool val){
      if(usePcf){
        bitWrite(outState, PcfSend, val);
        Wire.beginTransmission(outputAddress);
        writeInv(outState);
        auto res = Wire.endTransmission();
        if(res){
          Serial.print("ERROR PCF setSend: ");
          Serial.println(res);
          return false;
        }
      }
      else{
        writeInvDigital(PinSend, val);
      }
      return true;
    }

    bool readPcfBit(uint8_t pos){
       auto res = Wire.requestFrom(inputAddress, uint8_t(1));
        if(res != 1){
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


    bool setupPins(){
      if(usePcf){
        Wire.begin(SDA, SCL);
        cmdMask = (uint8_t(0x01) << CmdPinsCount) - 1;
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
