#include "U8g2lib.h"
#include <Encoder.h>
#include <EEPROM.h>
#define ENCODER_USE_INTERRUPTS

//EC12E rotator
Encoder ec12e(5, 6);   //EC12E
long ec12eP0 = 0;      //EC12E last value
long ec12eP1 = 0;      //EC12E current value

//Oled display
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g(U8G2_R0);

//Button
const byte bPin = 3;   //EC12E button

//output
const byte a0Out = 7;  //a0
const byte a1Out = 8;  //a1
const byte b0Out = 9;  //b0
const byte b1Out = 10; //b1
boolean isRunning = false;

//Piezo
const byte pPin = A1;  //piezo pin

//values
long vHz0 = 1;
long vHz1 = 1;
long vLen = 1;
int select = 0;
bool egg = 0;
bool waveDetails = 0;
//0 -> f
//1 -> t
//2 -> out
//3 -> start

void setup() {
  //Serial setup
  Serial.begin(115200);
  //pin setup
  pinMode(bPin, INPUT);
  for(int i = 7; i < 11; i++)
    pinMode(i, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(bPin), bAction, RISING);
  //display setup
  u8g.begin();
  //load and draw
  loadE();
  delay(100);
  draw();
  Serial.println("2cPEMF|setup| setup done");
}

long loopDN = 0;
bool loopSD = 0;
void loop() {
  ec12eP1 = ec12e.read();
  if (ec12eP0 != ec12eP1 && ec12eP1%4==0) {
    ec12eM();
    ec12eP0 = ec12eP1 = 0;
    ec12e.write(0);
  }
  if(isRunning) wave();
  if(loopDN < millis() && loopSD) {
    draw();
    loopDN = millis()+500;
    loopSD = 0;
  }
}

void ec12eM() {
  bool rot = ec12eP0 > ec12eP1; // 1 -> c; 0 -> cw
  if(!isRunning) {
    if(rot) {
      switch(select) {
        case 0: vHz0++; if(vHz0>99) vHz0=0; break;
        case 1: vHz1++; if(vHz1>99) vHz1=0; break;
        case 2: vLen++; if(vLen>30) vLen=1; break;
        case 3: egg = !egg; break;
      }
    } else {
      switch(select) {
        case 0: vHz0--; if(vHz0<0) vHz0=99; break;
        case 1: vHz1--; if(vHz1<0) vHz1=99; break;
        case 2: vLen--; if(vLen<1) vLen=30; break;
        case 3: egg = !egg; break;
      }
    }
    loopSD = 1;
    saveE();
  }
}

//Button action
long bActionLast = 0;
void bAction() {
  if(bActionLast < millis()) {
    Serial.println("bAction");  
    if(!isRunning) {
      select++;
      loopSD = 1;
      if(select==4 && !egg){isRunning = true; select=0;}
      else if(select==4 && egg) {select=0; egg = 0;}
    }
    else {isRunning = false;}
    bActionLast = millis()+500;
  }
}

//Wave
long waveTEnd = 0;
long waveAt = 0;
byte waveAs = 0;
long waveBt = 0;
byte waveBs = 0;
void wave() {
  long cycle = 0;
  bool loAE = vHz0 > 0;
  bool loBE = vHz1 > 0;
  long loAHz = loAE ? (1000000/vHz0/10) : -1;
  long loBHz = loBE ? (1000000/vHz1/10) : -1;
  long loND = millis();
  waveTEnd = millis()+vLen*60*1000;
  tone(pPin, 1000);
  delay(1000);
  noTone(pPin);
  while(isRunning) {
    if(waveAt < micros() && loAE) {
      switch(waveAs) {
        case 0: digitalWrite(a0Out, 1); digitalWrite(a1Out, 0); waveAt = micros()+loAHz*3; waveAs++; break;
        case 1: digitalWrite(a0Out, 0); digitalWrite(a1Out, 0); waveAt = micros()+loAHz*2; waveAs++; break;
        case 2: digitalWrite(a0Out, 0); digitalWrite(a1Out, 1); waveAt = micros()+loAHz*3; waveAs++; break;
        case 3: digitalWrite(a0Out, 0); digitalWrite(a1Out, 0); waveAt = micros()+loAHz*2; waveAs=0; break;
      }
    }
    if(waveBt < micros() && loBE) {
      switch(waveBs) {
        case 0: digitalWrite(b0Out, 1); digitalWrite(b1Out, 0); waveBt = micros()+loBHz*3; waveBs++; break;
        case 1: digitalWrite(b0Out, 0); digitalWrite(b1Out, 0); waveBt = micros()+loBHz*2; waveBs++; break;
        case 2: digitalWrite(b0Out, 0); digitalWrite(b1Out, 1); waveBt = micros()+loBHz*3; waveBs++; break;
        case 3: digitalWrite(b0Out, 0); digitalWrite(b1Out, 0); waveBt = micros()+loBHz*2; waveBs=0; break;
      }
    }
    if(loND < millis()) {
      draw();
      loND = millis()+1000*10;
    }
    if(waveTEnd < millis()) {
      isRunning = false;
    }
    ec12eP1 = ec12e.read();
    if (ec12eP0 != ec12eP1 && ec12eP1%4==0) {
      waveDetails = !waveDetails;
      ec12eP0 = ec12eP1 = 0;
      ec12e.write(0);
      draw();
    }
    cycle++;
  }
  for(int i = 7; i < 11; i++)
    digitalWrite(i, 0);
  tone(pPin, 1000);
  delay(1000);
  noTone(pPin);
  delay(500);
  tone(pPin, 1000);
  delay(1000);
  noTone(pPin);
  delay(500);
  tone(pPin, 1000);
  delay(1000);
  noTone(pPin);
  delay(500);
  draw();
}

