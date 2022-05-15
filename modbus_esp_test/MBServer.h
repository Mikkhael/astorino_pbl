#pragma once
#include <ModbusTCP.h>
#include "Config.h"

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
          Serial.printf("%s READ   0x%.4X = 0x%.4X\n", type, reg->address.address, reg->value);
    }
    void debugWrite(const char* type, TRegister* reg, uint16_t val){
        if(logWrites)
          Serial.printf("%s WRITE  0x%.4X = 0x%.4X -> 0x%.4X\n", type, reg->address.address, reg->value, val);
    }

    ModbusTCPWrapper mb;

    uint16_t CmdHRegOffset = 100;
    //... CmdArgs 101, 102

    uint16_t ResetCoilOffset = 101;
    
    uint16_t RobotIdleIstsOffset  = 100;
    uint16_t QueueEmptyIstsOffset = 101;
    uint16_t QueueFullIstsOffset  = 102;
    
    uint16_t ExecutedCmdsIregOffset  = 101;

private:
    TRegister* CmdHReg[1+CmdArgsCount];
    TRegister* ResetCoil;
    TRegister* RobotIdleIsts;
    TRegister* QueueEmptyIsts;
    TRegister* QueueFullIsts;
    TRegister* ExecutedCmdsIreg;
public:

    bool newCmd = false;
    bool requestedReset = false;

    uint16_t& cmdPart(uint16_t offset){
        return CmdHReg[offset]->value;
    }
    void setRobotIdle(bool value){ RobotIdleIsts->value = ISTS_VAL(value); }
    bool getRobotIdle(){ return ISTS_BOOL(RobotIdleIsts->value); }
    void setQueueEmpty(bool value){ QueueEmptyIsts->value = ISTS_VAL(value); }
    void setQueueFull(bool value){ QueueFullIsts->value = ISTS_VAL(value); }
    void setExecutedCmds(uint16_t value){ ExecutedCmdsIreg->value = value; }
    
    Msg getFullMsg(){
        Msg res;
        for(int i=0; i<CmdArgsCount+1; i++){
            res.parts[i] = cmdPart(i);
        }
        return res;
    }

    void createAndBindIsts(uint16_t offset, TRegister*& tregptr){
        mb.addIsts(offset, false);
        mb.onGetIsts(offset, [this](TRegister* reg, uint16_t val){
            debugRead("Ists", reg, val);
            return reg->value;  
        });
        tregptr = mb.searchRegister(ISTS(offset));
    }
    void createAndBindIreg(uint16_t offset, TRegister*& tregptr){
        mb.addIreg(offset, 0);
        mb.onGetIreg(offset, [this](TRegister* reg, uint16_t val){
            debugRead("Ireg", reg, val);
            return reg->value;  
        });
        tregptr = mb.searchRegister(IREG(offset));
    }
    void createAndBindCoil(uint16_t offset, TRegister*& tregptr){
        mb.addCoil(offset, false);
        mb.onGetCoil(offset, [this](TRegister* reg, uint16_t val){
            debugRead("Coil", reg, val);
            return reg->value;
        });
        tregptr = mb.searchRegister(COIL(offset));
    }

    std::function<void(void)> onNewCmd;

    void init(){
        mb.server();
        /// HREG
        mb.addHreg(CmdHRegOffset, 0, CmdArgsCount+1);
        mb.onGetHreg(CmdHRegOffset, [this](TRegister* reg, uint16_t val){
            debugRead("Hreg", reg, val);
            return reg->value;  
        }, CmdArgsCount+1);
        mb.onSetHreg(CmdHRegOffset, [this](TRegister* reg, uint16_t val){
            debugWrite("Hreg", reg, val);
            newCmd = true;
            return val;  
        });
        mb.onSetHreg(CmdHRegOffset+1, [this](TRegister* reg, uint16_t val){
            debugWrite("Hreg", reg, val);
            return val;  
        }, CmdArgsCount);
        /// COIL
        createAndBindCoil(ResetCoilOffset, ResetCoil);
        mb.onSetCoil(ResetCoilOffset, [this](TRegister* reg, uint16_t val){
            debugWrite("Coil", reg, val);
            requestedReset = true;
            return val;
        });
        /// ISTS
        createAndBindIsts(RobotIdleIstsOffset, RobotIdleIsts);
        createAndBindIsts(QueueEmptyIstsOffset, QueueEmptyIsts);
        createAndBindIsts(QueueFullIstsOffset, QueueFullIsts);
        /// IREG
        createAndBindIreg(ExecutedCmdsIregOffset, ExecutedCmdsIreg);

        for(uint16_t i=0; i<CmdArgsCount+1; i++){
            CmdHReg[i] = mb.searchRegister(HREG(CmdHRegOffset + i));
        }

        mb.onConnect([](IPAddress address){
            Serial.printf("Modbus Connection: %s\n", address.toString().c_str());
            return true;
        });
        mb.onDisconnect([](IPAddress address){
            (void)address;
            Serial.printf("Modbus Disconnect\n");
            return true;
        });
    }

    void loop(){
        mb.task();
        if(newCmd){
            onNewCmd();
            newCmd = false;
        }
    }

};
