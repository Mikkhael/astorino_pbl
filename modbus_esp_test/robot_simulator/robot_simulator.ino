#include <TM1637Display.h>

int PinsCmdCount;
int PinsCmd[8];
int PinSend;
int PinRobotIdle;
int PinRobotAck = -1;

constexpr int PinTmClk = 12;
constexpr int PinTmDio = 13;
TM1637Display display(PinTmClk, PinTmDio);



void CMD_SIGNAL(int pin){
  Serial.print("SIGNAL ");
  Serial.println(pin);
  digitalWrite(abs(pin), pin < 0);
}
int SIG(int pin){
  auto val = digitalRead(pin) ? 0 : 1;
  Serial.print("SIG ");
  Serial.print(pin);
  Serial.print(" = ");
  Serial.println(val);
  return val;
}
void TWAIT(float s){
  Serial.print("TWAIT ");
  Serial.println(s);
  delay(s * 1000);
}

int emptySegment[] {0,0,0,0};
void executeCmd(int cmd){
  Serial.print("===== EXECUTING CMD : ");
  Serial.print(cmd);
  Serial.println(" =====");
  display.setBrightness(7, true);
  display.showNumberDec(cmd);
  delay(5000);
  display.setBrightness(0, false);
  display.clear();
  Serial.print(" ==== DONE CMD : ");
  Serial.print(cmd);
  Serial.println(" =====");
}

void setup(){
    Serial.begin(9600);
    while(!Serial){
        delay(500);
    }
    Serial.println("\nSerial Ready.");

    PinRobotIdle = 2;
    PinSend = 3;
    PinsCmdCount = 2;
    PinsCmd[0] = 4;
    PinsCmd[1] = 5;
    PinRobotAck = 6;

    pinMode(PinRobotIdle, OUTPUT);
    digitalWrite(PinRobotIdle, 1);
    pinMode(PinSend, INPUT);
    for(int i=0; i<PinsCmdCount; i++){
        pinMode(PinsCmd[i], INPUT);
    }
    if(PinRobotAck >= 0){
        pinMode(PinRobotAck, OUTPUT);
        digitalWrite(PinRobotAck, 0);
    }
    display.setBrightness(0,false);
    display.clear();
}

void loop(){
  int CmdWord = 0, ValueA, ValueB, CurrentBitPower = 1;
  int awaitsend, ret, alt = 1;
  alt = 1;
  CMD_SIGNAL(PinRobotAck);
  TWAIT(3);
  CMD_SIGNAL(-PinRobotIdle);
  for(int b = 0; b < 8; b++){
    alt = 0-alt;
    awaitsend = (1+alt)/2;
    ret = awaitsend;
    Serial.print("Waitig for Send = ");
    Serial.println(awaitsend);
    while(ret == awaitsend){
      while(SIG(PinSend) == awaitsend){
        TWAIT(0.2);
      }
      ret = SIG(PinSend);
    }
    Serial.println("Reading Cmd Part");
    CMD_SIGNAL(PinRobotIdle);
    ValueA = 0 + SIG(PinsCmd[0]);
    ValueB = 0 + SIG(PinsCmd[1]);
    CmdWord += ValueA * CurrentBitPower * 1;
    CmdWord += ValueB * CurrentBitPower * 2;
    CurrentBitPower *= 4;
    CMD_SIGNAL(PinRobotAck*alt);
  }
  executeCmd(CmdWord);
}
