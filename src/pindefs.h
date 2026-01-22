#ifndef PINDEFS_H
#define PINDEFS_H

// i2c pins
#ifndef I2C0_SDA_PIN
#define I2C0_SDA_PIN 20
#endif
#ifndef I2C0_SCL_PIN
#define I2C0_SCL_PIN 21
#endif

#ifndef I2C1_SDA_PIN
#define I2C1_SDA_PIN 18
#endif
#ifndef I2C1_SCL_PIN
#define I2C1_SCL_PIN 19
#endif

// spi pins
#ifndef SPI0_CS_PIN
#define SPI0_CS_PIN 5
#endif
#ifndef SPI0_MOSI_PIN
#define SPI0_MOSI_PIN 3
#endif
#ifndef SPI0_MISO_PIN
#define SPI0_MISO_PIN 4
#endif
#ifndef SPI0_SCK_PIN
#define SPI0_SCK_PIN 2
#endif
#ifndef SPI0_DC_PIN
#define SPI0_DC_PIN 6
#endif

#ifndef SPI1_CS_PIN
#define SPI1_CS_PIN 13
#endif
#ifndef SPI1_MOSI_PIN
#define SPI1_MOSI_PIN 11
#endif
#ifndef SPI1_MISO_PIN
#define SPI1_MISO_PIN 12
#endif
#ifndef SPI1_SCK_PIN
#define SPI1_SCK_PIN 10
#endif

// sd card pins
#ifndef SD_CARD_CS_PIN
#define SD_CARD_CS_PIN 16
#endif

// nfc pins
#ifndef NFC_CS_PIN
#define NFC_CS_PIN SPI1_CS_PIN
#endif
#ifndef NFC_IRQ_PIN
#define NFC_IRQ_PIN 22
#endif
#ifndef NFC_RESET_PIN
#define NFC_RESET_PIN 23
#endif

// e-paper display pins
#ifndef EPD_CS_PIN
#define EPD_CS_PIN SPI0_CS_PIN
#endif
#ifndef EPD_RST_PIN
#define EPD_RST_PIN 7
#endif
#ifndef EPD_DC_PIN
#define EPD_DC_PIN SPI0_DC_PIN
#endif
#ifndef EPD_BUSY_PIN
#define EPD_BUSY_PIN 8
#endif

// buzzer pin
#ifndef BUZZER_PIN
#define BUZZER_PIN 9
#endif

// battery management pins
#ifndef BAT_CHARGE_EN_PIN
#define BAT_CHARGE_EN_PIN 14
#endif
#ifndef BAT_STAT1_PIN
#define BAT_STAT1_PIN 24
#endif
#ifndef BAT_STAT2_PIN
#define BAT_STAT2_PIN 25
#endif
#ifndef BAT_VOLTAGE_PIN
#define BAT_VOLTAGE_PIN 26 // ADC0
#endif

// button pins
#ifndef BTN_DPAD_UP_PIN
#define BTN_DPAD_UP_PIN 15
#endif
#ifndef BTN_DPAD_DOWN_PIN
#define BTN_DPAD_DOWN_PIN 29
#endif
#ifndef BTN_DPAD_LEFT_PIN
#define BTN_DPAD_LEFT_PIN 28
#endif
#ifndef BTN_DPAD_RIGHT_PIN
#define BTN_DPAD_RIGHT_PIN 27
#endif
#ifndef BTN_DPAD_CENTER_PIN
#define BTN_DPAD_CENTER_PIN 17
#endif

#ifndef BTN_A_PIN
#define BTN_A_PIN 1
#endif
#ifndef BTN_B_PIN
#define BTN_B_PIN 0
#endif

#endif