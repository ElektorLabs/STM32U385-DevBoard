/*
 * Arduino variant for the STM32U385RGT6Q board, schematic 260660-002 Rev 1.0
 *
 * Pin numbering
 *   D0..D13    Arduino Uno digital header (J3, J4)
 *   D14, D15   SDA, SCL on J4 (I2C1)
 *   D16..D27   on-board parts (LEDs, buttons, Grove power, mikroBUS, microSD, BOOT0)
 *   D28..D40   system pins and spare pins
 *   D41..D46   A0..A5 on the analog header (J2)
 *
 * STM32 pin names (PA5, PC10, ...) work everywhere a pin number is expected.
 * Logic level is 3.3 V on every pin.
 */
#pragma once

/*----------------------------------------------------------------------------
 *        STM32 pins number
 *----------------------------------------------------------------------------*/
// J3: Arduino digital header, low half
#define PA10                    0    // D0  USART1 RX (shared with Grove UART K6, mikroBUS J6)
#define PA9                     1    // D1  USART1 TX (shared with Grove UART K6, mikroBUS J6)
#define PC9                     2    // D2  mikroBUS INT
#define PB13                    3    // D3
#define PB14                    4    // D4
#define PB15                    5    // D5
#define PB10                    6    // D6  mikroBUS PWM
#define PC6                     7    // D7
// J4: Arduino digital header, high half
#define PC7                     8    // D8
#define PA8                     9    // D9  also MCO (clock output)
#define PA15                    10   // D10 SPI SS
#define PA7                     11   // D11 SPI1 MOSI
#define PA6                     12   // D12 SPI1 MISO
#define PA5                     13   // D13 SPI1 SCK
#define PB7                     14   // D14 I2C1 SDA
#define PB6                     15   // D15 I2C1 SCL
// On-board parts
#define PC10                    16   // LED1 pin 1 (R13)
#define PC11                    17   // LED1 pin 2 (R12)
#define PC12                    18   // LED2 pin 1 (R10)
#define PB4                     19   // LED2 pin 2 (R11)
#define PB5                     20   // SW2 "User1", HIGH when pressed
#define PB2                     21   // SW1 "User2", HIGH when pressed
#define PA4                     22   // Grove I2C power switch enable (IC4)
#define PC3                     23   // mikroBUS RST
#define PB0                     24   // mikroBUS CS
#define PC8                     25   // microSD CS
#define PD2                     26   // microSD card detect, LOW = card inserted
#define PH3                     27   // BOOT0 / SW3
// System and spare pins
#define PA13                    28   // SWDIO
#define PA14                    29   // SWCLK
#define PB3                     30   // SWO
#define PH0                     31   // HSE 16 MHz crystal
#define PH1                     32   // HSE 16 MHz crystal
#define PC14                    33   // LSE 32.768 kHz crystal
#define PC15                    34   // LSE 32.768 kHz crystal
#define PA11                    35   // USB D-
#define PA12                    36   // USB D+
#define PB8                     37   // not connected
#define PC0                     38   // not connected
#define PC2                     39   // not connected
#define PC13                    40   // not connected
// J2: Arduino analog header
#define PA0                     PIN_A0   // D41 A0, OPAMP1 VINP, mikroBUS AN
#define PA1                     PIN_A1   // D42 A1, OPAMP1 VINM
#define PA2                     PIN_A2   // D43 A2
#define PC1                     PIN_A3   // D44 A3
#define PA3                     PIN_A4   // D45 A4, OPAMP1 VOUT
#define PB1                     PIN_A5   // D46 A5

// Alternate pins number
#define PA1_ALT1                (PA1  | ALT1)
#define PA2_ALT1                (PA2  | ALT1)
#define PA3_ALT1                (PA3  | ALT1)
#define PA4_ALT1                (PA4  | ALT1)
#define PA5_ALT1                (PA5  | ALT1)
#define PA6_ALT1                (PA6  | ALT1)
#define PA7_ALT1                (PA7  | ALT1)
#define PA7_ALT2                (PA7  | ALT2)
#define PA9_ALT1                (PA9  | ALT1)
#define PA10_ALT1               (PA10 | ALT1)
#define PA15_ALT1               (PA15 | ALT1)
#define PB0_ALT1                (PB0  | ALT1)
#define PB1_ALT1                (PB1  | ALT1)
#define PB3_ALT1                (PB3  | ALT1)
#define PB4_ALT1                (PB4  | ALT1)
#define PB5_ALT1                (PB5  | ALT1)
#define PB6_ALT1                (PB6  | ALT1)
#define PB7_ALT1                (PB7  | ALT1)
#define PB8_ALT1                (PB8  | ALT1)
#define PB10_ALT1               (PB10 | ALT1)
#define PB13_ALT1               (PB13 | ALT1)
#define PB14_ALT1               (PB14 | ALT1)
#define PB15_ALT1               (PB15 | ALT1)
#define PC1_ALT1                (PC1  | ALT1)
#define PC10_ALT1               (PC10 | ALT1)
#define PC11_ALT1               (PC11 | ALT1)
#define PH3_ALT1                (PH3  | ALT1)

