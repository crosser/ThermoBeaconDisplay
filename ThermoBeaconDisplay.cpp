/*
  Listen to advertisements of ThermoBeacons and display.
  Shows values from two ThermoBeacons with highest RSSI (i.e. the nearest).

  For Lilygo T-Display. TFT_eSPI needs to be configured for that.
  Text formatting for 138x240 display, in landscape mode.
  Use --fqbn esp32:esp32:esp32
  Do _not_ use the copy of TFT_eSPI that Liligo's manual refers to,
  but do take the user-selecatble configuration file from it.
*/

#include <Arduino.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ArduinoBLE.h>
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define SLOTS 2
#define MAXAGE 100

struct toggle {
    bool state = false;
    unsigned long last = 0L;
} toggle;
bool lasttoggle = false;

struct entry {
    char addr[12];
    int bat;
    int tmp;
    int hum;
    int ticks;
    int rssi;
};

struct elem {
    struct elem *next;
    int age;
    struct entry entry;
};

struct elem *head = NULL;

void dbgEntry(struct entry *entry)
{
    if (entry) {
	Serial.print(entry->addr);
	Serial.print(" Bat: ");
	Serial.print(entry->bat);
	Serial.print(" Temp: ");
	Serial.print(entry->tmp);
	Serial.print(" Hum: ");
	Serial.print(entry->hum);
	Serial.print(" Tm: ");
	Serial.print(entry->ticks);
	Serial.print(" Rssi: ");
	Serial.print(entry->rssi);
    } else {
	Serial.print("<NULL ENTRY>");
    }
}

void dbgOp(int pos, String str1, struct entry *entry1,
	   String str2, struct entry *entry2)
{
#if 0
    Serial.print("pos ");
    Serial.print(pos);
    Serial.print(" ");
    Serial.print(str1);
    Serial.print(" ");
    dbgEntry(entry1);
    if (str2) {
	Serial.print(" ");
	Serial.print(str2);
	Serial.print(" ");
	dbgEntry(entry2);
    }
    Serial.println();
#endif
}

void dbgVal(int oldv, int newv, int xpos, int yf, int yt, int colour) {
#if 0
    Serial.print("oldv ");
    Serial.print(oldv);
    Serial.print(" newv ");
    Serial.print(newv);
    Serial.print(" xpos ");
    Serial.print(xpos);
    Serial.print(" yf ");
    Serial.print(yf);
    Serial.print(" yt ");
    Serial.print(yt);
    Serial.print(" colour ");
    Serial.print(colour);
    Serial.println();
#endif
}

void clrtop(int xpos, int yf, int yt) {
    tft.setViewport(xpos * (tft.width() / 2) + 10, yf + 2,
		    (tft.width() / 2) - 20 , yt - 2);
    tft.fillScreen(TFT_BLACK);
    tft.resetViewport();
}

void dispaddr(char *oldv, char *newv, int xpos, int yf, int yt) {
    if (oldv && newv && strcmp(oldv, newv) == 0) return;
    tft.setViewport(xpos * (tft.width() / 2) + 10, yf + 2,
		    (tft.width() / 2) - 20 , yt - 2);
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 4);
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.print(newv + 3);
    tft.resetViewport();
}

void dispbat(int oldv, int newv, int xpos, int yf, int yt) {
    int olen = (oldv - 230) * 30 / 80;
    if (olen < 0) olen = 0;
    if (olen > 30) olen = 30;
    int nlen = (newv - 230) * 30 / 80;
    if (nlen < 0) nlen = 0;
    if (nlen > 30) nlen = 30;
    dbgVal(olen, nlen, xpos, yf, yt, 0);
    if (olen == nlen) return;
    tft.setViewport(xpos * (tft.width() / 2) + 10, yf + 2, 40, yt - 2);
    tft.fillScreen(TFT_BLACK);
    int colour = nlen > 5 ? TFT_GREEN : TFT_RED;
    tft.drawRect(5, 10, 30, tft.height() - 15, colour);
    tft.fillRect(5, 10, nlen, tft.height() - 15, colour);
    tft.resetViewport();
}


