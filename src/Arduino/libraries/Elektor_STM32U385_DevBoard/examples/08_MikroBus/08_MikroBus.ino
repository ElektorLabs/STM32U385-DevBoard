/*
 * 08_MikroBus
 *
 * Talks to a MikroElektronika Click board in the mikroBUS socket (J5 + J6).
 * Every mikroBUS signal has a board name, so the sketch needs no pin numbers:
 *
 *   J5 (left)                      J6 (right)
 *   1 AN    PA0  MIKROBUS_AN (A0)  1 PWM  PB10  MIKROBUS_PWM (D6)
 *   2 RST   PC3  MIKROBUS_RST      2 INT  PC9   MIKROBUS_INT (D2)
 *   3 CS    PB0  MIKROBUS_CS       3 RX   PA10  Serial1 RX (D0)
 *   4 SCK   PA5  SPI               4 TX   PA9   Serial1 TX (D1)
 *   5 MISO  PA6  SPI               5 SCL  PB6   Wire
 *   6 MOSI  PA7  SPI               6 SDA  PB7   Wire
 *   7 +3V3                         7 +5V
 *   8 GND                          8 GND
 *
 * SPI is shared with the microSD slot and D11-D13, I2C with Grove/Qwiic, and
 * the UART with D0/D1 and the K6 header. RST has a 10 k pull-up (R24), so a
 * click runs even if the sketch never touches RST.
 *
 * Serial Monitor commands:
 *   s  status: AN voltage, INT and RST level, INT edges seen
 *   r  reset the click (RST low for 10 ms)
 *   i  scan I2C for the click's address
 *   j  SPI: send 0x9F (JEDEC "read ID") and print 3 bytes
 *      (gives the chip ID on Flash, FRAM and many other SPI clicks)
 *   p  PWM pin: step the duty cycle 0 / 25 / 50 / 75 / 100 %
 *   b  UART speed: 9600 -> 19200 -> 57600 -> 115200
 *   u  UART bridge: everything typed goes to the click and back
 *      (for GSM, GNSS, Wi-Fi, LoRa, BLE clicks with AT commands).
 *      Press Ctrl+] (0x1D) to leave. In the Arduino IDE Serial Monitor use
 *      "Both NL & CR" for AT commands.
 *
 * LEDs: LED index 0 blinks on UART traffic, 1 shows the INT level, 2 is on
 * while the click is held in reset, 3 lights in bridge mode.
 */
#include <U385Board.h>
#include <SPI.h>
#include <Wire.h>

const uint32_t UART_BAUDS[] = { 9600, 19200, 57600, 115200 };
uint8_t baudIndex = 3;                    // 115200
uint8_t pwmStep = 0;                      // 0..4 = 0..100 %
volatile uint32_t intEdges = 0;

void onInt()
{
  intEdges++;
}

void help()
{
  Serial.println(F("Commands: s status, r reset, i I2C scan, j SPI ID, p PWM, b baud, u UART bridge"));
}

void status()
{
  uint32_t raw = analogRead(MIKROBUS_AN);
  float volts = raw * Board.vdda() / 4095.0f;
  Serial.print(F("AN = "));
  Serial.print(volts, 3);
  Serial.print(F(" V (raw "));
  Serial.print(raw);
  Serial.print(F(") | INT = "));
  Serial.print(digitalRead(MIKROBUS_INT) ? F("HIGH") : F("LOW"));
  Serial.print(F(", "));
  Serial.print(intEdges);
  Serial.print(F(" edges | RST = "));
  Serial.print(digitalRead(MIKROBUS_RST) ? F("HIGH (running)") : F("LOW (reset)"));
  Serial.print(F(" | UART "));
  Serial.print(UART_BAUDS[baudIndex]);
  Serial.println(F(" baud"));
}

void resetClick()
{
  Board.led(2, true);
  digitalWrite(MIKROBUS_RST, LOW);
  delay(10);
  digitalWrite(MIKROBUS_RST, HIGH);
  Board.led(2, false);
  Serial.println(F("Click reset (RST pulsed low for 10 ms)."));
}

