#include "NimBLEDevice.h"
#include <BleKeyboard.h>
// #include <BleMouse.h>
// #define USE_NIMBLE

#include "SPIFFS.h"
#include <ArduinoJson.h>

#include <GyverOLED.h>
GyverOLED<SSH1106_128x64> oled;

#include <Wire.h>

#include <EncButton.h>

VirtButton up;
VirtButton down;
VirtButton left;
VirtButton right;
VirtButton ok;
VirtButton back;
VirtButton top_left;
VirtButton top_right;

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

#define top_right_pin 16
#define top_left_pin 17
#define led_pin 26
#define RCPin 4
unsigned long store1 = 0;
uint8_t bitR1 = 0;
uint8_t protoc = 0;

// float mainFrequency = 433.92;
// SETTINGS
uint8_t FrequencyPointer = 9;
int start12bitBruteForce = 1;

uint8_t pinsCount = 4;         // numPins + 1 -- this value is pins + main Available pins
const uint8_t pinsCounts = 3;  // numPins + 1 -- this value is pins + main Available pins
int OutPinModeG[pinsCounts];

// SINGAL DETECTION
#define SIGNAL_DETECTION_FREQUENCIES_LENGTH 19
float signalDetectionFrequencies[SIGNAL_DETECTION_FREQUENCIES_LENGTH] = { 300.00, 303.87, 304.25, 310.00, 315.00, 318.00, 390.00, 418.00, 433.07, 433.92, 434.42, 434.77, 438.90, 868.3, 868.35, 868.865, 868.95, 915.00, 925.00 };
int detectedRssi = -100;
float detectedFrequency = 0.0;
int minRssi = -68;
bool RECORDING_SIGNAL = false;
bool RxTxMode = true;

const int boardSize = 4;

//  RAW DATA
DynamicJsonDocument doc(4000);
JsonObject objectDoc = doc.createNestedObject("sensorData");
JsonObject objectRC = doc.createNestedObject("data");

class OUTPIN {
public:
  OUTPIN(byte pin) {
    _pin = pin;
    pinMode(_pin, OUTPUT);
  }

  void change() {
    _flag = !_flag;
    digitalWrite(_pin, _flag);
  }
private:
  byte _pin;
  bool _flag;
};

OUTPIN pin1(32);
OUTPIN pin2(33);
OUTPIN pin3(25);

int ITEMS = 8;
int RC_ITEMS = 32;


char* main_lay[8] = {
  "RC Custom",
  "RawData Read",
  "Frequency Analyzer",
  "RC 12bit bruteForce",
  "BLUETOOTH",
  "GPIO",
  "Settings",
  "Games",
};
char* blue_lay[8] = {
  "Keyboard",
  "Mouse",
  "",
  "",
  "",
  "",
  "",
  "",
};


void setupRx() {
  ELECHOUSE_cc1101.Init();
  // ELECHOUSE_cc1101.setRxBW(812.50);
  ELECHOUSE_cc1101.setRxBW(256);
  ELECHOUSE_cc1101.setMHZ(signalDetectionFrequencies[FrequencyPointer]);
  // if (!RxTxMode) {
  pinMode(RCPin, INPUT);
  mySwitch.disableTransmit();
  ELECHOUSE_cc1101.SetRx();
  mySwitch.enableReceive(RCPin);
  RxTxMode = !RxTxMode;
  // }
}
void setupTx() {
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(signalDetectionFrequencies[FrequencyPointer]);
  // if (RxTxMode) {
  pinMode(RCPin, OUTPUT);
  mySwitch.disableReceive();
  ELECHOUSE_cc1101.SetTx();
  mySwitch.enableTransmit(RCPin);
  RxTxMode = !RxTxMode;
  // }
}

void setup() {
  // if (SPIFFS.exists("/raw.json")) {
  //   SPIFFS.remove("/raw.json");
  //   Serial.println("Файл удален");
  // } else {
  //   Serial.println("Файл не существует");
  // }
  Wire.setClock(800000L);

  oled.init();
  oled.clear();
  oled.setContrast(255);
  drawLogo();
  oled.update();

  // create_screen();

  for (int i = 0; i < pinsCount - 1; i++) {  // file GPIO.ino
    OutPinModeG[i] = 0;
  }

  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(signalDetectionFrequencies[FrequencyPointer]);
  // ELECHOUSE_cc1101.setRxBW(58);
  mySwitch.enableReceive(RCPin);
  // mySwitch.setPulseLength();
  ELECHOUSE_cc1101.SetRx();
  ELECHOUSE_cc1101.goSleep();

  pinMode(top_left_pin, INPUT_PULLUP);
  pinMode(top_right_pin, INPUT_PULLUP);
  pinMode(led_pin, OUTPUT);

  digitalWrite(led_pin, LOW);

  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("Ошибка инициализации SPIFFS");
  }

  readJsonFromFile("/raw.json", doc);
  Serial.println();
  // readJsonFromFile("/raw.json", docRC);
  // Serial.println();

  // first start - ".as" to ".to"
  objectDoc = doc["sensorData"].as<JsonObject>();  // frequency, name, RawData[]
  objectRC = doc["data"].as<JsonObject>();         // frequency, name, data, bytes

  // serializeJson(docRC, Serial);
  // Serial.println();
}

