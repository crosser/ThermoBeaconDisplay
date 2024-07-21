/*
  Listen to advertisements of ThermoBeacons and display
*/

#include <stdlib.h>
#include <ArduinoBLE.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#define SLOTS 2

struct entry {
  char addr[12];
  int bat;
  int tmp;
  int hum;
  int ticks;
  int rssi;
};

struct entry **head;
struct entry *slot[SLOTS];

String viewports[] = { "00:00:0b:15", "00:00:01:40" };

TFT_eSPI tft = TFT_eSPI();

void displayT(String addr, int bat, int tmp, int hum, int ticks, int rssi) {

  for (int i = 0; i <= 1; i++) {
    if (viewports[i] == addr) {
      tft.setViewport(i * (tft.width() / 2), 0, tft.width() / 2, tft.height());
    }
  }
  tft.fillScreen(TFT_BLACK);
  tft.frameViewport(TFT_NAVY, 1);
  tft.setCursor(4, 4, 4);
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
        displayT(dev.address().substring(6), bat, tmp, hum, ticks, dev.rssi());
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
  tft.resetViewport();
  tft.setViewport(tft.width() / 2, 0, tft.width() / 2, tft.height());
  tft.resetViewport();
}

void loop() {
  BLE.poll();
}
