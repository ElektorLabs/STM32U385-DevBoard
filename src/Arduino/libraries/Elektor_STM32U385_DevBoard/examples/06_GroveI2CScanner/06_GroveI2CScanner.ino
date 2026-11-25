/*
 * 06_GroveI2CScanner
 *
 * Switches on power to the Grove I2C connector (K4) and scans I2C1 for
 * devices. The same bus goes to the Qwiic connector (K8), the mikroBUS socket
 * and SDA/SCL on the Arduino header; Wire uses PB7/PB6 with the board's own
 * 4.7 k pull-ups, so no setup is needed.
 *
 * Press User1 (SW2) to scan again. LEDs show the number of devices found.
 */
#include <U385Board.h>
#include <Wire.h>

void scan()
{
  Serial.println(F("Scanning I2C1 (SDA = PB7, SCL = PB6)..."));
  uint8_t found = 0;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  device at 0x"));
      if (addr < 0x10) Serial.print('0');
      Serial.println(addr, HEX);
      found++;
    }
  }
  Serial.print(found);
  Serial.println(F(" device(s) found"));
  Board.ledsShow(found > 4 ? 0x0F : (1U << found) - 1);
}

void setup()
{
  Board.begin();
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}

  Board.groveI2CPower(true);    // TPS22919 load switch on
  delay(50);                    // let the module power up
  Wire.begin();
  Wire.setClock(100000);

  Serial.println(F("\n=== Grove / Qwiic I2C scanner ==="));
  scan();
}

void loop()
{
  Board.update();
  if (Board.pressed(USER1)) {
    scan();
  }
}