void loop() {
  mainn();
}

void mainn() {
  bool displayUpdate = 1;
  tk();
  while (1) {
    static uint8_t pointer = 0;
    tk();
    if (displayUpdate != 0) {
      displayUpdate = 0;
      oled.clear();
      oled.home();
      for (uint8_t i = 0; i < 8; i++) {
        oled.setCursor(14, i);
        oled.print(main_lay[i]);
      }
      oled.setCursor(100, 0);
      int volt = analogRead(35);
      volt += analogRead(35);
      volt += analogRead(35);
      volt += analogRead(35);
      volt /= 4;
      oled.print(volt * 3.26 * 2.25 / 4096);  // volt * 3.26 * ((r1 + r2) / r2) / 4096
      printPointer(pointer);
      oled.update();
    }

    // ||||| Кнопки |||||
    if (up.click() or up.step()) {
      pointer = constrain(pointer - 1, 0, ITEMS - 1);
      displayUpdate = 1;
    }
    if (down.click() or down.step()) {
      pointer = constrain(pointer + 1, 0, ITEMS - 1);
      displayUpdate = 1;
    }
    if (ok.click() or ok.hold()) {
      switch (pointer) {
        case 0: rcLay(); break;
        case 1: rawLay(); break;
        case 2: detectSignal(); break;
        case 3:
          rc12bitBruteForce();
          ELECHOUSE_cc1101.goSleep();
          break;
        case 4: bluetoothLay(); break;
        case 5: gpioLay(); break;
        case 6: settings(); break;
        case 7: games(); break;
      }
      displayUpdate = 1;
    }
  }
}


void rawLay() {
  bool updDisplay = true;
  while (1) {
    static uint8_t pointer = 0;
    tk();
    if (updDisplay) {
      updDisplay = false;
      oled.clear();
      oled.home();
      for (int i = 0; i < 8; i++) {
        int ui = i + ((pointer / 8) * 8);
        oled.setCursor(14, i);
        String j = String(ui);
        if (objectDoc[j].isNull()) {
          oled.print("__");
        } else {
          oled.print(objectDoc[j]["name"].as<const char*>());
        }
      }
      printPointer(pointer);
      oled.update();
    }

    if (ok.click() or ok.hold()) {
      String j = String(pointer);
      if (!objectDoc[j]["RawData"].isNull()) {
        digitalWrite(led_pin, HIGH);

        oled.clear();
        oled.home();
        oled.print("Translation..");
        oled.update();

        sendSamples(objectDoc[j]["RawData"].as<JsonArray>());
        digitalWrite(led_pin, LOW);
        ELECHOUSE_cc1101.goSleep();
      }
      updDisplay = 1;
    }

    if (up.click() or up.step()) {
      pointer = constrain(pointer - 1, 0, RC_ITEMS - 1);
      updDisplay = 1;
    }
    if (down.click() or down.step()) {
      pointer = constrain(pointer + 1, 0, RC_ITEMS - 1);
      updDisplay = 1;
    }
    if (right.click() or right.hold()) {
      RecordSignal(pointer, objectDoc);
      saveJsonToFile("/raw.json", doc);
      ELECHOUSE_cc1101.goSleep();
      tk();
      updDisplay = 1;
    }
    if (back.click() or back.hold()) break;
    if (left.hold()) {
      rawMenu(pointer);
      updDisplay = 1;
    }
  }
}


