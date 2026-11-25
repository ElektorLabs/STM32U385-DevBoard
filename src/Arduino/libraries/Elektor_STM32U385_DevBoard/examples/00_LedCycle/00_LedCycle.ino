/*
 * 00_LedCycle
 *
 * Runs the four LEDs through a set of patterns, over and over (about 20 s
 * per round). No buttons needed.
 *
 *   1. Chase:          one LED at a time, PC10 -> PC11 -> PB4 -> PC12
 *   2. Bounce:         one LED runs back and forth
 *   3. Fill and empty: LEDs switch on one by one, then off again
 *   4. Alternate:      LEDs 0+2 against 1+3
 *   5. LED1 vs LED2:   both channels of LED1, then both of LED2
 *   6. Binary count:   0 to 15 on the four LEDs
 *   7. Flash:          all four together
 *
 * Board.ledsShow(bits) sets all four LEDs at once: bit 0 = LED index 0
 * (PC10), bit 1 = PC11, bit 2 = PB4, bit 3 = PC12.
 *
 * Board: Tools > Board > Elektor STM32U385 Dev Board (260660)
 */
#include <U385Board.h>

// Show an LED pattern, then wait
void show(uint8_t bits, uint32_t ms)
{
  Board.ledsShow(bits);
  delay(ms);
}

void setup()
{
  Board.begin();          // LEDs as outputs, all off
}

void loop()
{
  // 1. Chase, three rounds
  for (int r = 0; r < 3; r++)
    for (int i = 0; i < 4; i++) show(1 << i, 120);

  // 2. Bounce, three rounds
  for (int r = 0; r < 3; r++) {
    for (int i = 0; i < 4; i++) show(1 << i, 90);
    for (int i = 2; i > 0; i--) show(1 << i, 90);
  }

  // 3. Fill up, then empty, twice
  for (int r = 0; r < 2; r++) {
    for (int i = 1; i <= 4; i++) show((1 << i) - 1, 150);
    for (int i = 3; i >= 0; i--) show((1 << i) - 1, 150);
  }

  // 4. Alternate: LEDs 0+2 against 1+3
  for (int r = 0; r < 6; r++) { show(0b0101, 200); show(0b1010, 200); }

  // 5. LED1 (PC10, PC11) against LED2 (PB4, PC12)
  for (int r = 0; r < 6; r++) { show(0b0011, 200); show(0b1100, 200); }

  // 6. Binary count 0 to 15
  for (int n = 0; n < 16; n++) show(n, 180);

  // 7. All four flash together
  for (int r = 0; r < 5; r++) { show(0b1111, 150); show(0, 150); }

  show(0, 400);           // short pause before the next round
}