void scanI2C()
{
  uint8_t found = 0;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  I2C device at 0x"));
      if (addr < 0x10) Serial.print('0');
      Serial.println(addr, HEX);
      found++;
    }
  }
  Serial.print(found);
  Serial.println(F(" I2C device(s) found (Grove/Qwiic devices show up here too)"));
}

void spiId()
{
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(MIKROBUS_CS, LOW);
  SPI.transfer(0x9F);
  uint8_t id[3];
  for (uint8_t i = 0; i < 3; i++) {
    id[i] = SPI.transfer(0x00);
  }
  digitalWrite(MIKROBUS_CS, HIGH);
  SPI.endTransaction();

  Serial.print(F("SPI 0x9F ->"));
  for (uint8_t i = 0; i < 3; i++) {
    Serial.print(F(" 0x"));
    if (id[i] < 0x10) Serial.print('0');
    Serial.print(id[i], HEX);
  }
  bool allFF = (id[0] & id[1] & id[2]) == 0xFF;
  bool all00 = (id[0] | id[1] | id[2]) == 0x00;
  if (allFF || all00) {
    Serial.println(F("  (no answer: nothing on CS, or the chip has no read-ID command)"));
  } else {
    Serial.println();
  }
}

void stepPwm()
{
  pwmStep = (pwmStep + 1) % 5;
  analogWrite(MIKROBUS_PWM, pwmStep * 255 / 4);
  Serial.print(F("PWM duty "));
  Serial.print(pwmStep * 25);
  Serial.println(F(" %"));
}

void nextBaud()
{
  baudIndex = (baudIndex + 1) % 4;
  Serial1.end();
  Serial1.begin(UART_BAUDS[baudIndex]);
  Serial.print(F("UART set to "));
  Serial.print(UART_BAUDS[baudIndex]);
  Serial.println(F(" baud"));
}

void bridge()
{
  Serial.print(F("UART bridge at "));
  Serial.print(UART_BAUDS[baudIndex]);
  Serial.println(F(" baud. Ctrl+] to leave."));
  Board.led(3, true);
  uint32_t lastTraffic = 0;
  for (;;) {
    while (Serial.available()) {
      int c = Serial.read();
      if (c == 0x1D) {                   // Ctrl+]
        Board.ledsOff();
        Serial.println(F("\nBridge closed."));
        return;
      }
      Serial1.write((uint8_t)c);
      lastTraffic = millis();
    }
    while (Serial1.available()) {
      Serial.write((uint8_t)Serial1.read());
      lastTraffic = millis();
    }
    Board.led(0, millis() - lastTraffic < 30);
    Board.led(1, digitalRead(MIKROBUS_INT));
  }
}

void setup()
{
  Board.begin();
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}

  pinMode(MIKROBUS_RST, OUTPUT);
  digitalWrite(MIKROBUS_RST, HIGH);       // click running
  pinMode(MIKROBUS_CS, OUTPUT);
  digitalWrite(MIKROBUS_CS, HIGH);        // SPI chip not selected
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);              // keep the microSD card off the shared SPI bus
  pinMode(MIKROBUS_INT, INPUT);
  attachInterrupt(digitalPinToInterrupt(MIKROBUS_INT), onInt, CHANGE);
  analogReadResolution(12);

  SPI.begin();
  Wire.begin();
  Serial1.begin(UART_BAUDS[baudIndex]);

  Serial.println(F("\n=== mikroBUS socket ==="));
  help();
  status();
}

void loop()
{
  Board.led(1, digitalRead(MIKROBUS_INT));

  while (Serial.available()) {
    switch (Serial.read()) {
      case 's': status(); break;
      case 'r': resetClick(); break;
      case 'i': scanI2C(); break;
      case 'j': spiId(); break;
      case 'p': stepPwm(); break;
      case 'b': nextBaud(); break;
      case 'u': bridge(); break;
      case 'h': case '?': help(); break;
      default: break;
    }
  }

  // Anything the click sends while not bridging is shown too.
  while (Serial1.available()) {
    Serial.write((uint8_t)Serial1.read());
  }
}