void rawMenu(uint8_t pointer1) {
  String j = String(pointer1);
  bool updDisplay = true;
  tk();
  while (1) {
    static uint8_t pointer = 2;
    tk();
    if (updDisplay) {
      updDisplay = 0;
      oled.clear();
      oled.home();
      oled.setCursor(14, 0);
      oled.print("OBJ: ");
      if (objectDoc[j].isNull()) {
        oled.print("[EMPTY]");
      } else {
        oled.print(objectDoc[j]["name"].as<const char*>());
      }
      oled.setCursor(14, 1);
      oled.print("size: ");
      oled.print(objectDoc[j]["RawData"].size());
      oled.setCursor(14, 2);
      oled.print(" Rename");
      oled.setCursor(14, 3);
      oled.print(" Delete");
      // oled.print(objectDoc[j]["RawData"].as<JsonArray>().size());

      printPointer(pointer);
      oled.update();
    }
    if (up.click() or up.step()) {
      pointer = constrain(pointer - 1, 2, 3);
      updDisplay = 1;
    }
    if (down.click() or down.step()) {
      pointer = constrain(pointer + 1, 2, 3);
      updDisplay = 1;
    }
    if (back.click() or back.hold()) break;
    if (ok.click() or ok.hold()) {
      switch (pointer) {
        case 2:
          objectDoc[j]["name"] = setName("Raw");
          break;
        case 3:
          objectDoc.remove(j);
          return;
          break;
      }
      saveJsonToFile("/raw.json", doc);
      updDisplay = 1;
    }
  }
}


void rcLay() {
  bool displayUpdate = 1;
  while (1) {
    static uint8_t pointer = 0;
    tk();
    if (displayUpdate != 0) {
      displayUpdate = 0;
      oled.clear();
      oled.home();
      for (uint8_t i = 0; i < 8; i++) {
        int ui = i + ((pointer / 8) * 8);
        oled.setCursor(14, i);
        // oled.print(trans_lay[ui]);
        String j = String(ui);
        if (objectRC[j].isNull()) {
          oled.print("__");
        } else {
          oled.print(objectRC[j]["name"].as<const char*>());
        }
      }
      printPointer(pointer);
      oled.update();
    }


    if (up.click() or up.step()) {
      pointer = constrain(pointer - 1, 0, RC_ITEMS - 1);
      displayUpdate = 1;
    }
    if (down.click() or down.step()) {
      pointer = constrain(pointer + 1, 0, RC_ITEMS - 1);
      displayUpdate = 1;
    }
    if (back.click() or back.hold()) break;
    if (ok.click() or ok.hold()) {
      func_trans(pointer);
      ELECHOUSE_cc1101.goSleep();
      displayUpdate = 1;
    }
    if (left.hold()) {
      menuRc(pointer);
      displayUpdate = 1;
    }
    if (right.click() or right.hold()) {
      func_reciv(pointer);
      ELECHOUSE_cc1101.goSleep();
      displayUpdate = 1;
    }
    delay(1);
  }
}

void detectSignal() {
  setupRx();
  ELECHOUSE_cc1101.setRxBW(256);
  byte signalDetected = 0;
  bool changed = false;
  detectedRssi = -100;
  detectedFrequency = 0.0;
  oled.clear();
  oled.home();
  oled.print("Frequency Analyzer..");
  oled.setCursor(0, 1);
  oled.print("minRssi: ");
  oled.print(minRssi);
  oled.update();
  while (signalDetected <= 5) {
    tk();
    if (back.click()) return;
    if (ok.click()) {
      detectSignal();
      return;
    }
    if (up.click()) {
      minRssi++;
      changed = true;
    }
    if (down.click()) {
      minRssi--;
      changed = true;
    }
    if (changed) {
      changed = false;
      oled.setCursor(0, 1);
      oled.print("minRssi: ");
      oled.print(minRssi);
      oled.update();
    }
    detectedRssi = -100;
    // ITERATE FREQUENCIES
    for (float fMhz : signalDetectionFrequencies) {
      ELECHOUSE_cc1101.setMHZ(fMhz);
      int rssi = ELECHOUSE_cc1101.getRssi();
      if (rssi >= detectedRssi) {
        detectedRssi = rssi;
        detectedFrequency = fMhz;
      }
    }
    for (float fMhz : signalDetectionFrequencies) {
      ELECHOUSE_cc1101.setMHZ(fMhz);
      int rssi = ELECHOUSE_cc1101.getRssi();
      if (rssi >= detectedRssi) {
        detectedRssi = rssi;
        detectedFrequency = fMhz;
      }
    }

    if (detectedRssi >= minRssi) {

      signalDetected++;

      // DEBUG
      Serial.print("MHZ: ");
      Serial.print(detectedFrequency);
      Serial.print(" RSSI: ");
      Serial.println(detectedRssi);

      oled.setCursor(0, 1 + signalDetected);
      oled.print("MHZ: ");
      oled.print(detectedFrequency);
      oled.print(" RSSI: ");
      oled.println(detectedRssi);
      oled.update();
      delay(1);
    }
  }
  while (1) {
    tk();
    if (back.click()) return;
    if (ok.click()) {
      detectSignal();
      return;
    }
    if (up.click()) {
      minRssi++;
      changed = true;
    }
    if (down.click()) {
      minRssi--;
      changed = true;
    }
    if (changed) {
      changed = false;
      oled.setCursor(0, 1);
      oled.print("minRssi: ");
      oled.print(minRssi);
      oled.update();
    }
  }
}


