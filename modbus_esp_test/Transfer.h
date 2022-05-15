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
    bool awaitAck   = false;

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
        ioManager.setSend(false);
        ioManager.setCmd(msgToSend.parts[0]);
        ioManager.setSend(true);
        awaitAck = false;
        debugf("Await !IDLE.\n");
    }

    bool getMsgBit(int num){
        int part = num / WordSize;
        int bit  = num % WordSize;
        return msgToSend.parts[part] & (1 << bit);
    }
    void outputCurrentMsgPart(bool invert = false){
        ioManager.setSend(invert);
        uint8_t cmdPart = 0;
        for(int i=0; i<ioManager.CmdPinsCount && currentBit < bitsToSend; i++){
            auto bitValue = getMsgBit(currentBit);
            debugf("Cmd%d = Bit%.2d (%d)\n", i, currentBit, bitValue);
            cmdPart |= bitValue << i;
            ++currentBit;
        }
        ioManager.setCmd(cmdPart);
        ioManager.setSend(!invert);
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
      msgToSend    = msg;
      currentBit   = 0;
      awaitAck     = true;
      state        = mode;
      if(state == State::Single){
        debugf("Starting to send Single Cmd (0x%.4X). Await IDLE\n", msgToSend.parts[0]);
        return true;
      }
      if(msg.parts[0] < 128){
          bitsToSend = WordSize;
      }else if(msg.parts[0] < 256){
          bitsToSend = WordSize*2;
      }else{
          bitsToSend = WordSize*3;
      }
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
        robotIdle = ioManager.readIdle();
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
        if(state == State::Single){
            if(awaitAck && robotIdle){
                outputMsgSingle();
            }else if(!awaitAck && !robotIdle){
                ioManager.setSend(false);
                state = State::Idle;
                debugf("Finished transmision.\n");
                awaitForFinish();
            }
        } else if(state == State::Advanced){
            bool robotAck = ioManager.readAck();
            if(awaitAck && robotAck){
                ioManager.setSend(false);
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
            bool robotAck = ioManager.readAck();
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
