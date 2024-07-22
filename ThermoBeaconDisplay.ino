/*
  Listen to advertisements of ThermoBeacons and display
*/

#include <stdbool.h>
#include <stdlib.h>
#include <ArduinoBLE.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#define SLOTS 2
#define MAXAGE 100

struct entry {
  struct entry *next;
  char addr[12];
  int bat;
  int tmp;
  int hum;
  int ticks;
  int rssi;
  int age;
};

struct entry *head;

String viewports[] = { "00:00:0b:15", "00:00:01:40" };

TFT_eSPI tft = TFT_eSPI();

void displayT(String addr, int bat, int tmp, int hum, int ticks, int rssi) {

  for (int i = 0; i <= 1; i++) {
    if (viewports[i] == addr) {
      tft.setViewport(i * (tft.width() / 2) + 4, 4, tft.width() / 2 - 8, tft.height() - 8);
    }
  }
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 4);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.println(addr.substring(3));
  tft.setTextColor(TFT_RED);
  tft.setTextSize(2);
  tft.print(tmp / 10);
  tft.print(".");
  tft.println(tmp % 10);
  tft.setTextColor(TFT_BLUE);
  tft.print(hum / 10);
  tft.print(".");
  tft.println(hum % 10);
  // tft.setTextColor(TFT_GREEN);
  // tft.setTextSize(1);
  // tft.print(bat / 100);
  // tft.print(".");
  // tft.print(bat % 100);
  // tft.print(" ");
  // tft.println(rssi);
  tft.resetViewport();
}

void updateCache(String addr, int bat, int tmp, int hum, int ticks, int rssi) {
  struct entry *update = NULL;
  struct entry **cur_p = &head;
  struct entry *cur = *cur_p;
  struct entry *slots[SLOTS] = {NULL};
  int pos = 0;
  bool needinsert = true;
  bool mayupdate = true;
  struct entry nentry;
  
  while (cur) {
    if (pos < SLOTS) slots[pos] = cur;
    if (strcmp(cur->addr, addr.c_str()) == 0) {
      Serial.print("found same address ");
      Serial.print(addr);
      Serial.print(" at pos ");
      Serial.println(pos);
      if (mayupdate && ((cur->rssi <= rssi) || !cur->next || (cur->next->rssi <= rssi))) {
        Serial.print("at suitable insertion point at pos ");
        Serial.print(pos);
        Serial.print(" new rssi ");
        Serial.print(rssi);
        Serial.print(" current rssi ");
        Serial.print(cur->rssi);
        Serial.print(" next rssi ");
        Serial.println(cur->next ? cur->next->rssi : 999999);
        needinsert = false;
        update = cur;
        nentry = (struct entry) {
          .bat = bat,
          .hum = hum,
          .ticks = ticks,
          .rssi = rssi
          };
        strncpy(nentry.addr, addr.c_str(), sizeof(nentry.addr));
        nentry.addr[sizeof(nentry.addr) - 1] = '\0';
      } else {
        Serial.print("removing same address at pos ");
        Serial.println(pos);
        (*cur_p) = cur->next;
        free(cur);
        cur = *cur_p;
        if (pos < SLOTS) slots[pos] = NULL;
      }
    }
    if (cur->age > MAXAGE) {
        Serial.print("removing old age pos ");
        Serial.println(pos);
        (*cur_p) = cur->next;
        free(cur);
        cur = *cur_p;
        if (pos < SLOTS) slots[pos] = NULL;
    } else {
      Serial.print("Increaing age for ");
      Serial.println(cur->addr);
      cur->age++;
    }
    if (needinsert && (cur->rssi < rssi)) {
      Serial.print("inserting addr ");
      Serial.print(addr);
      Serial.print(" at pos ");
      Serial.println(pos);
      (*cur_p) = (struct entry*)malloc(sizeof(struct entry));
      (**cur_p) = (struct entry) {
        .next = cur,
        .bat = bat,
        .hum = hum,
        .ticks = ticks,
        .rssi = rssi
      };
      strncpy((*cur_p)->addr, addr.c_str(), sizeof(nentry.addr));
      (*cur_p)->addr[sizeof(nentry.addr) - 1] = '\0';
      needinsert = false;
      mayupdate = false;
    }
    cur_p = &(cur->next);
    cur = *cur_p;
    pos++;
  }
  if (needinsert) {
    Serial.print("inserting addr ");
    Serial.print(addr);
    Serial.println(" at the tail");
    (*cur_p) = (struct entry*)malloc(sizeof(struct entry));
    (**cur_p) = (struct entry) {
      .next = cur,
      .bat = bat,
      .hum = hum,
      .ticks = ticks,
      .rssi = rssi
    };
    strncpy((*cur_p)->addr, addr.c_str(), sizeof(nentry.addr));
    (*cur_p)->addr[sizeof(nentry.addr) - 1] = '\0';
  }
  for (cur = head, pos = 0; pos < SLOTS; cur = cur ? cur->next : cur, pos++) {
    if (update && (slots[pos] == update)) {
      // old = slots[pos], new = nentry
      Serial.print("Display as updated pos ");
      Serial.print(pos);
      Serial.print(" addr was ");
      Serial.print(slots[pos] ? slots[pos]->addr : "NONE");
      Serial.print(" now ");
      Serial.println(nentry.addr);
    } else {
      //old = slots[pos], new = cur
      Serial.print("Display as new pos ");
      Serial.print(pos);
      Serial.print(" addr was ");
      Serial.print(slots[pos] ? slots[pos]->addr : "NONE");
      Serial.print(" now ");
      Serial.println(cur ? cur->addr : "NULL");
    }
  }
  if (update) {
    update->bat = nentry.bat;
    update->hum = nentry.hum;
    update->ticks = nentry.ticks;
    update->rssi = nentry.rssi;
    update->age = 0;
  }
  displayT(addr, bat, tmp, hum, ticks, rssi);
}

