#include <U8x8lib.h>
#include <Encoder.h>
#include <EEPROM.h>
#define ENCODER_USE_INTERRUPTS

// oled
U8X8_SH1106_128X64_NONAME_HW_I2C oled;

// Piezo
const byte pPin = A1;  //piezo pin

// encoder
Encoder encoder(5, 6);
const byte bPin = 3;   //EC12E button
long encoderVal0 = 0, encoderVal1 = 0;

// values
unsigned long waveTEnd = 0;
uint8_t vF0 = 1, vF1 = 1, vT = 1;
int select = 0;
boolean isRunning = false, needDraw = true, waveDetails = false;

void setup() {
  Serial.begin(115200);
  oled.begin();
  oled.setFont(u8x8_font_chroma48medium8_r);
  loadE();

  for(int i = 7; i < 11; i++)
    pinMode(i, OUTPUT);
  pinMode(bPin, INPUT);
}

void loop() {
  if (isRunning) {
    wave();
  } else {
    draw();
    bAction();
    encoderVal1 = encoder.read();
    if (encoderVal0 != encoderVal1 && encoderVal1%4==0) {
      ec12eM();
      encoderVal0 = encoderVal1 = 0;
      encoder.write(0);
    }
  }
}

void wave() {
  bool loAE = vF0 > 0;
  bool loBE = vF1 > 0;
  long loAHz = loAE ? (100000 / vF0) : -1;
  long loBHz = loBE ? (100000 / vF1) : -1;
  unsigned long millisCur = millis(), loND = millisCur;
  long waveAt = 0, waveBt = 0, microsCur;
  uint8_t waveAs = 0, waveBs = 0;
  waveTEnd = millisCur + vT*60000;
  tone(pPin, 1000);
  delay(1000);
  noTone(pPin);
  oled.clear();
  for (uint8_t i = 7; i < 11; i++) digitalWrite(i, 0);
  while(isRunning) {
    microsCur = micros();
    millisCur = millis();
    if(loAE && waveAt < microsCur) {
      switch(waveAs) {
        case 0: digitalWrite(7, 1); waveAt = microsCur + loAHz*3; waveAs++; break;
        case 1: digitalWrite(7, 0); waveAt = microsCur + loAHz*2; waveAs++; break;
        case 2: digitalWrite(8, 1); waveAt = microsCur + loAHz*3; waveAs++; break;
        case 3: digitalWrite(8, 0); waveAt = microsCur + loAHz*2; waveAs=0; break;
      }
    }
    if(loBE && waveBt < microsCur) {
      switch(waveBs) {
        case 0: digitalWrite(9, 1); waveBt = microsCur + loBHz*3; waveBs++; break;
        case 1: digitalWrite(9, 0); waveBt = microsCur + loBHz*2; waveBs++; break;
        case 2: digitalWrite(10, 1); waveBt = microsCur + loBHz*3; waveBs++; break;
        case 3: digitalWrite(10, 0); waveBt = microsCur + loBHz*2; waveBs=0; break;
      }
    }
    if (loND < millisCur) {
      needDraw = true;
      draw();
      loND = millisCur + 1000;
    }
    if (waveTEnd < millisCur) {
      isRunning = false;
    }
    bAction();
    encoderVal1 = encoder.read();
    if (encoderVal0 != encoderVal1 && encoderVal1%4==0) {
      waveDetails = !waveDetails;
      encoderVal0 = encoderVal1 = 0;
      encoder.write(0);
      needDraw = true;
      oled.clear();
      draw();
    }
  }
  for(int i = 7; i < 11; i++)
    digitalWrite(i, 0);
  for (uint8_t i = 0; i < 3; i++) {
    tone(pPin, 1000);
    delay(1000);
    noTone(pPin);
    delay(500);
  }
  oled.clear();
  needDraw = true;
  draw();
}

