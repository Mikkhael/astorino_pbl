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

void printBinaryUint8(uint8_t val){
  for(int i=0; i<8; i++){
    Serial.print(val & (1 << (7 - i)) ? '1' : '0');
  }
}

class DIOMap{
  constexpr static int PinsCount = 8;
  constexpr static int OutFunctionsCount = 11;
  constexpr static int FunctionsCount  = 23;
public:
  enum class Function : int {OMotorOn, OCycleStart, OReset, OHold, OCycleStop, OMotorOff, OZeroing, OInterrupt, // 8 DIO
                              OSend, OCmd1, OCmd2,                                                              // 3 Custom
                             ICycle, IRepeat, ITeach, IMotorOn, IESTOP, IReady, IError, IHold, IHome, IZeroed,  // 10 DIO
                              IIdle, IAck,                                                                      // 2 Custom
                             NONE};
  constexpr static char* FunctionNames[FunctionsCount+1] {
    "MotorOn", "CycleStart", "Reset", "Hold", "CycleStop", "MotorOff", "Zeroing", "Interrupt",
    "Send", "Cmd1", "Cmd2",
    "Cycle", "Repeat", "Teach", "MotorOn", "ESTOP",  "Ready", "Error", "Hold", "Home", "Zeroed",
    "Idle", "Ack",
    "NONE"};
private:
  static_assert(static_cast<int>(Function::OMotorOn) == 0);
  static_assert(static_cast<int>(Function::ICycle) == OutFunctionsCount);
  static_assert(static_cast<int>(Function::NONE) == FunctionsCount);
  
  int pcfPinToFunctionMap[PinsCount];
  int functionToPcfPinMap[FunctionsCount];
public:

  static constexpr char* getFunctionName(Function function){
    return FunctionNames[static_cast<int>(function)];
  }
  static Function getFunctionFromName(String name, bool isInput){
    for(int i = isInput ? OutFunctionsCount : 0; i < FunctionsCount; i++){
      if(name == FunctionNames[i])
        return static_cast<Function>(i);
    }
    return Function::NONE;
  }
  static void printFunction(Function function){
    Serial.print(getFunctionName(function));
  }

  int getPcfPin(Function function){
    return functionToPcfPinMap[static_cast<int>(function)];
  }
  Function getFunction(int pcfPin){
    return static_cast<Function>(pcfPinToFunctionMap[pcfPin]);
  }

  DIOMap(){
    for(int i=0; i<PinsCount; i++){
      pcfPinToFunctionMap[i] = static_cast<int>(Function::NONE);
    }
    for(int i=0; i<FunctionsCount; i++){
      functionToPcfPinMap[i] = -1;
    }
  }

  void assignFunctionToPcfPin(Function function, int pcfPin){
    int functionInt = static_cast<int>(function);
    functionToPcfPinMap[functionInt] = pcfPin;
    pcfPinToFunctionMap[pcfPin] = functionInt;
  }
};

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
    bool invertedOutput = false;
    bool invertedInput  = true;
    
// GPIO pins
    int PinsCmd[CmdPinsCountMax] {D1, D2};
    int PinSend = D0;
    int PinRobotIdle = D5;
    int PinRobotAck  = D6;

