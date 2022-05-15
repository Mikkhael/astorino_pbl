#pragma once
#include "Queue.h"
#include "Transfer.h"

struct AstorinoCmdManager{  

    enum Mode {Single, Advanced, Alternating} sendingMode = Mode::Alternating;

    Queue<Msg> cmdQueue;
    Sender sender;
    bool isRobotIdle = false;

    unsigned int receivedCmds = 0;
    unsigned int executedCmds = 0;
    

    void abort(){
      sender.abort();
    }

    bool enqueueCommand(Msg& msg, bool isDebug = false){
      if(cmdQueue.isfull())
            return false;
        receivedCmds++;
        if(isDebug){
          Serial.print("[D]");
        }
        Serial.printf("Enqueuing Cmd: 0x%.4X, 0x%.4X, 0x%.4X\n", msg.parts[0], msg.parts[1], msg.parts[2]);
        msg.isDebug = isDebug;
        cmdQueue.enqueue(msg);
        return true;
    }

    void init(){
        sender.init();
    }

    void loop(){
        if(!sender.isBusy() && !cmdQueue.isempty()){
            cmdQueue.show();
            if(sendingMode == Mode::Advanced){
                sender.beginSend(cmdQueue.top(), Sender::State::Advanced);
            }else if(sendingMode == Mode::Single){
                sender.beginSend(cmdQueue.top(), Sender::State::Single);
            }else{
                sender.beginSend(cmdQueue.top(), Sender::State::Alternating);
            }
            cmdQueue.pop();
        }
        sender.loop();
        isRobotIdle = sender.robotIdle;
    }

};
