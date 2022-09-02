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


    bool flashControlSignals = false;
    unsigned long zerointTimer = 0;
    bool setupControlSignal(DIO::Function signal, DIO::Function conditionSignal){
        bool res = ioManager.dio.readVal(conditionSignal);
        ioManager.dio.write(signal, flashControlSignals ? false : !res);
        return res;
    }
    bool setupControlSingnals(){
        bool res = true;
        if(!setupControlSignal(DIO::Function::OMotorOn, DIO::Function::IMotorOn)){
            Serial.println("Waiting for MotorON...");
            res = false;
        }
        if(!setupControlSignal(DIO::Function::OReset, DIO::Function::IReady)){
            Serial.println("Waiting for Reset...");
            res = false;
        }
        if(!setupControlSignal(DIO::Function::OZeroing, DIO::Function::IZeroed)){
            if(zerointTimer < millis()){
                Serial.println("Waiting for Zeroing...");
                zerointTimer = millis() + 1000 * 10;
            }
            res = false;
        }
        if(res && !setupControlSignal(DIO::Function::OCycleStart, DIO::Function::ICycle)){
                Serial.println("Waiting for Cycle...");
        }
        flashControlSignals = false;
        if(!res){
            delay(50);
        }
        return res;
    }

    bool performSetupSignals = false;
    void loop(){
        if(!sender.isBusy() && !cmdQueue.isempty()){
            if(performSetupSignals){
                bool ready = setupControlSingnals();
                if(!ready){
                    //Serial.print(".");
                    return;
                }
            }
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
