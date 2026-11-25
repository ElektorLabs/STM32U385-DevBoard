/*
 * Arduino variant for the STM32U385RGT6Q board, schematic 260660-002 Rev 1.0
 * Pin tables, clock tree and the reboot-to-DFU used for button-free uploads.
 */
#if defined(ARDUINO_U385_BOARD)
#include "pins_arduino.h"

// Digital PinName array, index = Arduino pin number
const PinName digitalPin[] = {
  PA_10, // D0   RX
  PA_9,  // D1   TX
  PC_9,  // D2
  PB_13, // D3
  PB_14, // D4
  PB_15, // D5
  PB_10, // D6
  PC_6,  // D7
  PC_7,  // D8
  PA_8,  // D9
  PA_15, // D10  SS
  PA_7,  // D11  MOSI
  PA_6,  // D12  MISO
  PA_5,  // D13  SCK
  PB_7,  // D14  SDA
  PB_6,  // D15  SCL
  PC_10, // D16  LED1_A
  PC_11, // D17  LED1_B
  PC_12, // D18  LED2_A
  PB_4,  // D19  LED2_B
  PB_5,  // D20  BUTTON_USER1 (SW2)
  PB_2,  // D21  BUTTON_USER2 (SW1)
  PA_4,  // D22  GROVE_I2C_POWER
  PC_3,  // D23  MIKROBUS_RST
  PB_0,  // D24  MIKROBUS_CS
  PC_8,  // D25  SD_CS
  PD_2,  // D26  SD_DETECT
  PH_3,  // D27  BOOT0
  PA_13, // D28  SWDIO
  PA_14, // D29  SWCLK
  PB_3,  // D30  SWO
  PH_0,  // D31  HSE
  PH_1,  // D32  HSE
  PC_14, // D33  LSE
  PC_15, // D34  LSE
  PA_11, // D35  USB D-
  PA_12, // D36  USB D+
  PB_8,  // D37
  PC_0,  // D38
  PC_2,  // D39
  PC_13, // D40
  PA_0,  // D41  A0
  PA_1,  // D42  A1
  PA_2,  // D43  A2
  PC_1,  // D44  A3
  PA_3,  // D45  A4
  PB_1   // D46  A5
};

// Analog (Ax) pin number array
const pin_size_t analogInputPin[] = {
  41, // A0, PA0
  42, // A1, PA1
  43, // A2, PA2
  44, // A3, PC1
  45, // A4, PA3
  46  // A5, PB1
};

// ----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  System Clock Configuration
  *         SYSCLK = MSIS from MSIRC0 = 96 MHz, voltage range 1, EPOD booster on.
  *         USB    = HSI48, trimmed by CRS against USB SOF (no crystal needed).
  *         The 16 MHz HSE and 32.768 kHz LSE crystals are left off here; the
  *         U385Board library and STM32RTC turn them on when a sketch needs them.
  *         The LDO is used: it works whether or not the SMPS parts are fitted.
  */
WEAK void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {};

  if (HAL_RCCEx_EpodBoosterClkConfig(RCC_EPODBOOSTER_SOURCE_MSIS, RCC_EPODBOOSTER_DIV1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_PWREx_EnableEpodBooster() != HAL_OK) {
    Error_Handler();
  }
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }
  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSI
                                     | RCC_OSCILLATORTYPE_MSIS;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.MSISState = RCC_MSI_ON;
  RCC_OscInitStruct.MSISSource = RCC_MSI_RC0;
  RCC_OscInitStruct.MSISDiv = RCC_MSI_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSIS;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1 | RCC_PERIPHCLK_USB1;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_HSI;
  PeriphClkInit.IclkClockSelection = RCC_ICLKCLKSOURCE_HSI48;
  PeriphClkInit.Usb1ClockSelection = RCC_USB1CLKSOURCE_ICLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
}

/*----------------------------------------------------------------------------
 *  Reboot into the ROM USB-DFU bootloader (button-free uploads)
 *
 *  Uploading from the Arduino IDE opens the USB serial port at 1200 baud and
 *  closes it again. The SysTick hook below notices that and calls
 *  u385_enter_bootloader().
 *
 *  The STM32U385 ROM bootloader does not accept a software jump: it checks
 *  how the chip booted and hands control back to the user program. So instead
 *  the boot option bits are set to "boot from system memory" (nSWBOOT0 = 0,
 *  nBOOT0 = 0) and the option bytes are reloaded, which resets the chip into
 *  a genuine ROM boot, exactly as if BOOT were held at reset.
 *
 *  When dfu-util has written the new sketch and the bootloader starts it, the
 *  sketch finds those option bits still set and puts them back to "boot from
 *  the BOOT0 pin" (nSWBOOT0 = 1, nBOOT0 = 1) before doing anything else. That
 *  reload resets the chip once more, and the sketch then starts normally.
 *  Only those two bits are written; read protection and the rest of the
 *  option bytes are left as they are.
 *----------------------------------------------------------------------------*/
static int u385_option_boot_is_sysmem(void)
{
  return (FLASH->OPTR & (FLASH_OPTR_nSWBOOT0 | FLASH_OPTR_nBOOT0)) == 0U;
}

/* Write nSWBOOT0 and nBOOT0, then reload the option bytes (this resets the chip). */
static void u385_set_boot_option(uint32_t swboot0, uint32_t nboot0)
{
  FLASH_OBProgramInitTypeDef ob = {};
  ob.OptionType = OPTIONBYTE_USER;
  ob.USERType = OB_USER_NSWBOOT0 | OB_USER_NBOOT0;
  ob.USERConfig = swboot0 | nboot0;

  __disable_irq();
  HAL_FLASH_Unlock();
  HAL_FLASH_OB_Unlock();

  /* The HAL refuses to program while an old error flag is still set, so
   * clear them all first, and retry once if the write is rejected. */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

  for (int attempt = 0; attempt < 2; attempt++) {
    if (HAL_FLASHEx_OBProgram(&ob) == HAL_OK) {
      HAL_FLASH_OB_Launch();                /* reloads option bytes and resets */
    }
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
  }
  /* Only reached if programming failed: lock again and carry on. */
  HAL_FLASH_OB_Lock();
  HAL_FLASH_Lock();
  __enable_irq();
}

void u385_enter_bootloader(void)
{
  u385_set_boot_option(OB_BOOT0_FROM_OB, OB_NBOOT0_RESET);
  NVIC_SystemReset();                       /* fallback if the option write failed */
}

/* Runs before premain() (priority 101), so no clocks or peripherals are set up yet. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
__attribute__((constructor(100))) static void u385_restore_boot_option(void)
{
  if (u385_option_boot_is_sysmem()) {
    u385_set_boot_option(OB_BOOT0_FROM_PIN, OB_NBOOT0_SET);
  }
}
#pragma GCC diagnostic pop

#if defined(USBCON) && defined(USBD_USE_CDC)
#include "usbd_cdc.h"
extern USBD_CDC_LineCodingTypeDef linecoding;
extern __IO bool dtrState;

/* Called every 1 ms from SysTick_Handler -> HAL_SYSTICK_IRQHandler() */
void HAL_SYSTICK_Callback(void)
{
  static uint8_t touch_ms = 0;
  if (linecoding.bitrate == 1200 && !dtrState) {
    if (++touch_ms >= 50) {                 /* let the USB control transfer finish */
      u385_enter_bootloader();
    }
  } else {
    touch_ms = 0;
  }
}
#endif /* USBCON && USBD_USE_CDC */

#ifdef __cplusplus
}
#endif
#endif /* ARDUINO_U385_BOARD */
