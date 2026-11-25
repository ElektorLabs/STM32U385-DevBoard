/*
 * 03_BoardInfo
 *
 * Prints what the chip knows about itself every 2 s: unique ID, flash size,
 * reset cause, die temperature, supply voltage, clocks, microSD card detect
 * and the state of both buttons. The LEDs show a bar graph of the temperature
 * (one LED per 5 C above 20 C).
 */
#include <U385Board.h>

void setup()
{
  Board.begin();
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}

  Serial.println(F("\n=== STM32U385 Board info ==="));
  Serial.print(F("Unique ID   : ")); Serial.println(Board.uniqueId());
  Serial.print(F("Flash size  : ")); Serial.print(Board.flashSizeKB()); Serial.println(F(" KB"));
  Serial.print(F("Reset cause : ")); Serial.println(Board.resetCause());
  Serial.print(F("Core        : Cortex-M33, CMSIS SystemCoreClock = "));
  Serial.print(SystemCoreClock / 1000000UL); Serial.println(F(" MHz"));
  Serial.println(F("--- clocks ---"));
  Board.printClocks(Serial);
}

void loop()
{
  Board.update();

  static uint32_t last = 0;
  if (millis() - last < 2000) return;
  last = millis();

  float t = Board.temperatureC();
  float v = Board.vdda();
  Serial.print(F("T = ")); Serial.print(t, 1);
  Serial.print(F(" C | VDDA = ")); Serial.print(v, 3);
  Serial.print(F(" V | Reg: ")); Serial.print(Board.smpsActive() ? F("SMPS") : F("LDO"));
  Serial.print(F(" | SD: ")); Serial.print(Board.sdCardPresent() ? F("card") : F("none"));
  Serial.print(F(" | User1: ")); Serial.print(Board.isDown(USER1) ? F("down") : F("up"));
  Serial.print(F(" | User2: ")); Serial.println(Board.isDown(USER2) ? F("down") : F("up"));

  int bars = (int)((t - 20.0f) / 5.0f) + 1;
  bars = constrain(bars, 0, 4);
  Board.ledsShow((1U << bars) - 1);
}
