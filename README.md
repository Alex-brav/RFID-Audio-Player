# RFID Audio Player

Arduino code for an RFID audio player: placing an RFID-tagged disk on the scanner plays the audio file paired with that tag, and a rotary encoder controls the volume.

> **Building the player?** Follow the instruction manual for assembly, wiring, and setup steps. This README only describes the code in this repository.

---

## Repository Contents

| Path | Description |
|---|---|
| `RFID-SetUp/RFID-SetUp.ino` | Setup sketch. Reads RFID tags and prints each tag's UID to the Serial Monitor. |
| `RFID-Main/RFID-Main.ino` | Main sketch. Runs the audio player. |

Each sketch is inside a folder with the same name, as required by the Arduino IDE.

---

## Dependencies

Both libraries are available through the Arduino IDE Library Manager.

| Library | Used by | Purpose |
|---|---|---|
| [MFRC522](https://github.com/miguelbalboa/rfid) | Both sketches | Communicates with the RC522 RFID reader |
| [DFRobot_DF1201S](https://github.com/DFRobot/DFRobot_DF1201S) | `RFID-Main` | Controls the DFPlayer Pro audio module |

`SPI` and `SoftwareSerial` are also used and come built into the Arduino IDE.

---

## RFID-SetUp

Reads each tag placed on the scanner and prints its UID, for example:

```
RFID UID Scanner ready.
Tap a disk on the reader to see its UID.

46219615219642129
```

**How the UID is formatted:** each byte of the tag's ID is written as a decimal number, and the numbers are joined with no separators. `RFID-Main` builds UIDs the same way, so the printed value can be pasted directly into the main sketch.

**Behavior:**
- On startup, checks that the RC522 is responding and prints `ERROR: RFID reader not found` if not.
- A tag left on the scanner is printed only once.
- Tapping the same tag twice in a row prints `(Same disk as last time - tap a different disk.)` instead of repeating the UID.
- There is no limit on the number of tags that can be scanned.

---

## RFID-Main

### How it works

1. Every 200 ms, the sketch checks the scanner for a tag.
2. If a tag is found, its UID is looked up in `diskLibrary[]`.
3. If the UID is in the library, the paired track plays through the DFPlayer Pro. The track repeats until it is stopped or replaced.
4. If a different tag is placed, its track replaces the current one.
5. If the tag is removed, playback either pauses or continues, depending on the play mode.

The rotary encoder is checked continuously: turning it changes the volume, and pressing it toggles mute.

### Configuration

All settings are near the top of `RFID-Main.ino`.

**Disk library:** pairs each tag UID with a track number. Placeholders must be replaced with real UIDs from `RFID-SetUp`. Lines can be added or removed; the code counts entries automatically.

```cpp
DiskEntry diskLibrary[] = {
  {"UID_FOR_DISK_1",  1},
  {"UID_FOR_DISK_2",  2},
  // ...
};
```

**Play mode:**

```cpp
const bool stopWhenDiskRemoved = true;
```

| Value | Behavior |
|---|---|
| `true` | Audio plays only while the disk is on the scanner. Removing it pauses playback; replacing it restarts the track. |
| `false` | Audio keeps playing after the disk is removed, until a different disk is placed. Placing the same disk again has no effect. |

**Other values:**

| Setting | Default | Description |
|---|---|---|
| `volume` | `15` | Starting volume |
| Volume cap | `20` | Maximum volume reachable with the knob |
| `presenceTimeoutMs` | `800` | Milliseconds a disk can go undetected before it counts as removed |
| `cardCheckIntervalMs` | `200` | Milliseconds between scanner checks |

### Audio file naming

Track numbers map to file names, not to the order files were copied:

| Track number | File played |
|---|---|
| 1 | `/001.mp3` |
| 2 | `/002.mp3` |
| 10 | `/010.mp3` |

Files must be MP3s in the root folder of the DFPlayer Pro, named with exactly three digits.

### Volume and mute

- Turning the encoder changes the volume one step at a time, between 0 and 20.
- Pressing the encoder mutes and saves the current volume. Pressing again restores it.
- Turning the encoder while muted unmutes, continuing from the saved volume.

### Pin assignments

| Define | Pin | Connected to |
|---|---|---|
| `SS_PIN` | D9 | RC522 SDA (SS) |
| `RST_PIN` | D8 | RC522 RST |
| — | D11, D12, D13 | RC522 MOSI, MISO, SCK (hardware SPI) |
| `audioTX` | D2 | DFPlayer Pro TX (Arduino receives) |
| `audioRX` | D3 | DFPlayer Pro RX (Arduino sends) |
| `CLK` | D5 | Encoder CLK |
| `DT` | D6 | Encoder DT |
| `SW` | D7 | Encoder push button |

`audioRX` and `audioTX` are named from the DFPlayer's point of view. `SoftwareSerial` takes the Arduino's RX pin first, so the constructor is `SoftwareSerial(audioTX, audioRX)`.

---

## Serial Monitor Messages

Both sketches print status messages at **9600 baud**.

| Message | Meaning |
|---|---|
| `Initializing DFPlayer...` | Startup in progress |
| `DFPlayer not responding!` | The DFPlayer Pro is not communicating (repeats until it responds) |
| `Playing /001.mp3` | A known disk was detected and its track started |
| `Could not play /001.mp3 ...` | The file is missing or misnamed |
| `Unknown disk - ignoring` | The scanned UID is not in `diskLibrary[]` |
| `Playback stopped (disk missing/changed)` | The disk was removed (only when `stopWhenDiskRemoved` is `true`) |
| `Volume: 12` / `Volume: 0 (muted)` | Volume changed |

A full troubleshooting table is in the instruction manual.

---

## Issues and Contributions

For problems not covered in the instruction manual, please [open an issue](../../issues). Include the board used, the Serial Monitor output, and a photo of the wiring if it may be a hardware problem.

Code suggestions are welcome as pull requests.
