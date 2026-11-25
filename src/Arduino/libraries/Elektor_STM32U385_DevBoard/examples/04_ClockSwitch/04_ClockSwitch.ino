/*
 * 04_ClockSwitch
 *
 * Changes the system clock at run time and shows the effect.
 * Control it from the Serial Monitor (any baud rate except 1200) or the buttons:
 *   n  or User1 (SW2) : next speed  96 -> 48 -> 24 -> 12 MHz -> 96 ...
 *   l  or User2 (SW1) : lock the MSI oscillator to the 32.768 kHz crystal
 *                       (MSI PLL mode) for crystal-accurate timing
 *   c                 : print the clock tree
 *
 * LED1_A blinks at 1 Hz at every speed (millis() follows the clock), while the
 * loop counter printed each second shows how much work the CPU gets done.
 * SYSCLK / 16 is sent to D9 (PA8, MCO) so you can measure it with a scope or
 * frequency counter: 6 MHz at 96 MHz, 750 kHz at 12 MHz.
 *
 * 6 MHz and 3 MHz also exist (SYSCLK_6MHZ, SYSCLK_3MHZ) but are left out here
 * because USB needs a faster bus clock to keep the serial port alive.
 */
#include <U385Board.h>

const U385SysClock speeds[] = { SYSCLK_96MHZ, SYSCLK_48MHZ, SYSCLK_24MHZ, SYSCLK_12MHZ };
const uint8_t speedMHz[] = { 96, 48, 24, 12 };
uint8_t speedIndex = 0;
uint32_t loops = 0, lastReport = 0;

void report()
{
  Serial.println(F("--------------------------------"));
  Board.printClocks(Serial);
}

void nextSpeed()
{
  speedIndex = (speedIndex + 1) % (sizeof(speeds) / sizeof(speeds[0]));
  Serial.print(F("Switching to "));
  Serial.print(speedMHz[speedIndex]);
  Serial.println(F(" MHz..."));
  Serial.flush();
  bool ok = Board.setSystemClock(speeds[speedIndex]);
  Serial.println(ok ? F("done") : F("FAILED"));
  report();
  loops = 0;
  lastReport = millis();
}

void lockToLSE()
{
  Serial.print(F("Starting LSE and locking MSI to it... "));
  Serial.println(Board.msiPllLock() ? F("locked") : F("failed (is X2 fitted?)"));
  report();
}

void setup()
{
  Board.begin();
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}

  Serial.println(F("\n=== Clock switching demo ==="));
  Serial.println(F("n / User1 = next speed, l / User2 = lock MSI to LSE, c = clocks"));
  Board.clockOut(MCO_SYSCLK, 16);
  report();
}

void loop()
{
  static uint32_t lastBlink = 0;
  loops++;
  Board.update();

  if (Board.pressed(USER1)) nextSpeed();
  if (Board.pressed(USER2)) lockToLSE();
  while (Serial.available()) {
    switch (Serial.read()) {
      case 'n': nextSpeed(); break;
      case 'l': lockToLSE(); break;
      case 'c': report(); break;
    }
  }

  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    Board.ledToggle(0);
  }

  if (millis() - lastReport >= 1000) {
    lastReport = millis();
    Serial.print(Board.systemClock() / 1000000UL);
    Serial.print(F(" MHz: "));
    Serial.print(loops);
    Serial.println(F(" loops/s"));
    loops = 0;
  }
}