void disprssi(int oldv, int newv, int xpos, int yf, int yt) {
    dbgVal(oldv, newv, xpos, yf, yt, -8);
    int olen = (oldv + 100) / 10;
    if (olen < 0) olen = 0;
    if (olen > 4) olen = 4;
    int nlen = (newv + 100) / 10;
    if (nlen < 0) nlen = 0;
    if (nlen > 4) nlen = 4;
    dbgVal(olen, nlen, xpos, yf, yt, -9);
    if (olen == nlen) return;
    tft.setViewport(xpos * (tft.width() / 2) + 70, yf + 2, 40, yt - 2);
    tft.fillScreen(TFT_BLACK);
    int colour = nlen > 1 ? TFT_GREEN : TFT_RED;
    int maxy = tft.height() - 15;
    for (int i = 0; i < nlen; i++) {
        int h = (i + 1) * maxy / 5;
        tft.fillRect(5 + i * 6, 10 + maxy - h, 3, h, colour);
    }
    tft.resetViewport();
}

void dispel(int oldv, int newv, int xpos, int yf, int yt, int colour) {
    dbgVal(oldv, newv, xpos, yf, yt, colour);
    if (oldv == newv) return;
    tft.setViewport(xpos * (tft.width() / 2) + 10, yf + 2,
		    tft.width() / 2 - 12, yt - 4);
    tft.setTextSize(2);
    if (oldv) {
	tft.setCursor(0, 0, 4);
        tft.setTextColor(TFT_BLACK);
        tft.print(oldv / 10);
        tft.print(".");
        tft.print(oldv % 10);
    } else {
        tft.fillScreen(TFT_BLACK);
    }
    if (newv) {
        tft.fillScreen(TFT_BLACK);  // TODO: must not have this. Need debugging
	tft.setCursor(0, 0, 4);
        tft.setTextColor(colour);
        tft.print(newv / 10);
        tft.print(".");
        tft.print(newv % 10);
    } else {
        tft.fillScreen(TFT_BLACK);
    }
    tft.resetViewport();
}

#define FL(st, fl) (st ? st->fl : 0)

void display(int pos, entry *oldval, entry *newval)
{
    if (!oldval) clrtop(pos, 0, 30);
    if (toggle.state) {
	dispaddr(FL(oldval, addr), FL(newval, addr), pos, 0, 30);
    } else {
	dispbat(FL(oldval, bat), FL(newval, bat), pos, 0, 30);
	disprssi(FL(oldval, rssi), FL(newval, rssi), pos, 0, 30);
    }
    dispel(FL(oldval, tmp), FL(newval, tmp), pos, 30, 50, TFT_YELLOW);
    dispel(FL(oldval, hum), FL(newval, hum), pos, 80, 50, TFT_CYAN);
}

