#include "U385Board.h"

U385BoardClass Board;

static const uint32_t kLedPins[U385_LED_COUNT] = { LED1_A, LED1_B, LED2_B, LED2_A };
static const uint32_t kButtonPins[2] = { BUTTON_USER1, BUTTON_USER2 };

// ============================================================================
//  Setup
// ============================================================================
void U385BoardClass::begin()
{
  for (uint8_t i = 0; i < U385_LED_COUNT; i++) {
    pinMode(kLedPins[i], OUTPUT);
    digitalWrite(kLedPins[i], LOW);
  }
  _ledBits = 0;
  for (uint8_t i = 0; i < 2; i++) {
    pinMode(kButtonPins[i], INPUT);
    delay(2);
    _idle[i] = digitalRead(kButtonPins[i]);   // level now = released
    _btn[i] = { false, false, false, false, millis(), 0 };
  }
  _begun = true;
}

// ============================================================================
//  LEDs
// ============================================================================
uint32_t U385BoardClass::ledPin(uint8_t index) const
{
  return kLedPins[index % U385_LED_COUNT];
}

void U385BoardClass::led(uint8_t index, bool on)
{
  if (index >= U385_LED_COUNT) {
    return;
  }
  digitalWrite(kLedPins[index], on ? HIGH : LOW);
  if (on) {
    _ledBits |= (1U << index);
  } else {
    _ledBits &= ~(1U << index);
  }
}

void U385BoardClass::ledToggle(uint8_t index)
{
  led(index, !ledState(index));
}

bool U385BoardClass::ledState(uint8_t index) const
{
  return index < U385_LED_COUNT && (_ledBits & (1U << index));
}

void U385BoardClass::ledsOff()
{
  ledsShow(0);
}

void U385BoardClass::ledsAll(bool on)
{
  ledsShow(on ? 0x0F : 0x00);
}

void U385BoardClass::ledsShow(uint8_t value)
{
  for (uint8_t i = 0; i < U385_LED_COUNT; i++) {
    led(i, value & (1U << i));
  }
}

// ============================================================================
//  Buttons
// ============================================================================
bool U385BoardClass::readRaw(U385Button b) const
{
  return digitalRead(kButtonPins[b & 1]) != _idle[b & 1];
}

void U385BoardClass::update()
{
  if (!_begun) {
    begin();
  }
  uint32_t now = millis();
  for (uint8_t i = 0; i < 2; i++) {
    ButtonState &s = _btn[i];
    bool raw = readRaw((U385Button)i);
    if (raw != s.lastRaw) {
      s.lastRaw = raw;
      s.changedAt = now;
    }
    if (raw != s.stable && (now - s.changedAt) >= _debounceMs) {
      s.stable = raw;
      if (raw) {
        s.pressEvent = true;
        s.downSince = now;
      } else {
        s.releaseEvent = true;
        s.downSince = 0;
      }
    }
  }
}

bool U385BoardClass::isDown(U385Button b) const
{
  return _btn[b & 1].stable;
}

bool U385BoardClass::pressed(U385Button b)
{
  bool e = _btn[b & 1].pressEvent;
  _btn[b & 1].pressEvent = false;
  return e;
}

bool U385BoardClass::released(U385Button b)
{
  bool e = _btn[b & 1].releaseEvent;
  _btn[b & 1].releaseEvent = false;
  return e;
}

uint32_t U385BoardClass::heldFor(U385Button b) const
{
  const ButtonState &s = _btn[b & 1];
  return s.stable ? (millis() - s.downSince) : 0;
}

// ============================================================================
//  Board peripherals
// ============================================================================
void U385BoardClass::groveI2CPower(bool on)
{
  pinMode(GROVE_I2C_POWER, OUTPUT);
  digitalWrite(GROVE_I2C_POWER, on ? HIGH : LOW);
}

