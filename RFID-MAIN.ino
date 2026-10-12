#include <SPI.h>
#include <MFRC522.h>
#include "SoftwareSerial.h"
#include <DFRobot_DF1201S.h>

// =====================================================================
//  PLAY MODE SETTING  —  choose how the player reacts when a disk is removed
// =====================================================================
//
//  true  = "Play only while the disk is on the reader"
//          The track plays while the disk sits on the reader and pauses
//          shortly after the disk is taken off. Putting the same disk
//          back starts its track over from the beginning.
//
//  false = "Keep playing after the disk is removed"
//          Tapping a disk starts its track, and it keeps playing after
//          the disk is taken away. Tapping a DIFFERENT disk switches to
//          that disk's track. Tapping the SAME disk again does nothing.
//          Because the player is set to repeat the current track, it
//          will keep looping until another disk is tapped. Press the
//          volume knob to mute it.
//
const bool stopWhenDiskRemoved = true;

// RFID reader
#define SS_PIN 9
#define RST_PIN 8
MFRC522 mfrc522(SS_PIN, RST_PIN);

// --- Disk library: map each UID to a track number ---
// Replace each "UID_FOR_DISK_X" with the UID printed by the
// RFID_UID_Scanner sketch.
//
// The track number decides which audio file plays. Files must be
// MP3s in the main folder of the DFPlayer, named with three digits:
//   track 1  -> 001.mp3
//   track 2  -> 002.mp3
//   track 10 -> 010.mp3
// The order the files were copied in does NOT matter, only the name.
struct DiskEntry {
  const char* uid;
  int trackNumber;
};

DiskEntry diskLibrary[] = {
  {"UID_FOR_DISK_1",  1},
  {"UID_FOR_DISK_2",  2},
  {"UID_FOR_DISK_3",  3},
  {"UID_FOR_DISK_4",  4},
  {"UID_FOR_DISK_5",  5},
  {"UID_FOR_DISK_6",  6},
  {"UID_FOR_DISK_7",  7},
  {"UID_FOR_DISK_8",  8},
  {"UID_FOR_DISK_9",  9},
  {"UID_FOR_DISK_10", 10},
};
const int numDisks = sizeof(diskLibrary) / sizeof(diskLibrary[0]);

bool isPlaying = false;
String currentDisk = "";  // which disk's track is currently playing

// DFPlayer
// Note: audioRX / audioTX are named from the DFPlayer's point of view.
// SoftwareSerial takes (Arduino RX pin, Arduino TX pin), so the Arduino
// listens on pin 2 (wired to the DFPlayer's TX) and talks on pin 3
// (wired to the DFPlayer's RX).
#define audioRX 3
#define audioTX 2
SoftwareSerial DF1201SSerial(audioTX, audioRX);
DFRobot_DF1201S player;

// Volume / encoder
int volume = 15;
#define CLK 5
#define DT 6
#define SW 7
int lastStateCLK;

// Mute button (encoder push)
bool isMuted = false;
int volumeBeforeMute = 15;   // volume to return to when unmuting
int lastSWState = HIGH;      // button reads HIGH when not pressed (INPUT_PULLUP)
unsigned long lastSWTime = 0;
const unsigned long swDebounceMs = 200;

// RFID poll timing
unsigned long lastCardCheck = 0;
const unsigned long cardCheckIntervalMs = 200;

// Presence timeout: how long a disk can go unseen before it counts as removed
// (only used when stopWhenDiskRemoved is true)
unsigned long lastSeenTime = 0;
const unsigned long presenceTimeoutMs = 800;

void setup() {
  Serial.begin(9600);

  SPI.begin();
  mfrc522.PCD_Init();
  DF1201SSerial.begin(115200);

  Serial.println("Initializing DFPlayer...");
  while (!player.begin(DF1201SSerial)) {
    Serial.println("DFPlayer not responding!");
    delay(1000);
  }

  player.setVol(volume);

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  lastStateCLK = digitalRead(CLK);

  player.switchFunction(player.MUSIC);
  delay(2000);

  // SINGLECYCLE = repeat the current track over and over.
  // In "play only while present" mode, the track loops for as long as
  // the disk stays on the reader. In "keep playing" mode, it loops until
  // a different disk is tapped.
  player.setPlayMode(player.SINGLECYCLE);
}