//Draw
static unsigned char drawImage0[] = {0x08,0x0f,0xe1,0x44,0x88,0x04,0x91,0x12,0x25,0x90,0x02,0x91,0x12,0x14,0xa0,0x7f,0x8f,0x12,0x0c,0xff,0x02,0x51,0x14,0x14,0xa0,0x04,0xd1,0x17,0x25,0x90,0x08,0x4f,0xe4,0x44,0x88};
static unsigned char drawImage1[] = {0x02,0xf4,0x03,0xfc,0x38,0xf3,0x99,0xf9,0x98,0xf1,0xc9,0xf9,0x98,0xf1,0xbd,0xf9,0xd8,0xf0,0x95,0xf9,0x03,0xfc,0x02,0xf4};
long drawLast = 0;
void draw() {
  if(drawLast < millis()) {
    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_9x18);
      u8g.setFontRefHeightExtendedText();
      u8g.setFontPosTop();
      if(!isRunning && !egg) {
        u8g.setCursor(10, 1); u8g.print("ch 0: "+(vHz0>0?val22(vHz0)+" Hz":"off"));
        u8g.setCursor(10,17); u8g.print("ch 1: "+(vHz1>0?val22(vHz1)+" Hz":"off"));
        u8g.setCursor(10,33); u8g.print("   t: "+val22(vLen)+" min");
        u8g.setCursor(10,49); u8g.print("START");
        if(select < 4){u8g.drawFrame(4, select*16, 122, 16);}
      } else if(!isRunning && select==3 && egg) {
        u8g.setCursor(10, 0); u8g.print("github.com/");
        u8g.setCursor(10,16); u8g.print("frfole/2cPEMF");
        u8g.drawLine(0,32,127,32);
        u8g.setCursor(10,37); u8g.print("23.7.2k20");
        u8g.drawXBM(44,54,39,7,drawImage0);
        u8g.drawXBM(116,52,12,12,drawImage1);
      } else if(isRunning && !waveDetails) {
        u8g.setCursor(10,1); u8g.print(" Press");
        u8g.setCursor(10,17); u8g.print("to stop");
        u8g.setCursor(10,40); u8g.print("t-: "+String((waveTEnd-millis())/1000/60)+" min");
        u8g.drawFrame(12,56,104,5);
        u8g.drawLine(14,58,100-((waveTEnd-millis())*100/(vLen*60*1000))+14,58);
      } else if(isRunning && waveDetails) {
        u8g.setCursor(10, 1); u8g.print("ch 0: "+(vHz0>0?val22(vHz0)+" Hz":"off"));
        u8g.setCursor(10,17); u8g.print("ch 1: "+(vHz1>0?val22(vHz1)+" Hz":"off"));
        u8g.setCursor(10,33); u8g.print("   t: "+val22(vLen)+" min");
      }
    } while(u8g.nextPage());
    Serial.println("2cPEMF|draw| drawing");
    drawLast = millis()+250;
  }
}

//save & load
struct LastSet {
  long freq0;
  long freq1;
  long t;
};
void saveE() {
  LastSet a = {vHz0, vHz1, vLen};
  EEPROM.put(0, a);
}
void loadE() {
  LastSet a;
  EEPROM.get(0, a);
  vHz0 = a.freq0;
  vHz1 = a.freq1;
  vLen = a.t;
  if(vHz0<0) vHz0 = 0;
  if(vHz0>99) vHz0 = 99;
  if(vHz1<0) vHz1 = 0;
  if(vHz1>99) vHz1 = 99;
  if(vLen<1) vLen = 1;
  if(vLen>30) vLen = 30;
}

//utils
String val22(long i) {
  if( i >= 0 && i < 10) return "0"+String(i);
  return String(i);
}
