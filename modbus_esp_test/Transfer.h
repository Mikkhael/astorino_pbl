#pragma once
#include "Config.h"
#include "IOManager.h"


template<typename ...Args>
static void debugf(const Args& ...args){
    Serial.printf(args...);
}

struct Msg{
    uint16_t parts[CmdArgsCount+1];
    bool isDebug = false;
    
    static Msg Single(uint16_t value){ Msg res; res.parts[0] = value; return res; }
};

struct Sender{
public:

    int WordSize = 16;

    // Current State
    enum State {Idle, Single, Advanced, Alternating} state;
    bool awaitAck    = false;
    bool performDemo = false;
    unsigned long  demoLength  = 2000;
    unsigned long  demoStart   = 0;

    int currentBit   = 0;
    int bitsToSend   = 0;

    Msg msgToSend;

    bool robotIdle = false;
    bool ack = false;

    bool isBusy() {return state != State::Idle;}

    void init(){
        
    }

    void abort(){
      state = State::Idle;
    }

    void outputMsgSingle(){
        ioManager.dio.write(DIO::Function::OSend, false);
        ioManager.setCmd(msgToSend.parts[0]);
        ioManager.dio.write(DIO::Function::OSend, true);
        awaitAck = false;
        debugf("Await !IDLE.\n");
    }

    bool getMsgBit(int num){
        int part = num / WordSize;
        int bit  = num % WordSize;
        return msgToSend.parts[part] & (1 << bit);
    }
    void outputCurrentMsgPart(bool invert = false){
        ioManager.dio.write(DIO::Function::OSend, invert);
        uint8_t cmdPart = 0;
        for(int i=0; i<ioManager.CmdPinsCount && currentBit < bitsToSend; i++){
            auto bitValue = getMsgBit(currentBit);
            debugf("Cmd%d = Bit%.2d (%d)\n", i, currentBit, bitValue);
            cmdPart |= bitValue << i;
            ++currentBit;
        }
        ioManager.setCmd(cmdPart);
        ioManager.dio.write(DIO::Function::OSend, !invert);
        awaitAck = !invert;
        if(invert)
          debugf("Send = 0, Await !ACK\n");
        else
          debugf("Send = 1, Await ACK\n");
    }

    bool updateOutputCurrentMsgPart = false;
    bool beginSend(const Msg& msg, State mode){
      if(state != State::Idle)
        return false;
      if(performDemo){
        Serial.println("PERFORMING DEMO SEND CMD");
        demoStart = millis();
      }
      msgToSend    = msg;
      currentBit   = 0;
      awaitAck     = true;
      state        = mode;
      if(state == State::Single){
        debugf("Starting to send Single Cmd (0x%.4X). Await IDLE\n", msgToSend.parts[0]);
        return true;
      }
      bitsToSend = WordSize;
      debugf("Starting to send %s Cmd (0x%.4X | 0x%.4X | 0x%.4X ), length %d\n", state==State::Advanced ? "Advanced" : "Alternating", msgToSend.parts[0], msgToSend.parts[1], msgToSend.parts[2], bitsToSend);
      updateOutputCurrentMsgPart = true;
      return true;
    }

    bool wasLastMessageDebug = false;
    bool awaitForIdle = false;
    unsigned int executedCmds = 0;
    unsigned int executedDebugCmds = 0;
    void awaitForFinish(){
        wasLastMessageDebug = msgToSend.isDebug;
        awaitForIdle = true;
    }
    
    void loop(){
        robotIdle = ioManager.dio.readVal(DIO::Function::IIdle, false);
        if(awaitForIdle && robotIdle){
          awaitForIdle = false;
          if(wasLastMessageDebug){
            executedDebugCmds++;
          }else{
            executedCmds++;
          }
        }
        if(updateOutputCurrentMsgPart){
          updateOutputCurrentMsgPart = false;
          outputCurrentMsgPart();
        }
        if(performDemo){
          robotIdle = true;
          if(state != State::Idle){
            robotIdle = false;
            if(millis() - demoStart >= demoLength){
              robotIdle = true;
              Serial.println("Completed DEMO.");
              state = State::Idle;
              if(msgToSend.isDebug){
                executedDebugCmds++;
              }else{
                executedCmds++;
              }
            }
          }
        } else if(state == State::Single){
            if(awaitAck && robotIdle){
                outputMsgSingle();
            }else if(!awaitAck && !robotIdle){
                ioManager.dio.write(DIO::Function::OSend, false);
                state = State::Idle;
                debugf("Finished transmision.\n");
                awaitForFinish();
            }
        } else if(state == State::Advanced){
            bool robotAck = ioManager.dio.readVal(DIO::Function::IAck);
            if(awaitAck && robotAck){
                ioManager.dio.write(DIO::Function::OSend, false);
                awaitAck = false;
                debugf("Send = 0, Await !ACK\n");
            }else if(!awaitAck && !robotAck){
                if(currentBit >= bitsToSend){
                    state = State::Idle;
                    debugf("Finished transmision.\n");
                    awaitForFinish();
                }else{
                    debugf("NextPart...\n");
                    outputCurrentMsgPart();
                }
            }
        } else if(state == State::Alternating){
            bool robotAck = ioManager.dio.readVal(DIO::Function::IAck);
            if(awaitAck == robotAck){
                if(currentBit >= bitsToSend){
                    state = State::Idle;
                    debugf("Finished transmision.\n");
                    awaitForFinish();
                }else{
                    debugf("NextPart...\n");
                    outputCurrentMsgPart(awaitAck);
                }
            }
        }
    }

};
