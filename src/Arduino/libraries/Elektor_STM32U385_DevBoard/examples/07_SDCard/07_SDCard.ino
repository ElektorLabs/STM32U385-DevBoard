/*
 * 07_SDCard
 *
 * Needs the "SdFat" library (by Bill Greiman) from the Library Manager.
 * SdFat reads FAT16, FAT32 and exFAT, so any card works as it comes,
 * including 64 GB+ SDXC cards (exFAT). The plain Arduino "SD" library only
 * does FAT16/FAT32.
 *
 * Uses the microSD slot K7 in SPI mode:
 *   SCK PA5, MISO PA6, MOSI PA7 (shared with D11-D13 and mikroBUS)
 *   CS  PC8 (SD_CS), card detect PD2 (SD_DETECT, LOW = card inserted)
 *
 * At start-up it waits for a card, prints the card type and size, lists the
 * files in the root folder and appends one line to U385LOG.TXT.
 *
 * Serial Monitor commands:
 *   l  list files        w  append a line to U385LOG.TXT
 *   r  print U385LOG.TXT d  delete U385LOG.TXT
 *   i  card info
 *
 * LEDs: LED index 0 = card present, 1 = card mounted, 2 flashes on each write,
 * 3 = error. Take the card out and put it back: it is mounted again.
 */
#include <U385Board.h>
#include <SdFat.h>

// The SPI bus is shared with the mikroBUS socket and the Arduino header.
#define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(12))

const char *const LOG_FILE = "U385LOG.TXT";

SdFs sd;                 // FAT16, FAT32 and exFAT
bool present = false;    // card in the slot
bool mounted = false;    // file system opened

void cardInfo()
{
  Serial.print(F("Card type   : "));
  switch (sd.card()->type()) {
    case SD_CARD_TYPE_SD1:  Serial.println(F("SD1")); break;
    case SD_CARD_TYPE_SD2:  Serial.println(F("SD2")); break;
    case SD_CARD_TYPE_SDHC: Serial.println(sd.card()->sectorCount() > 67108864UL ? F("SDXC") : F("SDHC")); break;
    default:                Serial.println(F("unknown")); break;
  }
  Serial.print(F("Card size   : "));
  Serial.print(sd.card()->sectorCount() / 2048UL);    // 512-byte sectors
  Serial.println(F(" MB"));
  Serial.print(F("File system : "));
  uint8_t t = sd.fatType();
  if (t == FAT_TYPE_EXFAT) {
    Serial.println(F("exFAT"));
  } else {
    Serial.print(F("FAT"));
    Serial.println(t);
  }
  Serial.print(F("Free space  : "));
  uint64_t freeBytes = (uint64_t)sd.freeClusterCount() * sd.bytesPerCluster();
  Serial.print((uint32_t)(freeBytes / (1024ULL * 1024ULL)));
  Serial.println(F(" MB"));
}

void listFiles()
{
  Serial.println(F("Files in / (size, name):"));
  sd.ls(&Serial, LS_SIZE);
}

void appendLine()
{
  FsFile f;
  if (!f.open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND)) {
    Serial.println(F("Cannot open the log file for writing."));
    Board.led(3, true);
    return;
  }
  Board.led(2, true);
  f.print(F("uptime_ms="));
  f.print(millis());
  f.print(F(" temp_C="));
  f.print(Board.temperatureC(), 1);
  f.print(F(" vdda_V="));
  f.println(Board.vdda(), 3);
  uint32_t size = f.fileSize();
  f.close();                     // close() writes everything to the card
  Board.led(2, false);
  Serial.print(F("Appended a line, "));
  Serial.print(LOG_FILE);
  Serial.print(F(" is now "));
  Serial.print(size);
  Serial.println(F(" bytes"));
}

void printLog()
{
  FsFile f;
  if (!f.open(LOG_FILE, O_RDONLY)) {
    Serial.println(F("No log file yet (send w)."));
    return;
  }
  Serial.print(F("--- "));
  Serial.print(LOG_FILE);
  Serial.println(F(" ---"));
  int c;
  while ((c = f.read()) >= 0) {
    Serial.write((uint8_t)c);
  }
  f.close();
  Serial.println(F("--- end ---"));
}

bool mount()
{
  Board.ledsOff();
  Board.led(0, true);
  if (!sd.begin(SD_CONFIG)) {
    Serial.print(F("Mount failed, SdFat error 0x"));
    Serial.print(sd.sdErrorCode(), HEX);
    Serial.println(sd.card()->sectorCount() ? F(" (card answers: no FAT/exFAT volume, format it)")
                                            : F(" (card does not answer: push it fully in)"));
    Board.led(3, true);
    return false;
  }
  Board.led(1, true);
  Serial.println(F("Card mounted."));
  cardInfo();
  return true;
}

void setup()
{
  Board.begin();
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}

  Serial.println(F("\n=== microSD card ==="));
  Serial.println(F("Commands: l list, w write, r read, d delete, i info"));
  if (!Board.sdCardPresent()) {
    Serial.println(F("No card in the slot. Insert one..."));
  }
}

void loop()
{
  // Mount when a card goes in, forget it when it comes out.
  bool now = Board.sdCardPresent();
  if (now != present) {
    delay(300);                                   // let the contacts settle
    present = Board.sdCardPresent();
    if (present) {
      mounted = mount();
      if (mounted) {
        listFiles();
        appendLine();
        printLog();
      }
    } else {
      sd.end();
      mounted = false;
      Board.ledsOff();
      Serial.println(F("Card removed."));
    }
  }

  while (Serial.available()) {
    char c = Serial.read();
    if (c <= ' ') continue;
    if (!mounted) {
      Serial.println(F("No card mounted."));
      continue;
    }
    switch (c) {
      case 'l': listFiles(); break;
      case 'w': appendLine(); break;
      case 'r': printLog(); break;
      case 'i': cardInfo(); break;
      case 'd':
        Serial.println(sd.remove(LOG_FILE) ? F("Log file deleted.") : F("Nothing to delete."));
        break;
      default:
        Serial.println(F("Commands: l list, w write, r read, d delete, i info"));
        break;
    }
  }
}
