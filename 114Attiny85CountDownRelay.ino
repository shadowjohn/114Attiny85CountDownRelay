// Author: 羽山秋人
// Date: 2026-02-01
// Version: 0.01
// 功能: 倒數計時器，啟動後 Relay 會開，時間到後 Relay 關上

// Internal 8hz
// PB2: Relay
// PB1: TM1637 CLK
// PB0: TM1637 DIO
// PB3: 切換時間/OFF
// PB4: 開始倒數

#include <Arduino.h>
#include <TM1637Display.h>

// ------------------ PIN 定義 ------------------
#define CLK PB1
#define DIO PB0
#define RELAY_PIN PB2
#define BTN_CHANGE PB3
#define BTN_START PB4

#define DEBOUNCE_MS 25   // 軟體去彈跳

TM1637Display display(CLK, DIO);

// ------------------ 按鈕結構 ------------------
struct Button {
  uint8_t pin;
  bool stableState;
  bool lastRead;
  uint32_t lastChange;
};

Button btnChange = { BTN_CHANGE, HIGH, HIGH, 0 };
Button btnStart  = { BTN_START, HIGH, HIGH, 0 };

// ------------------ 倒數設定 ------------------
const int timerOptions[] = {5, 10, 20, 30, 60, 180, 300, 600, 1200, 1800, 3600, 7200}; // 秒
const int optionCount = sizeof(timerOptions)/sizeof(timerOptions[0]);
int currentIndex = 0;
int countdown = timerOptions[0]; // 初始倒數秒數
bool running = false;
uint32_t lastTick = 0;
bool paused = true;
int isStart = true; // 剛開機

// ------------------ 按鈕去彈跳 ------------------
bool buttonPressed(Button &b) {
  bool now = digitalRead(b.pin);
  uint32_t t = millis();

  if (now != b.lastRead) {
    b.lastRead = now;
    b.lastChange = t;
  }

  if ((t - b.lastChange) > DEBOUNCE_MS) {
    if (b.stableState != now) {
      b.stableState = now;
      if (now == LOW) return true;   // 只在剛按下時觸發
    }
  }
  return false;
}

// ------------------ 顯示倒數秒數 ------------------
void showCountdown(int sec) {
  if(sec < 0) {
    display.clear();
    uint8_t data[] = {0x0, 0x0, 0x0, 0x0}; 
    display.setSegments(data);   // blank the display    
  } else {
    display.showNumberDec(sec, false); // 不顯示前導 0
  }
}

// ------------------ setup ------------------
void setup() {
  isStart = true;  
  pinMode(BTN_CHANGE, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // 預設關閉

  display.setBrightness(2);      // 中低亮度
  showCountdown(countdown);

  // 初始化按鈕狀態
  // 避免開機就觸發
  btnChange.stableState = digitalRead(BTN_CHANGE);
  btnChange.lastRead = btnChange.stableState;
  btnChange.lastChange = millis();

  btnStart.stableState = digitalRead(BTN_START);
  btnStart.lastRead = btnStart.stableState;
  btnStart.lastChange = millis();
}

// ------------------ loop ------------------
void loop() {

  // ----- CHANGE 按鈕 -----
  if(!isStart && buttonPressed(btnChange)) {
    if(!running) {
      // 倒數未開始，循環切換
      currentIndex++;
      if(currentIndex >= optionCount) {
        // 最後一組顯示 OFF
        countdown = 5;
        currentIndex = -1;        
        display.clear();
        uint8_t data[] = {0x0, 0x0, 0x0, 0x0}; 
        display.setSegments(data);   // blank the display
      } else {
        countdown = timerOptions[currentIndex];
        showCountdown(countdown);
      }
    } else {
      // 倒數中，直接切換當前倒數秒數到下一階
      currentIndex++;
      if(currentIndex >= optionCount) {
        currentIndex = 0;
      }
      countdown = timerOptions[currentIndex];
      showCountdown(countdown);
    }
  }

  // ----- START 按鈕 -----
  if(!isStart && buttonPressed(btnStart)) {
    if(!running) {
      // 開始倒數
      running = true;
      paused = false;
      lastTick = millis();
      digitalWrite(RELAY_PIN, HIGH);
      showCountdown(countdown);
    } else {
      // 倒數中 → 暫停 / 恢復
      paused = !paused;
      if(paused) {
        digitalWrite(RELAY_PIN, LOW);
      }
      else {
        digitalWrite(RELAY_PIN, HIGH);
      }
    }
  }

  // ----- 倒數邏輯 -----
  if(!isStart && running && !paused) {
    uint32_t now = millis();
    if(now - lastTick >= 1000) {
      lastTick = now;
      if(countdown >= 0) {
        countdown--;
        showCountdown(countdown);
      } else {
        running = false;
        paused = false;
        digitalWrite(RELAY_PIN, LOW);
        display.clear();
        uint8_t data[] = {0x0, 0x0, 0x0, 0x0}; 
        display.setSegments(data);  
        countdown = 0;       
        currentIndex = 0;
      }
    }
  }
  // 剛開始旗標
  isStart = false;
}
