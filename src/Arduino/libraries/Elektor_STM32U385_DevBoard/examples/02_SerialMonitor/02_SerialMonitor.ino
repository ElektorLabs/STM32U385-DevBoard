/*
 * 02_SerialMonitor
 *
 * Talks over the USB serial port (no adapter needed). Open the Serial Monitor
 * at any baud rate (except 1200, which triggers the upload reset).
 *
 * Commands (type and press Enter):
 *   1..4   toggle LED 1..4         a   all LEDs on      o   all LEDs off
 *   c      print the clock tree    i   board info       b   reboot to DFU
 *   h      this help
 * Button presses and releases are reported as they happen.
 */
#include <U385Board.h>

void help()
{
  Serial.println(F("Commands: 1-4 toggle LED | a all on | o all off | c clocks | i info | b bootloader | h help"));
}

void info()
{
  Serial.print(F("Unique ID   : ")); Serial.println(Board.uniqueId());
  Serial.print(F("Flash       : ")); Serial.print(Board.flashSizeKB()); Serial.println(F(" KB"));
  Serial.print(F("Reset cause : ")); Serial.println(Board.resetCause());
  Serial.print(F("Temperature : ")); Serial.print(Board.temperatureC(), 1); Serial.println(F(" C"));
  Serial.print(F("VDDA        : ")); Serial.print(Board.vdda(), 3); Serial.println(F(" V"));
  Serial.print(F("microSD     : ")); Serial.println(Board.sdCardPresent() ? F("card inserted") : F("empty"));
}

void setup()
{
  Board.begin();
  Serial.begin(115200);

  // Wait up to 3 s for the Serial Monitor so the banner is not lost
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {
    Board.ledToggle(0);
    delay(100);
  }
  Board.ledsOff();

  Serial.println();
  Serial.println(F("=== STM32U385 Board - USB serial demo ==="));
  help();
}

void loop()
{
  Board.update();

  if (Board.pressed(USER1))  Serial.println(F("User1 (SW2) pressed"));
  if (Board.released(USER1)) Serial.println(F("User1 (SW2) released"));
  if (Board.pressed(USER2))  Serial.println(F("User2 (SW1) pressed"));
  if (Board.released(USER2)) Serial.println(F("User2 (SW1) released"));

  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case '1': case '2': case '3': case '4': {
        uint8_t i = c - '1';
        Board.ledToggle(i);
        Serial.print(F("LED ")); Serial.print(i + 1);
        Serial.println(Board.ledState(i) ? F(" on") : F(" off"));
        break;
      }
      case 'a': Board.ledsAll(true); Serial.println(F("All LEDs on")); break;
      case 'o': Board.ledsOff();     Serial.println(F("All LEDs off")); break;
      case 'c': Board.printClocks(Serial); break;
      case 'i': info(); break;
      case 'b':
        Serial.println(F("Rebooting into the DFU bootloader..."));
        Serial.flush();
        delay(100);
        Board.enterBootloader();
        break;
      case 'h': case '?': help(); break;
      case '\r': case '\n': case ' ': break;
      default:
        Serial.print(F("Unknown command '")); Serial.print(c); Serial.println('\'');
        help();
    }
  }

  // Heartbeat every 5 s
  static uint32_t last = 0;
  if (millis() - last >= 5000) {
    last = millis();
    Serial.print(F("[")); Serial.print(millis() / 1000); Serial.print(F(" s] LEDs = 0b"));
    for (int i = U385_LED_COUNT - 1; i >= 0; i--) Serial.print(Board.ledState(i) ? '1' : '0');
    Serial.print(F(", T = ")); Serial.print(Board.temperatureC(), 1); Serial.println(F(" C"));
  }
}
