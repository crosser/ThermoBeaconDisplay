/*
  Listen to advertisements of ThermoBeacons and display
*/

#include <ArduinoBLE.h>

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
        float b = (float)((buf[11] << 8) + buf[10]) / 1000.0;
        float t = (float)((buf[13] << 8) + buf[12]) / 16.0;
        float h = (float)((buf[15] << 8) + buf[14]) / 16.0;
        uint32_t tm = (buf[18] << 16) + (buf[17] << 8) + buf[16];
        Serial.print(dev.address());
        Serial.print(" Bat: ");
        Serial.print(b);
        Serial.print(" Temp: ");
        Serial.print(t);
        Serial.print(" Hum: ");
        Serial.print(h);
        Serial.print(" Tm: ");
        Serial.print(tm);
        Serial.print(" Rssi: ");
        Serial.print(dev.rssi());
        Serial.println();
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
}

void loop() {
  BLE.poll();
}
