/*
 * U385Board - helpers for the STM32U385 board (schematic 260660-002 Rev 1.0)
 *
 *   #include <U385Board.h>
 *
 *   void setup() { Board.begin(); }
 *   void loop()  {
 *     Board.update();                          // reads the buttons
 *     if (Board.pressed(USER1)) Board.ledToggle(0);
 *   }
 *
 * The pin names themselves (LED1_A, BUTTON_USER1, SD_DETECT, ...) come from
 * the board package and work without this library.
 */
#pragma once

#include <Arduino.h>

#if !defined(ARDUINO_U385_BOARD)
  #error "U385Board needs the board 'Elektor STM32U385 Dev Board (260660)' selected in Tools > Board."
#endif

/* The four LEDs, in the order they sit on the board: LED1 (PC10, PC11), LED2 (PB4, PC12). */
static const uint8_t U385_LED_COUNT = 4;

enum U385Button : uint8_t {
  USER1 = 0,     // SW2, PB5
  USER2 = 1      // SW1, PB2
};

/* System clock speeds the MSIS oscillator can give without a PLL. */
enum U385SysClock : uint8_t {
  SYSCLK_96MHZ,  // MSIRC0 / 1, voltage range 1, booster on (default at boot)
  SYSCLK_48MHZ,  // MSIRC0 / 2, voltage range 1, booster on
  SYSCLK_24MHZ,  // MSIRC1 / 1, voltage range 2
  SYSCLK_12MHZ,  // MSIRC1 / 2, voltage range 2
  SYSCLK_6MHZ,   // MSIRC1 / 4, voltage range 2 (too slow for USB)
  SYSCLK_3MHZ    // MSIRC1 / 8, voltage range 2 (too slow for USB)
};

/* Sources that can be sent to the MCO pin (D9 / PA8). */
enum U385ClockOut : uint8_t {
  MCO_OFF,
  MCO_SYSCLK,
  MCO_MSIS,
  MCO_HSI16,
  MCO_HSE,
  MCO_HSI48,
  MCO_LSI,
  MCO_LSE
};

class U385BoardClass {
public:
  /* LEDs as outputs (off), buttons as inputs. Call once in setup(). */
  void begin();

  // ---- LEDs -----------------------------------------------------------------
  /* LED index 0..3 = PC10, PC11, PB4, PC12 (LED1_A, LED1_B, LED2_B, LED2_A). */
  void led(uint8_t index, bool on);
  void ledToggle(uint8_t index);
  bool ledState(uint8_t index) const;
  void ledsOff();
  void ledsAll(bool on);
  /* Show the low 4 bits of 'value' on the LEDs (bit 0 = LED index 0). */
  void ledsShow(uint8_t value);
  uint32_t ledPin(uint8_t index) const;

  // ---- Buttons --------------------------------------------------------------
  /* Read and debounce both buttons. Call it often, normally once per loop(). */
  void update();
  /* True while the button is held down (debounced). */
  bool isDown(U385Button b) const;
  /* True once for each press, on the update() where the press was seen. */
  bool pressed(U385Button b);
  /* True once for each release. */
  bool released(U385Button b);
  /* How long the button has been held, in ms (0 if not held). */
  uint32_t heldFor(U385Button b) const;
  /* Raw, not debounced: true when the pin differs from its released level. */
  bool readRaw(U385Button b) const;
  /* Pin level the button had at begin(), taken as "released" (HIGH or LOW).
     Lets the library work whichever way the buttons are wired; don't hold a
     button while the sketch starts. */
  int releasedLevel(U385Button b) const { return _idle[b & 1]; }
  void setDebounce(uint16_t ms) { _debounceMs = ms; }

  // ---- Board peripherals ----------------------------------------------------
  /* Power the Grove I2C connector K4 through the TPS22919 load switch. */
  void groveI2CPower(bool on);
  /* True when a card is in the microSD slot. */
  bool sdCardPresent() const;
  /* Reset into the ROM USB-DFU bootloader (same as BOOT + RESET). */
  void enterBootloader();
  /* Core supply: the board has the SMPS parts fitted (L1 2.2 uH, C13/C15),
     so the step-down SMPS can replace the default LDO and roughly halve the
     run current. Returns true when the regulator switched. */
  bool useSMPS(bool on = true);
  bool smpsActive() const;

  // ---- Clocks ---------------------------------------------------------------
  /* Change SYSCLK at run time. millis()/delay() stay correct. Re-run
     Serial1.begin()/Wire.setClock() etc. afterwards so their dividers follow.
     Returns false if the HAL refused the change. */
  bool setSystemClock(U385SysClock speed);
  uint32_t systemClock() const;          // SYSCLK in Hz
  uint32_t hclk() const;                 // AHB clock in Hz
  uint32_t pclk1() const;
  uint32_t pclk2() const;
  uint32_t pclk3() const;
  const char *systemClockSource() const; // "MSIS", "HSI16", "HSE"
  /* Start the 32.768 kHz crystal X2. Needed for an accurate RTC and for
     msiPllLock(). Returns false if it did not start within ~2 s. */
  bool startLSE();
  /* Start the 16 MHz crystal X1. Returns false if it did not start. */
  bool startHSE();
  /* Lock the MSI oscillator to the LSE crystal (PLL mode). SYSCLK accuracy
     goes from about +/-1 % to crystal accuracy. Starts the LSE if needed. */
  bool msiPllLock();
  /* Output a clock on D9 (PA8), divided by 1, 2, 4, 8 or 16. */
  void clockOut(U385ClockOut source, uint8_t divider = 1);
  /* Print every clock on a Stream, e.g. Board.printClocks(Serial). */
  void printClocks(Print &out) const;

  // ---- Chip information -----------------------------------------------------
  /* 96-bit unique ID as a 24-character hex string. */
  String uniqueId() const;
  uint16_t flashSizeKB() const;
  /* Why the chip last reset ("power-on", "pin", "software", "watchdog", ...). */
  const char *resetCause() const;
  /* Die temperature in degrees C, from the internal sensor. */
  float temperatureC();
  /* Supply voltage VDDA in volts, measured against the internal reference. */
  float vdda();

private:
  struct ButtonState {
    bool stable;
    bool lastRaw;
    bool pressEvent;
    bool releaseEvent;
    uint32_t changedAt;
    uint32_t downSince;
  };
  ButtonState _btn[2] = {};
  uint16_t _debounceMs = 25;
  int _idle[2] = { !BUTTON_PRESSED, !BUTTON_PRESSED };
  uint8_t _ledBits = 0;
  bool _begun = false;
};

extern U385BoardClass Board;