void loop() {
  setVolume();

  if (millis() - lastCardCheck >= cardCheckIntervalMs) {
    lastCardCheck = millis();
    checkDisk();
  }
}

// Look up a UID in the library. Returns track number, or -1 if not found.
int findTrackForDisk(String uid) {
  for (int i = 0; i < numDisks; i++) {
    if (uid == diskLibrary[i].uid) {
      return diskLibrary[i].trackNumber;
    }
  }
  return -1;
}

// Turn a track number into a file path, e.g. 3 -> "/003.mp3"
String fileNameForTrack(int track) {
  String name = String(track);
  while (name.length() < 3) {
    name = "0" + name;   // add leading zeros until it's 3 digits
  }
  return "/" + name + ".mp3";
}

void checkDisk() {
  bool sawKnownDisk = false;

  // WakeupA also wakes tags that were halted, so a disk left sitting on
  // the reader is detected on every check (needed to know it's still there).
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  MFRC522::StatusCode result = mfrc522.PICC_WakeupA(bufferATQA, &bufferSize);

  if (result == MFRC522::STATUS_OK || result == MFRC522::STATUS_COLLISION) {
    if (mfrc522.PICC_ReadCardSerial()) {
      String disk = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        disk += mfrc522.uid.uidByte[i];
      }
      Serial.println(disk);

      int track = findTrackForDisk(disk);

      if (track != -1) {
        sawKnownDisk = true;
        lastSeenTime = millis();

        // Only start a track if it's a different disk than what's
        // currently playing, OR nothing is currently playing
        if (!isPlaying || disk != currentDisk) {
          String fileName = fileNameForTrack(track);

          if (player.playSpecFile(fileName)) {
            Serial.print("Playing ");
            Serial.println(fileName);
          } else {
            Serial.print("Could not play ");
            Serial.print(fileName);
            Serial.println(" - check that the file exists and is named correctly");
          }

          isPlaying = true;
          currentDisk = disk;
        }
      } else {
        Serial.println("Unknown disk - ignoring");
      }

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  }

  // Disk removed: only pause if the play mode says to
  if (stopWhenDiskRemoved && isPlaying && !sawKnownDisk) {
    if (millis() - lastSeenTime > presenceTimeoutMs) {
      player.pause();
      isPlaying = false;
      currentDisk = "";
      Serial.println("Playback stopped (disk missing/changed)");
    }
  }
}

void printVolume() {
  Serial.print("Volume: ");
  Serial.print(volume);
  if (isMuted) Serial.print(" (muted)");
  Serial.println();
}

void setVolume() {
  int currentCLK = digitalRead(CLK);
  int currentSW = digitalRead(SW);

  // --- Button: press once to mute, press again to unmute ---
  // Only reacts at the moment the button goes down, so holding it
  // doesn't toggle over and over.
  if (currentSW == LOW && lastSWState == HIGH &&
      millis() - lastSWTime > swDebounceMs) {
    lastSWTime = millis();

    if (isMuted) {
      isMuted = false;
      volume = volumeBeforeMute;   // go back to the old volume
    } else {
      isMuted = true;
      volumeBeforeMute = volume;   // remember it for later
      volume = 0;
    }
    player.setVol(volume);
    printVolume();
  }
  lastSWState = currentSW;

  // --- Knob: turn to change volume ---
  if (currentCLK != lastStateCLK && currentCLK == LOW) {
    // Turning the knob while muted unmutes, starting from the old volume
    if (isMuted) {
      isMuted = false;
      volume = volumeBeforeMute;
    }

    if (digitalRead(DT) != currentCLK) {
      volume++;
    } else {
      volume--;
    }

    if (volume > 20) volume = 20;
    if (volume < 0) volume = 0;

    player.setVol(volume);
    printVolume();
  }

  lastStateCLK = currentCLK;
}