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
template<typename T>
static bool getbit(const T& var, int pin){
  return var & (1 << pin);
}

template<typename T>
constexpr int toint(const T& t) {return static_cast<int>(t);};

void printBinaryUint8(uint8_t val){
  for(int i=0; i<8; i++){
    Serial.print(val & (1 << (7 - i)) ? '1' : '0');
  }
}

struct DIO{

  constexpr static int PinsCount = 9;
  constexpr static int OutFunctionsCount = 8+3;
  constexpr static int InFunctionsCount = 10+3;
  constexpr static int GPIOFunctionsCount  = 5;
  constexpr static int FunctionsCount  = OutFunctionsCount+InFunctionsCount+GPIOFunctionsCount;
  enum class Function : int {OMotorOn, OCycleStart, OReset, OHold, OCycleStop, OMotorOff, OZeroing, OInterrupt, // 8 DIO
                              OSend, OCmd1, OCmd2,                                                              // 3 Custom
                             ICycle, IRepeat, ITeach, IMotorOn, IESTOP, IReady, IError, IHold, IHome, IZeroed,  // 10 DIO
                              IIdle, IAck, IGrab,                                                               // 3 Custom
                             GGrab, GTestButt, GFar1, GTens, GGrabbed,                                          // 5 GPIO
                             NONE};

  constexpr static const char* FunctionNames[FunctionsCount+1] {
    "OMotorOn", "OCycleStart", "OReset", "OHold", "OCycleStop", "OMotorOff", "OZeroing", "OInterrupt",
    "OSend", "OCmd1", "OCmd2",
    "ICycle", "IRepeat", "ITeach", "IMotorOn", "IESTOP", "IReady", "IError", "IHold", "IHome", "IZeroed",
    "IIdle", "IAck", "IGrab",
    "GGrab", "GTestButt", "GFar1", "GTens", "GGrabbed",
    "NONE"};

  static constexpr const char* getFunctionName(DIO::Function function){
    return FunctionNames[toint(function)];
  }
  static constexpr Function getFunctionFromIndex(int i){
    return static_cast<DIO::Function>(i);
  }
  static Function getFunctionFromName(String str){
    int i=0;
    for(i=0; i<FunctionsCount; i++){
      if(str == FunctionNames[i]){
        break;
      }
    }
    return getFunctionFromIndex(i);
  }

  constexpr static int TypesCount = 4;
  enum class Type: int {RobotIn, RobotOut, GPIO, BoardPcf, NONE};
  
  enum class PinMode {Unused, Input, Output};
  
  static_assert(static_cast<int>(Function::OMotorOn) == 0);
  static_assert(static_cast<int>(Function::ICycle) == OutFunctionsCount);
  static_assert(static_cast<int>(Function::NONE) == FunctionsCount);
  static_assert(static_cast<int>(Type::RobotIn) == 0);
  static_assert(static_cast<int>(Type::NONE) == TypesCount);
  
  struct Module{
    static constexpr int MaxPinsCount = 10;
    int PinsCount = 0;
    uint16_t lastInState = 0;
    uint16_t outState = 0;
    DIO::PinMode pinModes[MaxPinsCount];
    DIO::Function pinFunctions[MaxPinsCount];
    bool isInverted[MaxPinsCount];
    Module(){
      for(int i=0; i<MaxPinsCount; i++){
        pinModes[i] = PinMode::Unused;
        pinFunctions[i] = DIO::Function::NONE;
      }
    }
    void setPin(int pin, DIO::PinMode mode = DIO::PinMode::Unused, DIO::Function function = DIO::Function::NONE, bool inverted = false){
      if(pin >= PinsCount){
        return;
      }
      pinModes[pin] = mode;
      pinFunctions[pin] = function;
      isInverted[pin] = inverted;
    }

    bool forceFreshAndFlush = false;
    std::function<int(int, Module*)> read_new_raw;
    std::function<int(int, bool, Module*)> write_new_raw;

    int read(int pin, bool* out, bool fresh = true, bool raw = false){
      int res = 0;
      if(fresh || forceFreshAndFlush){
        res = read_new_raw(pin, this);
      }
      *out = getbit(lastInState, pin);
      if(!raw && isInverted[pin])
        *out = !(*out);
      return res;
    }
    bool readVal(int pin, bool fresh = true, bool raw = false){
      bool val;
      read(pin, &val, fresh, raw);
      return val;
    }
    int readall(){
      return read_new_raw(0, this);
    }

    int write(int pin, bool value, bool raw = false,bool flush = true){
      bool val = raw ? value: (value != isInverted[pin]);
      if(pin >= 0)
        setbit(outState, pin, val);
      if(flush || forceFreshAndFlush){
        return write_new_raw(pin, val, this);
      }
      return 0;
    }
  };

