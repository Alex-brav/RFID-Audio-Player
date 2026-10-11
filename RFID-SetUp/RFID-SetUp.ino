/*
  RFID UID Scanner — setup helper for the RFID music player

  What it does:
    Reads each RFID disk you tap on the reader and prints its ID (UID)
    to the Serial Monitor as a ready-to-paste line for the main sketch:

        {"46219615219642129", 1},

  How to use:
    1. Wire the RC522 reader the same way as the main project
       (SDA/SS -> pin 9, RST -> pin 8, plus the SPI pins and 3.3V/GND).
    2. Upload this sketch.
    3. Open the Serial Monitor (Tools > Serial Monitor) at 9600 baud.
    4. Tap each disk on the reader, one at a time, in the order you
       want the tracks to play (disk 1 = track 1, disk 2 = track 2, ...).
    5. Copy the printed lines into diskLibrary[] in the main sketch,
       replacing the placeholder "UID_FOR_DISK_X" entries.

  Note: the UID is built the same way as in the main sketch (each byte
  written as a decimal number, joined with no spaces), so the IDs will
  match exactly.
*/

#include <SPI.h>
#include <MFRC522.h>

// Same pins as the main project
#define SS_PIN 9
#define RST_PIN 8
MFRC522 mfrc522(SS_PIN, RST_PIN);

int diskCount = 0;       // how many disks have been scanned so far
String lastUid = "";     // the most recent disk, to avoid double-counting

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  delay(50);

  // Check that the reader is actually connected.
  // A version of 0x00 or 0xFF means the Arduino can't talk to it.
  byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println(F("ERROR: RFID reader not found. Check your wiring and restart."));
    while (true) { }  // stop here
  }

  Serial.println(F("RFID UID Scanner ready."));
  Serial.println(F("Tap each disk on the reader, one at a time."));
  Serial.println(F("Copy each line below into diskLibrary[] in the main sketch."));
  Serial.println();
}

void loop() {
  // Wait until a new tag is placed on the reader
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Build the UID string (same method as the main sketch)
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += mfrc522.uid.uidByte[i];
  }

  if (uid == lastUid) {
    Serial.println(F("(Same disk as last time - skipped. Tap the next disk.)"));
  } else {
    diskCount++;
    lastUid = uid;

    // Print a ready-to-paste line, e.g.  {"46219615219642129", 1},
    Serial.print(F("  {\""));
    Serial.print(uid);
    Serial.print(F("\", "));
    Serial.print(diskCount);
    Serial.println(F("},"));
  }

  // Tell the tag to stop so it isn't read again until it's removed and re-tapped
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(500);
}