void bluetoothLay() {
  static uint8_t pointer = 0;
  bool displayUpdate = 1;
  while (1) {
    tk();
    if (displayUpdate != 0) {
      displayUpdate = 0;
      oled.clear();
      oled.home();
      for (uint8_t i = 0; i < 8; i++) {
        oled.setCursor(14, i);
        oled.print(blue_lay[i]);
      }
      printPointer(pointer);
      oled.update();
    }

    if (up.click() or up.step()) {
      pointer = constrain(pointer - 1, 0, ITEMS - 1);
      displayUpdate = 1;
    }
    if (down.click() or down.step()) {
      pointer = constrain(pointer + 1, 0, ITEMS - 1);
      displayUpdate = 1;
    }
    if (back.click() or back.hold()) break;
    if (ok.click() or ok.hold()) {
      switch (pointer) {
        case 0: selectKeyboard(); break;  // selectKeyboard()
        case 1: selectMouse(); break;
        case 2: nothing(); break;
        case 3: nothing(); break;
        case 4: nothing(); break;
        case 5: nothing(); break;
        case 6: nothing(); break;
        case 7: nothing(); break;
      }
      displayUpdate = 1;
    }
  }
}





void menuRc(uint8_t pointer1) {
  String j = String(pointer1);
  static uint8_t pointer = 3;
  bool updDisplay = true;
  while (1) {
    tk();

    if (updDisplay) {
      updDisplay = 0;
      oled.clear();
      oled.home();
      oled.setCursor(14, 0);
      oled.print("OBJ: ");
      if (objectRC[j].isNull()) {
        oled.print("[EMPTY]");
      } else {
        oled.print(objectRC[j]["name"].as<const char*>());
      }
      oled.setCursor(14, 1);
      oled.print("data: ");
      oled.print(objectRC[j]["data"].as<unsigned long>());

      oled.setCursor(14, 2);
      oled.print("bytes: ");
      oled.print(objectRC[j]["bytes"].as<unsigned int>());

      oled.setCursor(14, 3);
      oled.print(" Rename");

      oled.setCursor(14, 4);
      oled.print(" Edit");

      oled.setCursor(14, 5);
      oled.print(" Delete");

      printPointer(pointer);
      oled.update();
    }

    if (up.click() or up.step()) {
      pointer = constrain(pointer - 1, 3, 5);
      updDisplay = true;
    }
    if (down.click() or down.step()) {
      pointer = constrain(pointer + 1, 3, 5);
      updDisplay = true;
    }
    if (back.click() or back.hold()) break;
    if (ok.click() or ok.hold()) {
      switch (pointer) {
        case 3:
          reNameRC(pointer1);
          return;
          break;
        case 4:
          changeRC(pointer1);
          return;
          break;
        case 5:
          deleteRC(pointer1);
          return;
          break;
      }
    }
  }
}

void deleteRC(int pointer1) {
  String j = String(pointer1);
  static uint8_t pointer = 1;
  bool updDisplay = 1;
  while (1) {
    tk();

    if (updDisplay) {
      updDisplay = 0;
      oled.clear();
      oled.home();
      oled.setCursor(28, 0);
      oled.print("Delete RC?");
      oled.setCursor(14, 1);
      oled.print("No");
      oled.setCursor(14, 2);
      oled.print("Yes");

      printPointer(pointer);
      oled.update();
    }

    if (ok.click() or ok.hold()) {
      switch (pointer) {
        case 1:
          return;
          break;
        case 2:
          select_deleteRc(pointer1);
          return;
          break;
      }
    }

    if (up.click() or up.step()) {
      pointer = constrain(pointer - 1, 1, 2);
      updDisplay = true;
    }
    if (down.click() or down.step()) {
      pointer = constrain(pointer + 1, 1, 2);
      updDisplay = true;
    }
  }
}

void reNameRC(int pointer) {
  String j = String(pointer);
  String mStr = setName("Raw");
  if (mStr.startsWith("tmp - ")) {
    return;
  }
  objectRC[j]["name"] = mStr;
  saveJsonToFile("/raw.json", doc);
}