void dbg(BLEDevice peripheral) {
  Serial.print("Address: ");
  Serial.println(peripheral.address());
  Serial.print("RSSI: ");
  Serial.println(peripheral.rssi());
  if (peripheral.hasAdvertisementData()) {
    uint8_t buf[128];
    int m = peripheral.advertisementDataLength();
    peripheral.advertisementData(buf, m);
    Serial.print("ADV: ");
    for (int i = 0; i < m; i++)
      Serial.print(buf[i], HEX);
    Serial.println();
  }
  if (peripheral.hasManufacturerData()) {
    uint8_t buf[128];
    int m = peripheral.manufacturerDataLength();
    peripheral.manufacturerData(buf, m);
    Serial.print("MFR len: ");
    Serial.print(m);
    Serial.print(" data: ");
    for (int i = 0; i < m; i++) {
      Serial.print(buf[i] >> 4, HEX);
      Serial.print(buf[i] & 0x0f, HEX);
    }
    Serial.println();
  }
  Serial.println();
}

void advHandler(BLEDevice dev) {
  if (dev.address().substring(0,6) == "a3:e4:") {
    // dbg(dev);
    if (dev.hasManufacturerData()) {
      int len = dev.manufacturerDataLength();
      if (len == 20) {
        uint8_t buf[20];
        dev.manufacturerData(buf, len);
        int bat = ((buf[11] << 8) + buf[10]) / 10;
        int tmp = ((buf[13] << 8) + buf[12]) * 10 / 16;
        int hum = ((buf[15] << 8) + buf[14]) * 10 / 16;
        uint32_t ticks = (buf[18] << 16) + (buf[17] << 8) + buf[16];
        /*
        Serial.print(dev.address());
        Serial.print(" Bat: ");
        Serial.print(bat);
        Serial.print(" Temp: ");
        Serial.print(tmp);
        Serial.print(" Hum: ");
        Serial.print(hum);
        Serial.print(" Tm: ");
        Serial.print(ticks);
        Serial.print(" Rssi: ");
        Serial.print(dev.rssi());
        Serial.println();
        */
        updateCache(dev.address().substring(6), bat, tmp, hum, ticks, dev.rssi());
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial);
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }
  BLE.setEventHandler(BLEDiscovered, advHandler);
  BLE.scan(true);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setViewport(0, 0, tft.width() / 2, tft.height());
  tft.frameViewport(TFT_NAVY, 1);
  tft.resetViewport();
  tft.setViewport(tft.width() / 2, 0, tft.width() / 2, tft.height());
  tft.frameViewport(TFT_NAVY, 1);
  tft.resetViewport();
}

void loop() {
  BLE.poll();
}