// PCF pins

    // Output
    uint8_t outputAddress = 0x38;
    DIOMap dedicatedOutputs;

    uint8_t outState = 0xFF;
    
    // Input
    uint8_t inputAddress = 0x39;
    DIOMap dedicatedInputs;

    IOManager(){
      dedicatedOutputs.assignFunctionToPcfPin(DIOMap::Function::OMotorOn, RobotToPcfPin(1003));
      dedicatedOutputs.assignFunctionToPcfPin(DIOMap::Function::OZeroing, RobotToPcfPin(1004));
      dedicatedOutputs.assignFunctionToPcfPin(DIOMap::Function::OReset,   RobotToPcfPin(1005));

      dedicatedOutputs.assignFunctionToPcfPin(DIOMap::Function::OSend,    RobotToPcfPin(1008));
      dedicatedOutputs.assignFunctionToPcfPin(DIOMap::Function::OCmd1,     RobotToPcfPin(1001));
      dedicatedOutputs.assignFunctionToPcfPin(DIOMap::Function::OCmd2,     RobotToPcfPin(1002));

      dedicatedInputs.assignFunctionToPcfPin(DIOMap::Function::IError, RobotToPcfPin(7));

      dedicatedInputs.assignFunctionToPcfPin(DIOMap::Function::IIdle, RobotToPcfPin(1));
      dedicatedInputs.assignFunctionToPcfPin(DIOMap::Function::IAck,  RobotToPcfPin(2));
      
    }

    bool setDio(DIOMap::Function function, bool value){
      int pcfPin = dedicatedOutputs.getPcfPin(function);
      if(pcfPin == -1){
        Serial.print("Pin for function ");
        DIOMap::printFunction(function);
        Serial.println(" is not assigned");
        return false;
      }
      return writePcfPin(pcfPin, value);
    }
    bool getDio(DIOMap::Function function, bool& valueOut){
      int pcfPin = dedicatedInputs.getPcfPin(function);
      if(pcfPin == -1){
        Serial.print("Pin for function ");
        DIOMap::printFunction(function);
        Serial.println(" is not assigned");
        return false;
      }
      return readPcfPin(pcfPin);
    }

    auto writeInv(){
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
    
    bool refreshOutState(bool noDelayError = true){
      static unsigned int errorDelay = 0;
      Wire.beginTransmission(outputAddress);
      writeInv();
      auto res = Wire.endTransmission();
      if(res != 0 && (noDelayError || errorDelay < millis()) ){
        errorDelay = millis() + 5000;
        Serial.print("ERROR PCF write: ");
        Serial.println(res);
        return false;
      }
      return true;
    }
    bool writePcfPin(uint8_t pin, bool val, bool raw = false, bool noDelayError = true){
        bool toSet = (raw && invertedOutput) ? !val : val;
        setbit(outState, pin, toSet);
        return refreshOutState(noDelayError);
    }

    uint8_t getRawOutState(){
      return invertedOutput ? ~outState : outState;
    }
    
    uint8_t readPcfAll(bool raw = false, bool noDelayError = true){
      static unsigned int errorDelay = 0;
      auto res = Wire.requestFrom(inputAddress, uint8_t(1));
      if(res != 1 && (noDelayError || errorDelay < millis())){
        errorDelay = millis() + 5000;
        Serial.print("ERROR PCF read: ");
        Serial.println(res);
        return 0;
      }
      return (raw || !invertedInput) ? Wire.read() : ~Wire.read();
    }

    bool readPcfPin(uint8_t pos, bool noDelayError = true){
      uint8_t res = readPcfAll(false, noDelayError);
      return res & (1 << pos);
    }

    bool setCmd(uint8_t cmd){
      if(usePcf){
        setbit(outState, dedicatedOutputs.getPcfPin(DIOMap::Function::OCmd1), cmd & 0x01);
        if(CmdPinsCount >= 2)
          setbit(outState, dedicatedOutputs.getPcfPin(DIOMap::Function::OCmd2), cmd & 0x02);
        return refreshOutState();
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
        return writePcfPin(dedicatedOutputs.getPcfPin(DIOMap::Function::OSend), val);
      }
      else{
        writeInvDigital(PinSend, val);
      }
      return true;
    }

    bool readIdle(bool noDelayError = true){
      if(usePcf){
       return readPcfPin(dedicatedInputs.getPcfPin(DIOMap::Function::IIdle), noDelayError);
      }
      else{
        return readInvDigital(PinRobotIdle);
      }
    }
    
    bool readAck(bool noDelayError = true){
      if(usePcf){
       return readPcfPin(dedicatedInputs.getPcfPin(DIOMap::Function::IAck), noDelayError);
      }
      else{
        return readInvDigital(PinRobotAck);
      }
    }

    void printPcfPins(){
      Serial.println("===== Pcf -> Robot =====");
      for(int i=0; i<8; i++){
        DIOMap::Function function = dedicatedOutputs.getFunction(i);
        if(function != DIOMap::Function::NONE){
          auto name = DIOMap::getFunctionName(function);
          bool val = outState & (1 << i);
          bool valRaw = invertedOutput ? !val : val;
          Serial.printf("%s\t(%d (%d))\t = %d [%d]\n", name, i, PcfToRobotPin(i, true), val, valRaw);
        }
      }
      uint8_t inState = readPcfAll();
      Serial.println("===== Robot -> Pcf =====");
      for(int i=0; i<8; i++){
        DIOMap::Function function = dedicatedInputs.getFunction(i);
        if(function != DIOMap::Function::NONE){
          auto name = DIOMap::getFunctionName(function);
          bool val = inState & (1 << i);
          bool valRaw = invertedInput ? !val : val;
          Serial.printf("%s\t(%d (%d))\t = %d [%d]\n", name, i, PcfToRobotPin(i, false), val, valRaw);
        }
      }
    }
    
    void assignPcfPin(String name, bool isInput, int pcfPinNumber){
      DIOMap::Function function = DIOMap::getFunctionFromName(name, isInput);
      if(function == DIOMap::Function::NONE){
        Serial.println("Unrecognized function name.");
        return;
      }
      if(isInput){
        dedicatedInputs.assignFunctionToPcfPin(function, pcfPinNumber);
      }else{
        dedicatedOutputs.assignFunctionToPcfPin(function, pcfPinNumber);
      }
      setupPins();
    }
    bool setupPins(){
      if(usePcf){
        Wire.begin(SDA, SCL);
        outState = 0;
        Wire.beginTransmission(outputAddress);
        writeInv();
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
