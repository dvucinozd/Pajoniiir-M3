# ESP32-P4 Pinout Inventory for JC-ESP32P4-M3-DEV & 5.0" MIPI-DSI (800x480)

Documentation status: active pin inventory for the **JC-ESP32P4-M3-DEV** board with 5-inch MIPI-DSI (800×480) display and FT5426 touch controller.

---

## 1. MIPI-DSI 15-pin FPC Connector (J2)

| Pin | Oznaka na ploči | Funkcija / Opis | P4 peripheral |
| --- | --- | --- | --- |
| 1 | GND | Masa (oklop / referenca) | GND |
| 2 | DSI_A_DATA1_N | MIPI DSI Data Lane 1 diferencijalni negativni par | MIPI DPHY DATA1- |
| 3 | DSI_A_DATA1_P | MIPI DSI Data Lane 1 diferencijalni pozitivni par | MIPI DPHY DATA1+ |
| 4 | GND | Masa | GND |
| 5 | DSI_A_CLK_N | MIPI DSI Clock diferencijalni negativni takt | MIPI DPHY CLK- |
| 6 | DSI_A_CLK_P | MIPI DSI Clock diferencijalni pozitivni takt | MIPI DPHY CLK+ |
| 7 | GND | Masa | GND |
| 8 | DSI_A_DATA0_N | MIPI DSI Data Lane 0 diferencijalni negativni par | MIPI DPHY DATA0- |
| 9 | DSI_A_DATA0_P | MIPI DSI Data Lane 0 diferencijalni pozitivni par | MIPI DPHY DATA0+ |
| 10 | GND | Masa | GND |
| 11 | ES_I2C_SCL | I2C Clock (Touch FT5426 SCL, 2.2k pull-up) | GPIO8 (I2C_NUM_1 SCL) |
| 12 | ES_I2C_SDA | I2C Data (Touch FT5426 SDA, 2.2k pull-up) | GPIO7 (I2C_NUM_1 SDA) |
| 13 | GND | Masa | GND |
| 14 | ESP_3V3 | +3.3V Glavno napajanje ekrana i kontrolera | +3.3V |
| 15 | ESP_3V3 | +3.3V Glavno napajanje ekrana i kontrolera | +3.3V |

---

## 2. Master Audio I2S DAC (PCM5102A Breakout na JP1 headeru)

| Funkcija | ESP32-P4 GPIO / JP1 Pin | Napomena |
| --- | --- | --- |
| I2S BCLK | GPIO1 (JP1 Pin 7) | I2S_NUM_1 Bit Clock prema PCM5102A BCK |
| I2S WS / LRCK | GPIO2 (JP1 Pin 9) | I2S_NUM_1 Word Select prema PCM5102A LCK |
| I2S DOUT / DIN | GPIO3 (JP1 Pin 11) | I2S_NUM_1 Data Out prema PCM5102A DIN |
| SCK | GND (JP1 Pin 5 ili 6) | Za aktivaciju internog PLL-a na PCM5102A |
| GND | GND (JP1 Pin 5, 6 ili 16) | Masa |
| VIN | +5V (JP1 Pin 2 ili 4) | Napajanje DAC pločice |

---

## 3. MicroSD Card (SDMMC Slot 0)

| Signal | ESP32-P4 GPIO | Napomena |
| --- | --- | --- |
| D0 | GPIO39 | SDMMC 4-bit Data 0 |
| D1 | GPIO40 | SDMMC 4-bit Data 1 |
| D2 | GPIO41 | SDMMC 4-bit Data 2 |
| D3 | GPIO42 | SDMMC 4-bit Data 3 |
| CLK | GPIO43 | SDMMC Clock |
| CMD | GPIO44 | SDMMC Command |
| Power | LDO VO4 | ESP32-P4 internim LDO kanalom 4 |

---

## 4. ESP32-C6 Koprocesor (SDIO Slot 1 za Wi-Fi 6 / Bluetooth 5)

| Signal | ESP32-P4 GPIO | Funkcija |
| --- | --- | --- |
| D0 | GPIO14 | SDIO Data 0 |
| D1 | GPIO15 | SDIO Data 1 |
| D2 | GPIO16 | SDIO Data 2 |
| D3 | GPIO17 | SDIO Data 3 |
| CLK | GPIO18 | SDIO Clock |
| CMD | GPIO19 | SDIO Command |
| Reset | GPIO54 | Reset linija ESP32-C6 koprocesora |

---

## 5. Komunikacija s DDJ-FLX4 / S3 i USB

| Namjena | Sučelje / GPIO | Napomena |
| --- | --- | --- |
| USB OTG 2.0 Host | D+ / D- na ploči | Spajanje FLX4 kontrolera ili USB Hub-a s USB stickom |
| UART Control Link (S3 fallback) | GPIO28 (RX), GPIO29 (TX) | 0xA5 / 0xA6 protokol prema pomoćnoj kontrolnoj ploči |