void changeRC(int pointer) {
  String j = String(pointer);
  objectRC[j]["data"] = setNumber("Change code", objectRC[j]["data"]);
  saveJsonToFile("/raw.json", doc);

  // int data = setNumber("Change code", save_data.data_prog[pointer]);
  // save_data.data_prog[pointer] = data;
  // snprintf(trans_lay[pointer], colls, "%lu - %hu", save_data.data_prog[pointer], save_data.data_prog1[pointer]);
  // write_eeprom();
}


void select_deleteRc(uint8_t pointer) {
  String j = String(pointer);
  objectRC.remove(j);
  saveJsonToFile("/raw.json", doc);
  // save_data.data_prog[pointer] = 0;
  // save_data.data_prog1[pointer] = 0;
  // trans_lay[pointer] = "__";
  // write_eeprom();
}


void func_trans(uint8_t codek) {
  String j = String(codek);
  if (objectRC[j]["data"].isNull()) return;
  setupTx();
  digitalWrite(led_pin, HIGH);
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("Transmition..");
  oled.update();
  // mySwitch.send(save_data.data_prog[codek], save_data.data_prog1[codek]);
  mySwitch.send(objectRC[j]["data"], objectRC[j]["bytes"]);
  delay(400);
  digitalWrite(led_pin, LOW);
}


void func_custom_translate(unsigned long address, uint8_t byte) {
  setupTx();
  digitalWrite(led_pin, HIGH);
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("Transmition..");
  oled.update();
  mySwitch.send(address, byte);
  delay(700);
  digitalWrite(led_pin, LOW);
}


void func_reciv(uint8_t cutr) {
  setupRx();
  tk();
  int uo = 1;
  oled.clear();
  oled.home();
  oled.print("Reading..");
  // oled.print(ELECHOUSE_cc1101.getMode());
  oled.update();
  digitalWrite(led_pin, HIGH);
  while (uo == 1) {
    tk();
    if (back.click() or back.hold()) {
      digitalWrite(led_pin, LOW);
      return;
    }
    if (mySwitch.available()) {
      store1 = mySwitch.getReceivedValue();
      bitR1 = mySwitch.getReceivedBitlength();
      protoc = mySwitch.getReceivedProtocol();
      Serial.print("Received ");
      Serial.print(store1);
      Serial.print(" / ");
      Serial.print(bitR1);
      Serial.print("bit ");
      Serial.print("Protocol: ");
      Serial.println(protoc);

      mySwitch.resetAvailable();
      uo = 0;
      digitalWrite(led_pin, LOW);
    }
  }
  String j = String(cutr);
  char def[20];
  snprintf(def, 20, "%lu - %hu", store1, bitR1);
  String mStr = setName("Signal");  // <----
  if (mStr.startsWith("tmp - ")) {
    mStr = def;
  }
  objectRC[j]["name"] = mStr;
  objectRC[j]["frequency"] = FrequencyPointer;
  objectRC[j]["data"] = store1;
  objectRC[j]["bytes"] = bitR1;

  saveJsonToFile("/raw.json", doc);
}


void rc12bitBruteForce() {
  setupTx();
  // mySwitch.setPulseLength(250);
  oled.clear();
  oled.home();
  oled.print("Rc 12bit brute force");
  // oled.setCursor(0, 4);
  // oled.print("        0/4096      ");
  oled.update();

  uint8_t byt = 12;
  int iy = 1;
  for (int i = start12bitBruteForce; i < 4096; i++) {
    tk();
    if (back.click() or back.hold()) {

      // mySwitch.setPulseLength(350);
      return;
    }
    if (ok.click() or ok.hold()) break;
    // oled.clear();
    mySwitch.send(i, byt);
    iy = i;
    oled.setCursor(0, 4);
    oled.print("     ");
    int draw = i;
    while (draw < 1000) {
      draw *= 10;
      oled.print(" ");
    }
    oled.print(i);
    oled.print("/4096      ");
    oled.update();
  }

  while (1) {
    static bool upd = 1;
    tk();
    if (back.click() or back.hold()) {
      // mySwitch.setPulseLength(350);
      return;
    };
    if (ok.click() or ok.hold()) {
      func_custom_translate(iy, byt);
      ELECHOUSE_cc1101.goSleep();
      upd = 1;
    }
    if (up.click()) {
      iy++;
      upd = 1;
    }
    if (down.click()) {
      iy--;
      upd = 1;
    }
    if (upd) {
      upd = 0;
      oled.setCursor(0, 4);
      oled.print("     ");
      long draw = iy;
      while (draw < 1000) {
        draw *= 10;
        oled.print(" ");
      }
      oled.print(iy);
      oled.print("/4096      ");
      oled.update();
    }
  }

  // mySwitch.setPulseLength(350);
}