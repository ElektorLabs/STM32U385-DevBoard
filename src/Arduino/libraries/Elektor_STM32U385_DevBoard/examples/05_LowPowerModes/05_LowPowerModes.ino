/*
 * 05_LowPowerModes
 *
 * Needs two libraries from the Library Manager:
 *   "STM32duino Low Power" and "STM32duino RTC"
 *
 * Choose a mode, then send the board to sleep for up to 10 s.
 * From the Serial Monitor (any baud rate except 1200):
 *   1  Sleep       CPU halted, everything else running. USB stays up
 *                  (and wakes the CPU every 1 ms, so the saving is small).
 *   2  Deep sleep  Stop mode, clocks off, RAM kept. Wakes up where it left off.
 *   3  Shutdown    Lowest power, RAM lost. Waking up restarts the sketch.
 *   s  go to sleep in the selected mode
 * Or with the buttons: User1 (SW2) picks the mode, User2 (SW1) starts it.
 * The LEDs show the selected mode (LED 1, 2 or 3).
 *
 * The board wakes up after 10 s (RTC alarm) or earlier on a button press:
 * both buttons are PWR wake-up lines (PB5 = WKUP6, PB2 = WKUP1).
 *
 * USB is switched off for deep sleep and shutdown (the bus cannot run in
 * those modes), so the serial port disappears and comes back afterwards.
 * Measure the board current in each mode to see the difference.
 */
#include <U385Board.h>
#include <STM32LowPower.h>
#include <STM32RTC.h>

enum Mode : uint8_t { MODE_SLEEP, MODE_DEEPSLEEP, MODE_SHUTDOWN };
const char *const modeNames[] = { "Sleep", "Deep sleep (Stop)", "Shutdown" };
const uint32_t SLEEP_MS = 10000;
uint8_t mode = MODE_SLEEP;
volatile bool wokeByButton = false;

STM32RTC &rtc = STM32RTC::getInstance();

void onButtonWake()
{
  wokeByButton = true;
}

void showMode()
{
  Board.ledsShow(1U << mode);
  Serial.print(F("Selected: "));
  Serial.println(modeNames[mode]);
}

void waitForSerial()
{
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}
}

void goToSleep()
{
  // A held button would wake the chip straight away.
  while (Board.readRaw(USER1) || Board.readRaw(USER2)) {}
  delay(50);

  Serial.print(F("Entering "));
  Serial.print(modeNames[mode]);
  Serial.println(F(" for up to 10 s..."));
  Serial.flush();
  delay(20);
  Board.ledsOff();
  wokeByButton = false;
  uint32_t t0 = rtc.getEpoch();

  switch (mode) {
    case MODE_SLEEP:
      // Any interrupt ends Sleep mode, and USB raises one every millisecond
      // (start of frame). So sleep again until the 10 s are over or a
      // button woke the board. The CPU still spends most of the time halted.
      while (!wokeByButton && rtc.getEpoch() - t0 < SLEEP_MS / 1000) {
        LowPower.sleep();
      }
      break;
    case MODE_DEEPSLEEP:
      Serial.end();                     // detach USB before the bus clock stops
      LowPower.deepSleep(SLEEP_MS);
      break;
    case MODE_SHUTDOWN:
      Serial.end();
      LowPower.shutdown(SLEEP_MS);      // never returns: wake-up restarts the sketch
      break;
  }

  // Back from sleep or deep sleep
  uint32_t slept = rtc.getEpoch() - t0;
  Board.ledsAll(true);
  if (mode == MODE_DEEPSLEEP) {
    // Re-attach USB; the PC sees the port again. Serial.end() leaves the
    // core's "terminal open" (DTR) flag set, so 'while (!Serial)' would
    // return at once: give the PC time to re-enumerate and reopen the port.
    Serial.begin(115200);
    delay(2500);
    waitForSerial();
  }
  delay(200);
  Serial.print(F("Woke up after about "));
  Serial.print(slept);
  Serial.print(F(" s, by "));
  Serial.println(wokeByButton ? F("a button") : F("the RTC alarm"));
  Board.update();
  Board.pressed(USER1);
  Board.pressed(USER2);
  showMode();
}

void setup()
{
  Board.begin();
  Serial.begin(115200);
  waitForSerial();

  // Accurate RTC from the 32.768 kHz crystal; it times the wake-up alarm.
  rtc.setClockSource(STM32RTC::LSE_CLOCK);
  rtc.begin();
  LowPower.begin();

  // Wake on a button press in every mode, including shutdown.
  // The library takes the level at start-up as "released", so wake on the
  // opposite edge.
  PinStatus edge = (Board.releasedLevel(USER1) == LOW) ? RISING : FALLING;
  LowPower.attachInterruptWakeup(BUTTON_USER1, onButtonWake, edge, SHUTDOWN_MODE);
  edge = (Board.releasedLevel(USER2) == LOW) ? RISING : FALLING;
  LowPower.attachInterruptWakeup(BUTTON_USER2, onButtonWake, edge, SHUTDOWN_MODE);

  Serial.println(F("\n=== Low-power modes ==="));
  Serial.print(F("Reset cause: "));
  Serial.println(Board.resetCause());
  Serial.println(F("1/2/3 = choose mode, s = sleep (or User1 = mode, User2 = sleep)"));
  showMode();
}

void loop()
{
  Board.update();

  if (Board.pressed(USER1)) {
    mode = (mode + 1) % 3;
    showMode();
  }
  bool start = Board.pressed(USER2);

  while (Serial.available()) {
    char c = Serial.read();
    if (c >= '1' && c <= '3') {
      mode = c - '1';
      showMode();
    } else if (c == 's') {
      start = true;
    }
  }

  if (start) {
    goToSleep();
  }
}