  Module modules[TypesCount];
  Module& getModule(DIO::Type type) {return modules[toint(type)]; };

  struct FunctionMapping{
    DIO::Type type = DIO::Type::NONE;
    int pin = 0;
  };

  FunctionMapping functionToPinMapping[FunctionsCount];
  FunctionMapping& getMapping(DIO::Function function) {return functionToPinMapping[toint(function)]; };
  
  void disassignFunction(DIO::Function function){
    auto& mapping = functionToPinMapping[toint(function)];
    if(mapping.type != DIO::Type::NONE){
      getModule(mapping.type).setPin(mapping.pin);
    }
    functionToPinMapping[toint(function)] = {DIO::Type::NONE, 0};
  }
  void assignFunctionToPin(DIO::Function function, DIO::PinMode mode, bool inverted, DIO::Type type, int pin){
    auto& mapping = functionToPinMapping[toint(function)];
    if(mapping.type != DIO::Type::NONE){
      getModule(mapping.type).setPin(mapping.pin);
    }
    getModule(type).setPin(pin, mode, function, inverted);
    functionToPinMapping[toint(function)] = {type, pin};
  }

  void onUnassignedFunction(DIO::Function function, bool fromRead){
    Serial.printf("ERROR: Attempted to %s unassigned function %s [%d]", 
                    fromRead ? "read" : "write", getFunctionName(function), toint(function));
  }

  auto read(DIO::Function function, bool* out, bool fresh = true, bool raw = false){
    auto& mapping = getMapping(function);
    if(mapping.type == DIO::Type::NONE){
      onUnassignedFunction(function, true);
      return -10;
    }
    return getModule(mapping.type).read(mapping.pin, out, fresh, raw);
  }
  
  bool readVal(DIO::Function function, bool fresh = true, bool raw = false){
    auto& mapping = getMapping(function);
    if(mapping.type == DIO::Type::NONE){
      onUnassignedFunction(function, true);
      return false;
    }
    return getModule(mapping.type).readVal(mapping.pin, fresh, raw);
  }
  
  enum class State : uint8_t {NONE = 0, ON = 1, OFF = 2};
  State readState(DIO::Function function, bool raw = false){
    auto& mapping = getMapping(function);
    if(mapping.type == DIO::Type::NONE){
      //onUnassignedFunction(function, true);
      return State::NONE;
    }
    bool val = getModule(mapping.type).readVal(mapping.pin, false, raw);
    return val ? State::ON : State::OFF;
  }