#define NUM_DIGITAL_PINS        47
#define NUM_ANALOG_INPUTS       6

/*----------------------------------------------------------------------------
 *        Board parts (names follow the schematic)
 *----------------------------------------------------------------------------*/
// LEDs, active high through 1 kOhm
#define LED1_A                  PC10
#define LED1_B                  PC11
#define LED2_A                  PC12
#define LED2_B                  PB4
#ifndef LED_BUILTIN
  #define LED_BUILTIN           LED1_A
#endif

// Buttons, read HIGH when pressed (external 10 kOhm pull-down)
#define BUTTON_USER1            PB5      // SW2
#define BUTTON_USER2            PB2      // SW1
#define BUTTON_PRESSED          HIGH
#ifndef USER_BTN
  #define USER_BTN              BUTTON_USER1
#endif

// Grove I2C power switch (TPS22919), HIGH = on
#define GROVE_I2C_POWER         PA4

// mikroBUS socket (J5, J6)
#define MIKROBUS_AN             PA0
#define MIKROBUS_RST            PC3
#define MIKROBUS_CS             PB0
#define MIKROBUS_PWM            PB10
#define MIKROBUS_INT            PC9

// microSD (K7), SPI mode on SPI1
#define SD_CS                   PC8
#define SD_DETECT               PD2      // LOW = card inserted
#define SD_DETECT_LEVEL         LOW

// Clock output (MCO1 on D9)
#define MCO_PIN                 PA8

/*----------------------------------------------------------------------------
 *        Peripheral defaults
 *----------------------------------------------------------------------------*/
// I2C1: Grove K4, Qwiic K8, mikroBUS, Arduino header
#ifndef PIN_WIRE_SDA
  #define PIN_WIRE_SDA          PB7
#endif
#ifndef PIN_WIRE_SCL
  #define PIN_WIRE_SCL          PB6
#endif

// SPI1: Arduino header, mikroBUS, microSD
#ifndef PIN_SPI_SS
  #define PIN_SPI_SS            PA15
#endif
#ifndef PIN_SPI_MOSI
  #define PIN_SPI_MOSI          PA7
#endif
#ifndef PIN_SPI_MISO
  #define PIN_SPI_MISO          PA6
#endif
#ifndef PIN_SPI_SCK
  #define PIN_SPI_SCK           PA5
#endif

// Timers used by tone() and Servo, so they don't take a PWM pin
#ifndef TIMER_TONE
  #define TIMER_TONE            TIM6
#endif
#ifndef TIMER_SERVO
  #define TIMER_SERVO           TIM7
#endif

// USART1 on D0/D1 (also Grove UART K6 and mikroBUS J6).
// With USB CDC enabled (default), 'Serial' is USB and this port is 'Serial1'.
#ifndef SERIAL_UART_INSTANCE
  #define SERIAL_UART_INSTANCE  1
#endif
#ifndef PIN_SERIAL_RX
  #define PIN_SERIAL_RX         PA10
#endif
#ifndef PIN_SERIAL_TX
  #define PIN_SERIAL_TX         PA9
#endif

// Crystals fitted on the board
#define HSE_VALUE_BOARD         16000000UL
#define LSE_VALUE_BOARD         32768UL

// SDMMC signals not available
#define SDMMC_CDIR_NA

// Extra HAL modules
#if !defined(HAL_DAC_MODULE_DISABLED)
  #define HAL_DAC_MODULE_ENABLED
#endif
#if !defined(HAL_I3C_MODULE_DISABLED)
  #define HAL_I3C_MODULE_ENABLED
#endif
#if !defined(HAL_OSPI_MODULE_DISABLED)
  #define HAL_OSPI_MODULE_ENABLED
#endif
#if !defined(HAL_SD_MODULE_DISABLED)
  #define HAL_SD_MODULE_ENABLED
#endif

// Alternate SYS_WKUP definition
#define PWR_WAKEUP_PIN1_1
#define PWR_WAKEUP_PIN2_1
#define PWR_WAKEUP_PIN3_1
#define PWR_WAKEUP_PIN4_1
#define PWR_WAKEUP_PIN4_2
#define PWR_WAKEUP_PIN5_1
#define PWR_WAKEUP_PIN6_1
#define PWR_WAKEUP_PIN7_1
#define PWR_WAKEUP_PIN8_1

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/
#ifdef __cplusplus
  #ifndef SERIAL_PORT_MONITOR
    #define SERIAL_PORT_MONITOR   Serial
  #endif
  #ifndef SERIAL_PORT_HARDWARE
    #define SERIAL_PORT_HARDWARE  Serial1
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* Reboot into the STM32 ROM USB-DFU bootloader (used by the 1200-baud upload touch). */
void u385_enter_bootloader(void);
#ifdef __cplusplus
}
#endif