void updateCache(struct entry *newentry)
{
    struct elem **cur_p = &head;
    struct elem *cur = *cur_p;
    struct elem *slots[SLOTS] = { NULL };
    struct elem *target = NULL;
    struct elem *to_remove = NULL;
    int pos = 0;
    bool not_yet_placed = true;

    while (true) {
	bool step_ahead = true;  // Omit advancing if an element was dropped
	bool at_the_end = !cur;
	if (pos < SLOTS)
	    slots[pos] = cur;
	pos++;
	// First handle insertion because it _can_ happen with cur == NULL
	if (not_yet_placed &&
		(at_the_end || cur->entry.rssi < newentry->rssi)) {
	    dbgOp(pos, "potentially insert", newentry,
		  "before", cur ? &(cur->entry) : NULL);
	    if (cur && strcmp(cur->entry.addr, newentry->addr) == 0) {
		dbgOp(pos, "update instead of insert", newentry,
		      "replacing", &(cur->entry));
		target = cur;
	    } else {
		dbgOp(pos, "really insert", newentry,
		           "before", cur ? &(cur->entry) : NULL);
		(*cur_p) = (struct elem *) malloc(sizeof(struct elem));
		(*cur_p)->next = cur;
		(*cur_p)->age = 0;
		(*cur_p)->entry = *newentry;
	    }
            not_yet_placed = false;
	}
	if (at_the_end)
	    break;
	// The rest of the operations in the loop assume that `cur` exists
	(cur->age)++;
	if (strcmp(cur->entry.addr, newentry->addr) == 0
	    || cur->age > MAXAGE) {
	    dbgOp(pos, "potentially delete", &(cur->entry), "", NULL);
	    if (target == cur) {
		dbgOp(pos, "do not delete", &(cur->entry),
		      "scheduled for replacement by", newentry);
	    } else {
		if (strcmp(cur->entry.addr, newentry->addr) == 0 &&
		    (!(cur->next)
		     || cur->next->entry.rssi < newentry->rssi)) {
		    dbgOp(pos, "update instead of delete", &(cur->entry),
			  "already at the insersion point for", newentry);
		    target = cur;
		    not_yet_placed = false;
		} else {
		    dbgOp(pos, "really delete", &(cur->entry), "", NULL);
		    (*cur_p) = cur->next;
		    cur->next = to_remove;
		    to_remove = cur;
		    cur = *cur_p;
		    step_ahead = false;  // Don't advance in the list
		}
	    }
	}
	// Proceed to the next element of the list
	if (step_ahead) {
	    cur_p = &(cur->next);
	    cur = *cur_p;
	}
    }
    for (; pos < SLOTS; pos++) slots[pos] = NULL;
    // Finished modifying the list, now display
    bool stgl = toggle.state == lasttoggle;
    for (cur = head, pos = 0; pos < SLOTS;
	 cur = cur ? cur->next : cur, pos++) {
	if (target && (slots[pos] == target)) {
	    // old = slots[pos], new = nentry
	    dbgOp(pos, "Display before updating, old",
		  slots[pos] ? &(slots[pos]->entry) : NULL, "new",
		  newentry);
	    display(pos, (stgl && slots[pos]) ? &(slots[pos]->entry) : NULL,
		    newentry);
	} else {
	    //old = slots[pos], new = cur
	    dbgOp(pos, "Display as different elems, old",
		  slots[pos] ? &(slots[pos]->entry) : NULL, "new",
		  cur ? &(cur->entry) : NULL);
	    display(pos, (stgl && slots[pos]) ? &(slots[pos]->entry) : NULL,
		    cur ? &(cur->entry) : NULL);
	}
    }
    lasttoggle = toggle.state;
    // Now we can actually update the entry
    if (target) {
	target->entry = *newentry;
	target->age = 0;
    }
    // And free elements that we took off the cache
    for (cur = to_remove; cur; ) {
	struct elem *next = cur->next;
	free(cur);
	cur = next;
    }
}

void dbgDevEv(BLEDevice peripheral)
{
#if 0
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
#endif
}

void advHandler(BLEDevice dev)
{
    if (dev.address().substring(0, 6) == "a3:e4:") {
	dbgDevEv(dev);
	if (dev.hasManufacturerData()) {
	    int len = dev.manufacturerDataLength();
	    if (len == 20) {
		uint8_t buf[20];
		dev.manufacturerData(buf, len);
		struct entry newentry = {
		    .bat = ((buf[11] << 8) + buf[10]) / 10,
		    .tmp = ((buf[13] << 8) + buf[12]) * 10 / 16,
		    .hum = ((buf[15] << 8) + buf[14]) * 10 / 16,
		    .ticks = (buf[18] << 16) + (buf[17] << 8) + buf[16],
		    .rssi = dev.rssi()
		};
		strncpy(newentry.addr, dev.address().substring(6).c_str(),
			sizeof(newentry.addr));
		newentry.addr[sizeof(newentry.addr) - 1] = '\0';
		updateCache(&newentry);
	    }
	}
    }
}

void ARDUINO_ISR_ATTR isr(void *arg) {
    struct toggle *tptr = (struct toggle *)arg;
    unsigned long now = millis();
    if (now - tptr->last < 100) return;
    tptr->last = now;
    tptr->state = !tptr->state;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial);
    if (!BLE.begin()) {
	Serial.println("starting Bluetooth® Low Energy module failed!");
	while (1);
    }

    pinMode(35, INPUT_PULLUP);
    attachInterruptArg(35, isr, &toggle, RISING);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setViewport(0, 0, tft.width() / 2, tft.height());
    tft.frameViewport(TFT_NAVY, 1);
    tft.resetViewport();
    tft.setViewport(tft.width() / 2, 0, tft.width() / 2, tft.height());
    tft.frameViewport(TFT_NAVY, 1);
    tft.resetViewport();

    BLE.setEventHandler(BLEDiscovered, advHandler);
    BLE.scan(true);
}

void loop()
{
    BLE.poll();
}
