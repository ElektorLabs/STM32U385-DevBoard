/*
 * 01_LedCycleButtons
 *
 * One LED is lit at a time, in the order PC10 -> PC11 -> PB4 -> PC12.
 *   User1 (SW2)            : next LED
 *   User2 (SW1)            : previous LED
 *   Hold User1 for 1 s     : auto-run on/off (moves every 250 ms)
 *
 * Every button event is also printed on the USB serial port.
 * Don't hold a button while the sketch starts: the level the buttons have at
 * start-up is taken as "released".
 *
 * Board: Tools > Board > Elektor STM32U385 Dev Board (260660)
 */
#include <U385Board.h>

uint8_t current = 0;
bool autoRun = false;
bool holdHandled = false;
uint32_t lastStep = 0;

void show(uint8_t index)
{
  Board.ledsOff();
  Board.led(index, true);
  Serial.print(F("LED ")); Serial.print(index + 1);
  Serial.print(F(" (pin index ")); Serial.print(index); Serial.println(F(") on"));
}

void report(const char *name, U385Button b, const char *what)
{
  Serial.print(name); Serial.print(' '); Serial.print(what);
  Serial.print(F("  (pin now ")); Serial.print(digitalRead(b == USER1 ? BUTTON_USER1 : BUTTON_USER2));
  Serial.print(F(", released level ")); Serial.print(Board.releasedLevel(b)); Serial.println(')');
}

void setup()
{
  Serial.begin(115200);
  Board.begin();
  show(current);
}

void loop()
{
  Board.update();

  // Long press on User1 toggles auto-run (handled once per hold)
  if (Board.heldFor(USER1) >= 1000 && !holdHandled) {
    holdHandled = true;
    autoRun = !autoRun;
    Serial.println(autoRun ? F("auto-run ON") : F("auto-run OFF"));
    Board.ledsAll(true);                        // short flash to confirm
    delay(150);
    show(current);
  }

  if (Board.pressed(USER1)) {
    report("User1 (SW2)", USER1, "pressed");
  }
  if (Board.released(USER1)) {
    report("User1 (SW2)", USER1, "released");
    if (!holdHandled) {                         // short press = next
      current = (current + 1) % U385_LED_COUNT;
      show(current);
    }
    holdHandled = false;
  }

  if (Board.pressed(USER2)) {                   // previous
    report("User2 (SW1)", USER2, "pressed");
    current = (current + U385_LED_COUNT - 1) % U385_LED_COUNT;
    show(current);
  }
  if (Board.released(USER2)) {
    report("User2 (SW1)", USER2, "released");
  }

  if (autoRun && millis() - lastStep >= 250) {
    lastStep = millis();
    current = (current + 1) % U385_LED_COUNT;
    Board.ledsOff();
    Board.led(current, true);
  }
}