bool U385BoardClass::sdCardPresent() const
{
  pinMode(SD_DETECT, INPUT);                  // R20 pull-up on the board
  return digitalRead(SD_DETECT) == SD_DETECT_LEVEL;
}

void U385BoardClass::enterBootloader()
{
  u385_enter_bootloader();
}

// ============================================================================
//  Clocks
// ============================================================================
namespace {
struct ClockProfile {
  uint32_t hz;
  uint32_t source;     // RCC_MSI_RC0 (96 MHz) or RCC_MSI_RC1 (24 MHz)
  uint32_t div;
  bool range1;         // voltage range 1 (needed above 24 MHz here)
  uint32_t latency;
};

const ClockProfile kProfiles[] = {
  { 96000000UL, RCC_MSI_RC0, RCC_MSI_DIV1, true,  FLASH_LATENCY_2 },
  { 48000000UL, RCC_MSI_RC0, RCC_MSI_DIV2, true,  FLASH_LATENCY_1 },
  { 24000000UL, RCC_MSI_RC1, RCC_MSI_DIV1, false, FLASH_LATENCY_1 },
  { 12000000UL, RCC_MSI_RC1, RCC_MSI_DIV2, false, FLASH_LATENCY_0 },
  {  6000000UL, RCC_MSI_RC1, RCC_MSI_DIV4, false, FLASH_LATENCY_0 },
  {  3000000UL, RCC_MSI_RC1, RCC_MSI_DIV8, false, FLASH_LATENCY_0 },
};
}

bool U385BoardClass::setSystemClock(U385SysClock speed)
{
  if (speed > SYSCLK_3MHZ) {
    return false;
  }
  const ClockProfile &p = kProfiles[speed];

  // Make sure the core runs from MSIS with all bus prescalers at 1.
  if (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_MSIS) {
    return false;
  }

  bool faster = p.hz > HAL_RCC_GetSysClockFreq();

  if (faster) {
    // Going up: voltage, booster and wait states first, then the clock.
    if (p.range1) {
      if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        return false;
      }
    }
    if (p.hz > 24000000UL) {
      if (HAL_RCCEx_EpodBoosterClkConfig(RCC_EPODBOOSTER_SOURCE_MSIS, RCC_EPODBOOSTER_DIV1) != HAL_OK) {
        return false;
      }
      if (HAL_PWREx_EnableEpodBooster() != HAL_OK) {
        return false;
      }
    }
    if (p.latency > __HAL_FLASH_GET_LATENCY()) {
      __HAL_FLASH_SET_LATENCY(p.latency);
      while (__HAL_FLASH_GET_LATENCY() != p.latency) {}
    }
  }

  RCC_OscInitTypeDef osc = {};
  osc.OscillatorType = RCC_OSCILLATORTYPE_MSIS;
  osc.MSISState = RCC_MSI_ON;
  osc.MSISSource = p.source;
  osc.MSISDiv = p.div;
  // Switches the MSIS source/divider, waits for MSISRDY, updates SystemCoreClock and SysTick.
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
    return false;
  }

  if (!faster) {
    // Going down: clock first, then fewer wait states, booster off, range 2.
    if (p.latency < __HAL_FLASH_GET_LATENCY()) {
      __HAL_FLASH_SET_LATENCY(p.latency);
      while (__HAL_FLASH_GET_LATENCY() != p.latency) {}
    }
    if (p.hz <= 24000000UL) {
      HAL_PWREx_DisableEpodBooster();
      HAL_RCCEx_EpodBoosterClkConfig(RCC_EPODBOOSTER_SOURCE_NONE, RCC_EPODBOOSTER_DIV1);
    }
    if (!p.range1) {
      HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
    }
  }

  SystemCoreClockUpdate();
  HAL_InitTick(uwTickPrio);
  return true;
}

uint32_t U385BoardClass::systemClock() const
{
  return HAL_RCC_GetSysClockFreq();
}

uint32_t U385BoardClass::hclk() const
{
  return HAL_RCC_GetHCLKFreq();
}

