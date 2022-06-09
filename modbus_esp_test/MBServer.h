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
    }

    enum class RegName : int {
        Cmd = 0, CmdArg1, CmdArg2, Reset, RobotIdle, QueueEmpty, QueueFull, ExecutedCmds
    };
    static constexpr int RegNameCount = 8;

    ModbusTCPWrapper mb;

    TRegister* usedRegisters[RegNameCount];

    template<typename OnWrite = nullptr_t, typename OnRead = nullptr_t>
    void addReg(RegName name, TAddress address, OnWrite onWrite = nullptr, OnRead onRead = nullptr){
        int regIndex = static_cast<int>(name);
        mb.addReg(address, (uint16_t)0, 1);
        mb.onSet(address, [this, onWrite, type = TAddressToString(address)](TRegister* reg, uint16_t val){
            debugWrite(type, reg, val);
            if constexpr(std::is_same_v<OnWrite, nullptr_t>){
                return val;
            }else{
                return onWrite(reg, val);
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
        usedRegisters[regIndex] = mb.searchRegister(address);
    }
    TRegister* getReg(RegName name){
        int regIndex = static_cast<int>(name);
        return usedRegisters[regIndex];
    }
    uint16_t get(RegName name){
        auto reg = getReg(name);
        return reg->value;
    }
    uint16_t& getRef(RegName name){
        auto reg = getReg(name);
        return reg->value;
    }
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

    uint16_t& cmdPart(uint16_t offset){
        auto name = RegName::Cmd;
        if(offset == 1) name = RegName::CmdArg1;
        if(offset == 2) name = RegName::CmdArg2;
        return getRef(name);
    }
    
    Msg getFullMsg(){
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
            newCmd = val;
            return val;
        });
        addReg(RegName::CmdArg1, HREG(101));
        addReg(RegName::CmdArg2, HREG(102));

        /// COIL
        addReg(RegName::Reset, COIL(101), [this](TRegister* reg, uint16_t val){
            requestedReset = true;
            return val;
        });

        /// ISTS
        addReg(RegName::RobotIdle,  ISTS(100));
        addReg(RegName::QueueEmpty, ISTS(102));
        addReg(RegName::QueueFull,  ISTS(103));

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
    }

    void loop(){
        mb.task();
    }

};