void ec12eM() {
  bool rot = encoderVal0 > encoderVal1; // 1 -> c; 0 -> cw
  if(!isRunning) {
    if(rot) {
      switch(select) {
        case 0: vF0++; if(vF0>99) vF0=0; break;
        case 1: vF1++; if(vF1>99) vF1=0; break;
        case 2: vT++; if(vT>30) vT=1; break;
      }
    } else {
      switch(select) {
        case 0: vF0--; if(vF0<0) vF0=99; break;
        case 1: vF1--; if(vF1<0) vF1=99; break;
        case 2: vT--; if(vT<1) vT=30; break;
      }
    }
    saveE();
    needDraw = true;
  }
}

//Button action
long bActionLast = 0;
uint8_t btnLastState = 1;
void bAction() {
  uint8_t btnCurState = digitalRead(bPin);
  if(btnCurState != btnLastState && btnCurState == 1 && bActionLast < millis()) {
    if(!isRunning) {
      select++;
      if(select == 4) {
        isRunning = true;
        select = 0;
      }
    } else {
      isRunning = false;
    }
    bActionLast = millis() + 300;
    needDraw = 1;
  }
  btnLastState = btnCurState;
}

uint8_t tiles[11][8] = {
  {90, 90, 90, 90, 90, 90, 90, 90},
  {90, 90, 90, 90, 90, 90, 90, 66},
  {90, 90, 90, 90, 90, 90, 66, 66},
  {90, 90, 90, 90, 90, 66, 66, 66},
  {90, 90, 90, 90, 66, 66, 66, 66},
  {90, 90, 90, 66, 66, 66, 66, 66},
  {90, 90, 66, 66, 66, 66, 66, 66},
  {90, 66, 66, 66, 66, 66, 66, 66},
  {66, 126, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 126, 66},
  {66, 66, 66, 66, 66, 66, 66, 66}
};

void draw() {
  if (!needDraw) return;
  needDraw = false;
  if (!isRunning || (isRunning && waveDetails)) {
    oled.setCursor(0, 0);
    oled.print("ch 0: ");
    if (vF0 > 0) {
      oled.print(vF0);
      oled.print(" Hz ");
    } else oled.print("off  ");
    oled.setCursor(0, 1);
    oled.print("ch 1: ");
    if (vF1 > 0) {
      oled.print(vF1);
      oled.print(" Hz ");
    } else oled.print("off  ");
    oled.setCursor(3, 2);
    oled.print("t: ");
    oled.print(vT);
    oled.print(" min ");
    if (!isRunning) {
      oled.setCursor(5, 3);
      oled.print("START");
      for (uint8_t i = 0; i < 4; i++) {
        oled.setCursor(13, i);
        if (i == select) oled.print("<");
        else oled.print(" ");
      }
    }
  } else {
    oled.setCursor(2, 0);
    oled.print("Press to stop");
    oled.setCursor(0, 1);
    oled.print("t-: ");
    oled.print((waveTEnd - millis()) / 60000);
    oled.print(" min ");
    oled.drawTile(0, 3, 1, tiles[9]);
    oled.drawTile(15, 3, 1, tiles[8]);
    uint8_t i = 0;
    for (i = 0; i < ((waveTEnd - millis()) * 14) / (vT*60000); i++) oled.drawTile(1 + i, 3, 1, tiles[0]);
    oled.drawTile(1 + i, 3, 1, tiles[7 - (((waveTEnd - millis()) * 112) / (vT*60000) % 8)]);
    for (i = i + 2; i < 15; i++) oled.drawTile(i, 3, 1, tiles[10]);
  }
}

//save & load
void saveE() {
  EEPROM.update(0, vF0);
  EEPROM.update(1, vF1);
  EEPROM.update(2, vT);
}
void loadE() {
  EEPROM.get(0, vF0);
  EEPROM.get(1, vF1);
  EEPROM.get(2, vT);
  
  if(vF0 < 0) vF0 = 0;
  else if(vF0 > 99) vF0 = 99;
  if(vF1 < 0) vF1 = 0;
  else if(vF1 > 99) vF1 = 99;
  if(vT < 1) vT = 1;
  else if(vT > 30) vT = 30;
}