uint32_t U385BoardClass::pclk1() const
{
  return HAL_RCC_GetPCLK1Freq();
}

uint32_t U385BoardClass::pclk2() const
{
  return HAL_RCC_GetPCLK2Freq();
}

uint32_t U385BoardClass::pclk3() const
{
  return HAL_RCC_GetPCLK3Freq();
}

const char *U385BoardClass::systemClockSource() const
{
  switch (__HAL_RCC_GET_SYSCLK_SOURCE()) {
    case RCC_SYSCLKSOURCE_STATUS_MSIS:
      return "MSIS";
    case RCC_SYSCLKSOURCE_STATUS_HSI:
      return "HSI16";
    case RCC_SYSCLKSOURCE_STATUS_HSE:
      return "HSE";
    default:
      return "?";
  }
}

bool U385BoardClass::useSMPS(bool on)
{
  return HAL_PWREx_ConfigSupply(on ? PWR_SMPS_SUPPLY : PWR_LDO_SUPPLY) == HAL_OK;
}

bool U385BoardClass::smpsActive() const
{
  return HAL_PWREx_GetSupplyConfig() == PWR_SMPS_SUPPLY;
}

bool U385BoardClass::startLSE()
{
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY)) {
    return true;
  }
  HAL_PWR_EnableBkUpAccess();
  RCC_OscInitTypeDef osc = {};
  osc.OscillatorType = RCC_OSCILLATORTYPE_LSE;
  osc.LSEState = RCC_LSE_ON;
  return HAL_RCC_OscConfig(&osc) == HAL_OK;   // HAL waits up to LSE_TIMEOUT_VALUE
}

bool U385BoardClass::startHSE()
{
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY)) {
    return true;
  }
  RCC_OscInitTypeDef osc = {};
  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState = RCC_HSE_ON;
  return HAL_RCC_OscConfig(&osc) == HAL_OK;
}

bool U385BoardClass::msiPllLock()
{
  if (!startLSE()) {
    return false;
  }
  RCC_MSIRCxPLLTypeDef pll = {};
  pll.State = RCC_MSIRCx_PLL_ON;
  pll.InputSrce = RCC_MSIRCx_PLL_INPUT_LSE;
  pll.FastMode = RCC_MSIRCx_PLL_FAST_DISABLE;
  pll.MSIRC1PLLN = RCC_MSIRC1_PLLN_732;      // 32.768 kHz x 732 = 23.986 MHz
  // Lock whichever MSIRC is feeding the system clock.
  uint32_t src = READ_BIT(RCC->ICSCR1, RCC_ICSCR1_MSISSEL) ? RCC_MSI_RC1 : RCC_MSI_RC0;
  return HAL_RCCEx_MSIRCxPLLModeConfig(src, &pll) == HAL_OK;
}

void U385BoardClass::clockOut(U385ClockOut source, uint8_t divider)
{
  static const uint32_t sources[] = {
    RCC_MCO1SOURCE_NOCLOCK, RCC_MCO1SOURCE_SYSCLK, RCC_MCO1SOURCE_MSIS, RCC_MCO1SOURCE_HSI,
    RCC_MCO1SOURCE_HSE, RCC_MCO1SOURCE_HSI48, RCC_MCO1SOURCE_LSI, RCC_MCO1SOURCE_LSE
  };
  uint32_t div;
  switch (divider) {
    case 2:  div = RCC_MCODIV_2;  break;
    case 4:  div = RCC_MCODIV_4;  break;
    case 8:  div = RCC_MCODIV_8;  break;
    case 16: div = RCC_MCODIV_16; break;
    default: div = RCC_MCODIV_1;  break;
  }
  if (source == MCO_HSE) {
    startHSE();
  } else if (source == MCO_LSE) {
    startLSE();
  } else if (source == MCO_LSI) {
    __HAL_RCC_LSI_ENABLE();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, sources[source < 8 ? source : 0], div);
}

