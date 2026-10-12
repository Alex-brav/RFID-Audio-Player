/*
  RFID UID Scanner — setup helper for the RFID music player

  What it does:
    Reads each RFID disk you tap on the reader and prints its ID (UID)
    to the Serial Monitor, for example:

        46219615219642129

  How to use:
    1. Wire the RC522 reader the same way as the main project
       (SDA/SS -> pin 9, RST -> pin 8, plus the SPI pins and 3.3V/GND).
    2. Upload this sketch.
    3. Open the Serial Monitor (Tools > Serial Monitor) at 9600 baud.
    4. Tap one disk on the reader.
    5. Copy the number that appears and paste it into diskLibrary[] in
       the main sketch, replacing one "UID_FOR_DISK_X" placeholder
       (keep the quotation marks).
    6. Repeat for each disk.

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

String lastUid = "";     // the most recent disk, to avoid printing it twice in a row

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
  Serial.println(F("Tap a disk on the reader to see its UID."));
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
    Serial.println(F("(Same disk as last time - tap a different disk.)"));
  } else {
    lastUid = uid;
    Serial.println(uid);
  }

  // Tell the tag to stop so it isn't read again until it's removed and re-tapped
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(500);
}