  auto write(DIO::Function function, bool value, bool raw = false,bool flush = true){
    auto& mapping = getMapping(function);
    if(mapping.type == DIO::Type::NONE){
      onUnassignedFunction(function, false);
      return -10;
    }
    return getModule(mapping.type).write(mapping.pin, value, raw, flush);
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
    int  CmdPinsCount   = 2;
    bool invertedOutput = false;
    bool invertedInput  = true;
    
    constexpr static int GPIOPinsMapCount = 9;
    constexpr static int GPIOPinsMap[GPIOPinsMapCount] = {
      D0, D1, D2, D3, D4, D5, D6, D7, D8
    };

// PCF pins

    bool isPcfCommunicationHalted = false;
    bool wasNoErrorWrite = true;
    bool wasNoErrorRead  = true;
    // Output
    uint8_t outputAddress = 0x38;
    uint8_t inputAddress  = 0x39;
    uint8_t boardPcfAddress = 0x3A;
    struct PcfState{
      int lastErrorRead = 0;
      int lastErrorWrite = 0;
      bool isHalted = false;
      bool showError = true;
      unsigned int errorDelay = 5000;
      unsigned int lastErrorTime = 0;
    };
    static constexpr int PcfsCount = 3;
    PcfState pcfStates[PcfsCount];

    void setShowPcfError(bool val){
        for(auto& state : pcfStates){
          state.showError = val;
        }
    }

    DIO dio;



    auto createPcfWriteFunction(uint8_t address, int pcfStateIndex, bool isInputModule = false){
      return [address, &pcfState = this->pcfStates[pcfStateIndex], isInputModule](int pin, bool val, DIO::Module* module){
        (void) pin;
        (void) val;
        if(pcfState.isHalted){
          return pcfState.lastErrorWrite = 0;
        }
        Wire.beginTransmission(address);
        if(module->pinModes[pin] == DIO::PinMode::Input){
          setbit(module->outState, pin, 1);
        }
        if(isInputModule){
          Wire.write((uint8_t)0xFF);
        }else{
          Wire.write((uint8_t)module->outState);
        }
        auto res = Wire.endTransmission();
        pcfState.lastErrorWrite = res;
        if(res != 0){
          if(pcfState.showError || millis() - pcfState.lastErrorTime >= pcfState.errorDelay){
            pcfState.lastErrorTime = millis();
            Serial.printf("ERROR PCF [%d] WRITE: %d\n", address, res);
          }
          return pcfState.lastErrorWrite = res;
        }
        return pcfState.lastErrorWrite = 0;
      };
    }
    auto createPcfReadFunction(uint8_t address, int pcfStateIndex){
      return [address, &pcfState = this->pcfStates[pcfStateIndex]](int pin, DIO::Module* module){
        (void) pin;
        if(pcfState.isHalted){
          return pcfState.lastErrorRead = 0;
        }
        auto res = int(Wire.requestFrom(address, uint8_t(1))) - 1;
        pcfState.lastErrorRead = res;
        if(res != 0){
          if(pcfState.showError || millis() - pcfState.lastErrorTime >= pcfState.errorDelay){
            pcfState.lastErrorTime = millis();
            Serial.printf("ERROR PCF [%d] READ:  %d\n", address, res);
          }
          return pcfState.lastErrorRead = res;
        }
        module->lastInState = Wire.read(); 
        return pcfState.lastErrorRead = 0;
      };
    }

    IOManager(){
      dio.getModule(DIO::Type::RobotIn).PinsCount = 8;
      dio.getModule(DIO::Type::RobotOut).PinsCount = 8;
      dio.getModule(DIO::Type::BoardPcf).PinsCount = 8;
      dio.getModule(DIO::Type::GPIO).PinsCount = 9;
      dio.getModule(DIO::Type::GPIO).forceFreshAndFlush = true;

      dio.getModule(DIO::Type::RobotIn).read_new_raw  = createPcfReadFunction(outputAddress, 0);
      dio.getModule(DIO::Type::RobotIn).write_new_raw = createPcfWriteFunction(outputAddress, 0);
      
      dio.getModule(DIO::Type::RobotOut).read_new_raw  = createPcfReadFunction(inputAddress, 1);
      dio.getModule(DIO::Type::RobotOut).write_new_raw = createPcfWriteFunction(inputAddress, 1, true);
      
      dio.getModule(DIO::Type::BoardPcf).read_new_raw  = createPcfReadFunction(boardPcfAddress, 2);
      dio.getModule(DIO::Type::BoardPcf).write_new_raw = createPcfWriteFunction(boardPcfAddress, 2, true);
      
      dio.getModule(DIO::Type::GPIO).read_new_raw  = [](int pin, DIO::Module* module) {
        int dpin = GPIOPinsMap[pin];
        bool res = digitalRead(dpin);
        //Serial.printf("READING PIN GPIO %d (D8==%d (%d)) = %d\n", dpin, dpin==D8, D8, res);
        setbit(module->lastInState, pin, res);
        return 0;
      };
      dio.getModule(DIO::Type::GPIO).write_new_raw = [](int pin, bool val, DIO::Module* module) {
        (void) module;
        int dpin = GPIOPinsMap[pin];
        //Serial.printf("WRITING PIN GPIO %d (D5==%d) TO %d\n", dpin, dpin==D5, val);
        digitalWrite(dpin, val);
        return 0;
      };

      dio.assignFunctionToPin(DIO::Function::OMotorOn, DIO::PinMode::Output, invertedOutput, DIO::Type::RobotIn,  RobotToPcfPin(1003));
      dio.assignFunctionToPin(DIO::Function::OZeroing, DIO::PinMode::Output, invertedOutput, DIO::Type::RobotIn,  RobotToPcfPin(1004));
      dio.assignFunctionToPin(DIO::Function::OReset,   DIO::PinMode::Output, invertedOutput, DIO::Type::RobotIn,  RobotToPcfPin(1005));
       
      dio.assignFunctionToPin(DIO::Function::OSend,    DIO::PinMode::Output, invertedOutput, DIO::Type::RobotIn,  RobotToPcfPin(1008));
      dio.assignFunctionToPin(DIO::Function::OCmd1,    DIO::PinMode::Output, invertedOutput, DIO::Type::RobotIn,  RobotToPcfPin(1001));
      dio.assignFunctionToPin(DIO::Function::OCmd2,    DIO::PinMode::Output, invertedOutput, DIO::Type::RobotIn,  RobotToPcfPin(1002));

      dio.assignFunctionToPin(DIO::Function::IError,   DIO::PinMode::Input,  invertedInput,  DIO::Type::RobotOut, RobotToPcfPin(7));
      dio.assignFunctionToPin(DIO::Function::IIdle,    DIO::PinMode::Input,  invertedInput,  DIO::Type::RobotOut, RobotToPcfPin(1));
      dio.assignFunctionToPin(DIO::Function::IAck,     DIO::PinMode::Input,  invertedInput,  DIO::Type::RobotOut, RobotToPcfPin(2));
      dio.assignFunctionToPin(DIO::Function::IGrab,    DIO::PinMode::Input,  invertedInput,  DIO::Type::RobotOut, RobotToPcfPin(3));

      dio.assignFunctionToPin(DIO::Function::GGrab,     DIO::PinMode::Output,  true,   DIO::Type::GPIO,     5);
      dio.assignFunctionToPin(DIO::Function::GTestButt, DIO::PinMode::Output,  false,  DIO::Type::GPIO,     6);
      
      dio.assignFunctionToPin(DIO::Function::GTens,    DIO::PinMode::Input,   false,  DIO::Type::BoardPcf,     0);
      dio.assignFunctionToPin(DIO::Function::GGrabbed, DIO::PinMode::Input,   false,  DIO::Type::BoardPcf,     1);
      dio.assignFunctionToPin(DIO::Function::GFar1,    DIO::PinMode::Input,   true,   DIO::Type::BoardPcf,     2);
    }
    
    bool refreshOutState(bool noDelayError = true){
      if(!noDelayError) setShowPcfError(false);
      dio.getModule(DIO::Type::RobotIn).write(-1, false);
      dio.getModule(DIO::Type::RobotOut).write(-1, false);
      dio.getModule(DIO::Type::BoardPcf).write(-1, false);
      if(!noDelayError) setShowPcfError(true);
      for(auto& state : pcfStates){
        if(state.lastErrorWrite != 0){
          return false;
        }
      }
      return true;
    }

    bool refreshInState(bool noDelayError = true){
      if(!noDelayError) setShowPcfError(false);
      dio.getModule(DIO::Type::RobotIn).readall();
      dio.getModule(DIO::Type::RobotOut).readall();
      dio.getModule(DIO::Type::BoardPcf).readall();
      if(!noDelayError) setShowPcfError(true);
      for(auto& state : pcfStates){
        if(state.lastErrorRead != 0){
          return false;
        }
      }
      return true;
    }

    // Dedicated Commands
    bool setCmd(uint8_t cmd){
      dio.write(DIO::Function::OCmd1, cmd & 0x01, false, false);
      if(CmdPinsCount >= 2)
        dio.write(DIO::Function::OCmd2, cmd & 0x02, false, false);
      return refreshOutState();
    }



    void printPcfPins(){
      auto& inModule = dio.getModule(DIO::Type::RobotIn);
      auto& outModule = dio.getModule(DIO::Type::RobotOut);
      auto& boardModule = dio.getModule(DIO::Type::BoardPcf);
      auto& gpioModule = dio.getModule(DIO::Type::GPIO);
      auto show = [](DIO::Module& module, int addInfo){
        for(int i=0; i<module.PinsCount; i++){
          auto func = module.pinFunctions[i];
          if(func != DIO::Function::NONE){
            int pin = i;
            if(addInfo < 2){
              pin = PcfToRobotPin(i, addInfo == 1);
            }else if(addInfo == 2){
              pin = GPIOPinsMap[i];
            }
            Serial.printf("%s %s [%d (%d)]\t = %d\n",
              module.pinModes[i] == DIO::PinMode::Input ? "IN: " : "OUT:",
              DIO::getFunctionName(module.pinFunctions[i]),
              i,
              pin,
              module.readVal(i));
          }
        }
      };
      Serial.println("======== Robot -> Pcf ======="); show(outModule,   1);
      Serial.println("======== Pcf -> Robot ======="); show(inModule,  0);
      Serial.println("========  Board PCF   ======="); show(boardModule,  3);
      Serial.println("========     GPIO     ======="); show(gpioModule, 2);
    }
    
    bool setupPins(){
      auto gpio = dio.getModule(DIO::Type::GPIO);
      for(int i=0; i<gpio.PinsCount; i++){
        if(gpio.pinModes[i] == DIO::PinMode::Unused)
          continue;
        auto mode = ((gpio.pinModes[i] == DIO::PinMode::Input) ? INPUT : OUTPUT);
        pinMode(GPIOPinsMap[i], mode);
        if(mode == OUTPUT){
          gpio.write(i, false);
        }
      }

      Wire.begin(SDA, SCL);
      return refreshOutState();
   }
   
   void loop(){
    refreshOutState(false);
    bool success = refreshInState(false);

    if(success){
      dio.write(DIO::Function::GGrab, dio.readVal(DIO::Function::IGrab, false));
    }else{
      dio.write(DIO::Function::GGrab, false, true);
    }
   }
  
};

inline IOManager ioManager;
