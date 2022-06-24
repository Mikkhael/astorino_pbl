#pragma once
#include <ModbusTCP.h>
#include "Config.h"
#include "Transfer.h"
#include <type_traits>

class ModbusTCPWrapper : public ModbusTCP{
public:
    TRegister* searchRegister(TAddress address){
        return Modbus::searchRegister(address);
    }
};

struct MBServer{

    bool logReads = false;
    bool logWrites = true;
    
    void debugRead(const char* type, TRegister* reg, uint16_t val){
        (void)val;
        if(logReads)
          Serial.printf("%s READ   %.4d = %.4d\n", type, reg->address.address, reg->value);
    }
    void debugWrite(const char* type, TRegister* reg, uint16_t val){
        if(logWrites)
          Serial.printf("%s WRITE  %.4d = %.4d -> %.4d\n", type, reg->address.address, reg->value, val);
    }
    const char* TAddressToString(TAddress& address){
        switch(address.type){
            case TAddress::COIL: return "COIL";
            case TAddress::HREG: return "HREG";
            case TAddress::ISTS: return "ISTS";
            case TAddress::IREG: return "IREG";
        }
        return "";
    }

    enum class RegName : int {
        Cmd = 0, CmdArg1, CmdArg2,
        Reset, TestButton,
        RobotIdle, QueueEmpty, QueueFull,
        GrabberClosed, ElementGrabbed, ButtonPressed,
        ExecutedCmds,
        COUNT
    };
    static constexpr int RegNameCount = 12;
    static_assert(RegNameCount == static_cast<int>(RegName::COUNT));

    ModbusTCPWrapper mb;

    TRegister* usedRegisters[RegNameCount];
    TAddress usedRegistersAddresses[RegNameCount];

    template<typename OnWrite = nullptr_t, typename OnRead = nullptr_t>
    void addReg(RegName name, TAddress address, OnWrite onWrite = nullptr, OnRead onRead = nullptr){
        int regIndex = static_cast<int>(name);
        auto success = mb.addReg(address, (uint16_t)0, 1);
        mb.onSet(address, [this, onWrite, type = TAddressToString(address)](TRegister* reg, uint16_t val){
            debugWrite(type, reg, val);
            if constexpr(std::is_same_v<OnWrite, nullptr_t>){
                return val;
            }else{
                uint16_t v = onWrite(reg, val);
                Serial.printf("%d WRITING: %d\n", reg->address.address, v);
                return v;
            }
        });
        mb.onGet(address, [this, onRead, type = TAddressToString(address)](TRegister* reg, uint16_t val){
            debugRead(type, reg, val);
            if constexpr(std::is_same_v<OnRead, nullptr_t>){
                return reg->value;
            }else{
                return onRead(reg, val);
            }
        });
//        Serial.print("new regindex: ");
//        Serial.print(regIndex);
//        Serial.print(" ");
//        Serial.print(success);
//        Serial.print(" ");
//        Serial.print((int)address.address);
//        Serial.print(" ");
//        Serial.print((int)mb.searchRegister(address));
//        Serial.println();
        //usedRegisters[regIndex] = mb.searchRegister(address);
        usedRegistersAddresses[regIndex] = address;
    }
    void loadTReggisters(){
      for(int i=0; i<RegNameCount; i++){
        usedRegisters[i] = mb.searchRegister(usedRegistersAddresses[i]);
      }
    }
    
    TRegister* getReg(RegName name){
        int regIndex = static_cast<int>(name);
//        Serial.print("Getting reg: ");
//        Serial.print(regIndex);
        //auto res = usedRegisters[regIndex];
        auto res = mb.searchRegister(usedRegistersAddresses[regIndex]);
//        Serial.print(" ");
//        Serial.print((int)res);
//        Serial.println();
        //delay(100);
        return res;
    }
    uint16_t get(RegName name){
        auto reg = getReg(name);
        return reg->value;
    }
//    uint16_t& getRef(RegName name){
//        auto reg = getReg(name);
//        return reg->value;
//    }
    void set(RegName name, uint16_t value){
        auto reg = getReg(name);
        reg->value = value;
    }
    void set(RegName name, bool value){
        auto reg = getReg(name);
        reg->value = value ? 0xFF00 : 0x0000;
    }


    bool newCmd = false;
    bool requestedReset = false;
    bool isTestButton(){
      return get(RegName::TestButton);
    }

    uint16_t cmdPart(uint16_t offset){
        auto name = RegName::Cmd;
        if(offset == 1) name = RegName::CmdArg1;
        if(offset == 2) name = RegName::CmdArg2;
        auto v = get(name);
        Serial.printf("CMD PART %d = %d\n", offset, v);
        return v;
    }
    
    Msg getFullMsg(){
//        Serial.printf("TEST 100: %d\n", mb.searchRegister(HREG(100)) == getReg(RegName::Cmd) );
//        Serial.printf("TEST 101: %d\n", mb.searchRegister(HREG(101)) == getReg(RegName::CmdArg1) );
//        Serial.printf("TEST 102: %d\n", mb.searchRegister(HREG(102)) == getReg(RegName::CmdArg2) );
        //delay(100);
        Msg res;
        for(int i=0; i<CmdArgsCount+1; i++){
            res.parts[i] = cmdPart(i);
        }
        return res;
    }

    void init(){
        mb.server();

        /// HREG
        addReg(RegName::Cmd, HREG(100), [this](TRegister* reg, uint16_t val){
            (void) reg;
            newCmd = true;
            return val;
        });
        addReg(RegName::CmdArg1, HREG(101));
        addReg(RegName::CmdArg2, HREG(102));

        /// COIL
        addReg(RegName::Reset, COIL(101), [this](TRegister* reg, uint16_t val){
            (void) reg;
            requestedReset = true;
            return val;
        });
        addReg(RegName::TestButton, COIL(102));

        /// ISTS
        addReg(RegName::RobotIdle,  ISTS(100));
        addReg(RegName::QueueEmpty, ISTS(102));
        addReg(RegName::QueueFull,  ISTS(103));
        
        addReg(RegName::GrabberClosed,  ISTS(50));
        addReg(RegName::ElementGrabbed, ISTS(51));
        addReg(RegName::ButtonPressed,  ISTS(52));

        /// IREG
        addReg(RegName::ExecutedCmds,  IREG(101));

        mb.onConnect([](IPAddress address){
            Serial.printf("Modbus Connection: %s\n", address.toString().c_str());
            return true;
        });
        mb.onDisconnect([this](IPAddress address){
            (void)address;
            auto ip = IPAddress(mb.eventSource());
            Serial.printf("Modbus Disconnect: %s\n", ip.toString().c_str());
            return true;
        });
        
        //loadTReggisters();
    }

    void loop(){
        mb.task();
    }

};