static void printMHz(Print &out, const char *name, uint32_t hz)
{
  out.print(name);
  out.print(hz / 1000000UL);
  out.print('.');
  uint32_t frac = (hz % 1000000UL) / 1000UL;
  if (frac < 100) {
    out.print('0');
  }
  if (frac < 10) {
    out.print('0');
  }
  out.print(frac);
  out.println(" MHz");
}

void U385BoardClass::printClocks(Print &out) const
{
  out.print("SYSCLK source : ");
  out.println(systemClockSource());
  printMHz(out, "SYSCLK       : ", systemClock());
  printMHz(out, "HCLK (AHB)   : ", hclk());
  printMHz(out, "PCLK1        : ", pclk1());
  printMHz(out, "PCLK2        : ", pclk2());
  printMHz(out, "PCLK3        : ", pclk3());
  out.print("Flash latency: ");
  out.print(__HAL_FLASH_GET_LATENCY());
  out.println(" WS");
  out.print("Voltage range: ");
  out.println(HAL_PWREx_GetVoltageRange() == PWR_REGULATOR_VOLTAGE_SCALE1 ? "1 (0.9 V)" : "2 (0.75 V)");
  out.print("EPOD booster : ");
  out.println(READ_BIT(PWR->VOSR, PWR_VOSR_BOOSTRDY) ? "on" : "off");
  out.print("HSI48 (USB)  : ");
  out.println(__HAL_RCC_GET_FLAG(RCC_FLAG_HSI48RDY) ? "on" : "off");
  out.print("HSE 16 MHz   : ");
  out.println(__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) ? "on" : "off");
  out.print("LSE 32.768k  : ");
  out.println(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) ? "on" : "off");
  out.print("MSI PLL lock : ");
  out.println(READ_BIT(RCC->CR, RCC_CR_MSIPLL0RDY | RCC_CR_MSIPLL1RDY) ? "yes" : "no");
}

// ============================================================================
//  Chip information
// ============================================================================
String U385BoardClass::uniqueId() const
{
  char buf[25];
  const uint32_t *uid = (const uint32_t *)UID_BASE;
  snprintf(buf, sizeof(buf), "%08lX%08lX%08lX",
           (unsigned long)uid[2], (unsigned long)uid[1], (unsigned long)uid[0]);
  return String(buf);
}

uint16_t U385BoardClass::flashSizeKB() const
{
  return *(const uint16_t *)FLASHSIZE_BASE;
}

const char *U385BoardClass::resetCause() const
{
  uint32_t csr = RCC->CSR;
  if (csr & RCC_CSR_LPWRRSTF) {
    return "low-power";
  }
  if (csr & RCC_CSR_WWDGRSTF) {
    return "window watchdog";
  }
  if (csr & RCC_CSR_IWDGRSTF) {
    return "independent watchdog";
  }
  if (csr & RCC_CSR_SFTRSTF) {
    return "software";
  }
  if (csr & RCC_CSR_BORRSTF) {
    return "power-on / brown-out";
  }
  if (csr & RCC_CSR_OBLRSTF) {
    return "option byte load";
  }
  if (csr & RCC_CSR_PINRSTF) {
    return "reset pin";
  }
  return "unknown";
}

float U385BoardClass::vdda()
{
  analogReadResolution(12);
  uint32_t raw = analogRead(AVREF);
  return __LL_ADC_CALC_VREFANALOG_VOLTAGE(raw, LL_ADC_RESOLUTION_12B) / 1000.0f;
}

float U385BoardClass::temperatureC()
{
  analogReadResolution(12);
  uint32_t vref_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(analogRead(AVREF), LL_ADC_RESOLUTION_12B);
  uint32_t raw = analogRead(ATEMP);
  return (float)__LL_ADC_CALC_TEMPERATURE(vref_mv, raw, LL_ADC_RESOLUTION_12B);
}
