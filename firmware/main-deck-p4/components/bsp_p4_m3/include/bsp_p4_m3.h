#pragma once

// Board support for JC-ESP32P4-M3-DEV (ESP32-P4 + 5.0" MIPI-DSI 800x480 + FT5426 + PCM5102A).
//
// Pin reference for JC-ESP32P4-M3-DEV:
//   MIPI-DSI (J2 15-pin FPC):
//     DSI Lane 1: DSI_A_DATA1_N / DSI_A_DATA1_P (Pins 2, 3)
//     DSI Clock:  DSI_A_CLK_N   / DSI_A_CLK_P   (Pins 5, 6)
//     DSI Lane 0: DSI_A_DATA0_N / DSI_A_DATA0_P (Pins 8, 9)
//     Touch I2C:  ES_I2C_SCL (Pin 11), ES_I2C_SDA (Pin 12) -> FT5426 addr 0x38
//     Power:      +3.3V (Pins 14, 15), GND (Pins 1, 4, 7, 10, 13)
//
//   SDMMC (MicroSD slot 0):
//     D0..D3:     GPIO39..42, CMD: GPIO44, CLK: GPIO43 (LDO channel 4)
//
//   PCM5102A Master Audio DAC (I2S Unit 1):
//     BCLK: GPIO1, WS/LRCK: GPIO2, DOUT: GPIO3
//
//   ESP32-C6 Coprocessor (SDIO slot 1):
//     D0..D3: GPIO14..17, CLK: GPIO18, CMD: GPIO19, RST: GPIO54

#include "esp_err.h"
#include "esp_lcd_types.h"
#include "esp_lcd_touch.h"
#include "driver/i2c_master.h"
#include "driver/i2s_types.h"
#include "esp_codec_dev.h"
#include <stdbool.h>
#include <stdint.h>

// Native panel geometry: 5.0" MIPI-DSI IPS panel in native landscape 800x480.
#define BSP_LCD_H_RES   800
#define BSP_LCD_V_RES   480

// Single DPI framebuffer scanned by ESP32-P4 MIPI-DSI controller.
#define BSP_LCD_FRAMEBUFFER_COUNT 1u

esp_err_t bsp_display_init(void);   // 5.0" MIPI-DSI 800x480 DPI video mode
void      bsp_display_set_backlight(uint8_t pct);
esp_err_t bsp_touch_init(void);     // FT5426 I2C touch controller (addr 0x38)
esp_err_t bsp_audio_init(void);     // PCM5102A MASTER I2S output
esp_err_t bsp_audio_force_safe_boot_state(void);
esp_err_t bsp_sd_init(void);        // MicroSD SDMMC slot 0 (/sd)

typedef struct {
    bool mounted;
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint32_t sector_size;
} bsp_sd_status_t;

bool bsp_sd_is_mounted(void);
esp_err_t bsp_sd_get_status(bsp_sd_status_t *out_status);

// Panel handle for LVGL flush callback and refresh-done ISR
esp_lcd_panel_handle_t bsp_display_get_panel_handle(void);

// FT5426 touch handle for LVGL pointer indev
esp_lcd_touch_handle_t bsp_touch_get_handle(void);

// Shared I2C master bus handle
i2c_master_bus_handle_t bsp_get_i2c_bus(void);

// Legacy codec handle (NULL when dedicated PCM5102A is active)
esp_codec_dev_handle_t bsp_audio_get_codec_dev(void);

typedef enum {
    BSP_AUDIO_OUT_SPEAKER = 0,
    BSP_AUDIO_OUT_RCA,
} bsp_audio_out_t;

esp_err_t       bsp_audio_set_output(bsp_audio_out_t out);
bsp_audio_out_t bsp_audio_get_output(void);

typedef enum {
    BSP_MONITOR_ROUTE_HEADPHONES = 0,
    BSP_MONITOR_ROUTE_SPEAKER,
} bsp_monitor_route_t;

esp_err_t bsp_audio_set_monitor_route(bsp_monitor_route_t route);
bsp_monitor_route_t bsp_audio_get_monitor_route(void);
esp_err_t bsp_audio_set_speaker_pa_enabled(bool enabled);
bool bsp_audio_get_speaker_pa_enabled(void);

// PCM5102A main stereo output
i2s_chan_handle_t bsp_audio_get_main_i2s_tx(void);
esp_err_t bsp_audio_main_i2s_set_sample_rate(uint32_t sample_rate);
esp_err_t bsp_audio_main_i2s_abort_write(void);
