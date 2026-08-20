#include "bsp_p4_m3.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "driver/sdmmc_host.h"
#include "ff.h"

#include <string.h>

static const char *TAG = "bsp_m3";

// ── Display pins & configuration ─────────────────────────────────────────────
#define BSP_LCD_BL_GPIO         GPIO_NUM_23
#define BSP_LCD_BL_ON_LEVEL     1
#define BSP_BL_LEDC_TIMER       LEDC_TIMER_0
#define BSP_BL_LEDC_CHANNEL     LEDC_CHANNEL_0
#define BSP_BL_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define BSP_BL_LEDC_RES         LEDC_TIMER_10_BIT
#define BSP_BL_LEDC_FREQ_HZ     5000
#define BSP_BL_DEFAULT_PCT      80

// ── Shared I2C bus (FT5426 touch @0x38 on pins 11/12 of J2 DSI connector) ────
#define BSP_I2C_PORT            I2C_NUM_1
#define BSP_I2C_SDA_GPIO        GPIO_NUM_7
#define BSP_I2C_SCL_GPIO        GPIO_NUM_8

// ── Master Out: PCM5102A I2S DAC ─────────────────────────────────────────────
#define BSP_PCM5102_I2S_NUM        I2S_NUM_1
#define BSP_PCM5102_BCLK_GPIO      GPIO_NUM_50
#define BSP_PCM5102_WS_GPIO        GPIO_NUM_52
#define BSP_PCM5102_DOUT_GPIO      GPIO_NUM_51
#define BSP_PCM5102_MCLK_GPIO      I2S_GPIO_UNUSED

// ── MicroSD card on SDMMC Slot 0 ─────────────────────────────────────────────
#define BSP_SD_LDO_CHAN             4
#define BSP_SD_MOUNT_ATTEMPTS       3
#define BSP_SD_MOUNT_RETRY_DELAY_MS 150

#if defined(CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE) && \
    ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
static esp_err_t bsp_sdmmc_host_already_initialized(void)
{
    return ESP_OK;
}
#endif

// ── MIPI DSI PHY power (ESP32-P4 internal LDO VO3 → VDD_MIPI_DPHY 2.5 V) ──────
#define BSP_MIPI_LDO_CHAN       3
#define BSP_MIPI_LDO_MV         2500

// ── MIPI DSI link ────────────────────────────────────────────────────────────
#define BSP_DSI_LANE_NUM        2
#define BSP_DSI_LANE_MBPS       500

// ── 5.0" MIPI-DSI IPS Video Timing (800x480 native landscape) ────────────────
#define BSP_DPI_CLK_MHZ         30
#define BSP_LCD_HSYNC           40
#define BSP_LCD_HBP             40
#define BSP_LCD_HFP             40
#define BSP_LCD_VSYNC           9
#define BSP_LCD_VBP             29
#define BSP_LCD_VFP             13

static esp_lcd_panel_handle_t   s_panel    = NULL;
static esp_ldo_channel_handle_t s_mipi_ldo = NULL;
static i2c_master_bus_handle_t  s_i2c_bus  = NULL;
static esp_lcd_touch_handle_t   s_touch    = NULL;
static i2s_chan_handle_t        s_i2s_tx_pcm5102 = NULL;
static bool                     s_i2s_tx_pcm5102_enabled = false;
static esp_codec_dev_handle_t   s_codec    = NULL;
static bsp_audio_out_t          s_audio_out = BSP_AUDIO_OUT_RCA;
static bool                     s_speaker_pa_enabled = false;
static bsp_monitor_route_t      s_monitor_route = BSP_MONITOR_ROUTE_HEADPHONES;
static sdmmc_card_t            *s_sd_card  = NULL;
static sd_pwr_ctrl_handle_t     s_sd_pwr   = NULL;

esp_lcd_panel_handle_t bsp_display_get_panel_handle(void)
{
    return s_panel;
}

esp_lcd_touch_handle_t bsp_touch_get_handle(void)
{
    return s_touch;
}

i2c_master_bus_handle_t bsp_get_i2c_bus(void)
{
    return s_i2c_bus;
}

static esp_err_t bsp_i2c_bus_init(void)
{
    if (s_i2c_bus) {
        return ESP_OK;
    }
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = BSP_I2C_SDA_GPIO,
        .scl_io_num = BSP_I2C_SCL_GPIO,
        .i2c_port   = BSP_I2C_PORT,
    };
    return i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
}

_Static_assert(BSP_LCD_FRAMEBUFFER_COUNT == 1u,
               "the current LVGL/PPA backend supports exactly one DPI framebuffer");

esp_err_t bsp_display_init(void)
{
    // ── 1. Power up MIPI DSI PHY via internal LDO ───────────────────────────
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id    = BSP_MIPI_LDO_CHAN,
        .voltage_mv = BSP_MIPI_LDO_MV,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &s_mipi_ldo));
    ESP_LOGI(TAG, "MIPI DSI PHY powered (LDO VO%d @ %d mV)", BSP_MIPI_LDO_CHAN, BSP_MIPI_LDO_MV);

    // ── 2. Create DSI bus ───────────────────────────────────────────────────
    esp_lcd_dsi_bus_handle_t dsi_bus = NULL;
    esp_lcd_dsi_bus_config_t bus_cfg = {
        .bus_id             = 0,
        .num_data_lanes     = BSP_DSI_LANE_NUM,
        .phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = BSP_DSI_LANE_MBPS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus));

    // ── 3. DPI panel config (RGB565, 800x480 @ 30 MHz) ──────────────────────
    esp_lcd_dpi_panel_config_t dpi_cfg = {
        .virtual_channel    = 0,
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = BSP_DPI_CLK_MHZ,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
        .in_color_format    = LCD_COLOR_FMT_RGB565,
#else
        .pixel_format       = LCD_COLOR_PIXEL_FORMAT_RGB565,
#endif
        .num_fbs            = BSP_LCD_FRAMEBUFFER_COUNT,
        .video_timing = {
            .h_size            = BSP_LCD_H_RES,
            .v_size            = BSP_LCD_V_RES,
            .hsync_pulse_width = BSP_LCD_HSYNC,
            .hsync_back_porch  = BSP_LCD_HBP,
            .hsync_front_porch = BSP_LCD_HFP,
            .vsync_pulse_width = BSP_LCD_VSYNC,
            .vsync_back_porch  = BSP_LCD_VBP,
            .vsync_front_porch = BSP_LCD_VFP,
        },
    };

    // ── 4. Create standard DPI video mode panel ──────────────────────────────
    ESP_ERROR_CHECK(esp_lcd_new_panel_dpi(dsi_bus, &dpi_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_LOGI(TAG, "5.0\" DSI DPI panel up (%dx%d, %d MHz DPI, RGB565, single framebuffer)",
             BSP_LCD_H_RES, BSP_LCD_V_RES, BSP_DPI_CLK_MHZ);

    // ── 5. Backlight PWM ────────────────────────────────────────────────────
    ledc_timer_config_t bl_timer = {
        .speed_mode      = BSP_BL_LEDC_MODE,
        .timer_num       = BSP_BL_LEDC_TIMER,
        .duty_resolution = BSP_BL_LEDC_RES,
        .freq_hz         = BSP_BL_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    (void)ledc_timer_config(&bl_timer);

    ledc_channel_config_t bl_chan = {
        .gpio_num   = BSP_LCD_BL_GPIO,
        .speed_mode = BSP_BL_LEDC_MODE,
        .channel    = BSP_BL_LEDC_CHANNEL,
        .timer_sel  = BSP_BL_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    (void)ledc_channel_config(&bl_chan);
    bsp_display_set_backlight(BSP_BL_DEFAULT_PCT);

    return ESP_OK;
}

void bsp_display_set_backlight(uint8_t pct)
{
    if (pct > 100) pct = 100;
    uint32_t duty = (1023u * pct) / 100u;
    ledc_set_duty(BSP_BL_LEDC_MODE, BSP_BL_LEDC_CHANNEL, duty);
    ledc_update_duty(BSP_BL_LEDC_MODE, BSP_BL_LEDC_CHANNEL);
}

esp_err_t bsp_touch_init(void)
{
    ESP_RETURN_ON_ERROR(bsp_i2c_bus_init(), TAG, "I2C bus init failed");

    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    tp_io_cfg.scl_speed_hz = 400000;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_i2c_bus, &tp_io_cfg, &tp_io),
                        TAG, "FT5426 panel IO failed");

    // Native panel is 800x480 landscape. Touch controller maps 1:1 without swap/mirror.
    esp_lcd_touch_config_t tp_cfg = {
        .x_max        = BSP_LCD_H_RES,   // 800
        .y_max        = BSP_LCD_V_RES,   // 480
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset     = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy  = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    esp_err_t err = esp_lcd_touch_new_i2c_ft5x06(tp_io, &tp_cfg, &s_touch);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "FT5426 touch controller not detected (no screen attached?): %s", esp_err_to_name(err));
        s_touch = NULL;
        return ESP_OK;
    }

    ESP_LOGI(TAG, "FT5426 touch ready (I2C SDA=%d SCL=%d, addr=0x38, 800x480 landscape)",
             BSP_I2C_SDA_GPIO, BSP_I2C_SCL_GPIO);
    return ESP_OK;
}

esp_codec_dev_handle_t bsp_audio_get_codec_dev(void)
{
    return s_codec;
}

i2s_chan_handle_t bsp_audio_get_main_i2s_tx(void)
{
    return s_i2s_tx_pcm5102;
}

esp_err_t bsp_audio_force_safe_boot_state(void)
{
    s_speaker_pa_enabled = false;
    s_audio_out = BSP_AUDIO_OUT_RCA;
    s_monitor_route = BSP_MONITOR_ROUTE_HEADPHONES;
    return ESP_OK;
}

static esp_err_t bsp_audio_init_i2s_pcm5102(void)
{
#if CONFIG_BSP_PCM5102A_MAIN_OUT
    if (s_i2s_tx_pcm5102) {
        return ESP_OK;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(BSP_PCM5102_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_i2s_tx_pcm5102, NULL), TAG, "pcm5102 i2s_new_channel failed");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = BSP_PCM5102_MCLK_GPIO,
            .bclk = BSP_PCM5102_BCLK_GPIO,
            .ws = BSP_PCM5102_WS_GPIO,
            .dout = BSP_PCM5102_DOUT_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_i2s_tx_pcm5102, &std_cfg), TAG, "pcm5102 i2s std init failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_i2s_tx_pcm5102), TAG, "pcm5102 i2s enable failed");
    s_i2s_tx_pcm5102_enabled = true;
    ESP_LOGI(TAG, "PCM5102A main out ready: BCLK=%d WS=%d DOUT=%d",
             BSP_PCM5102_BCLK_GPIO, BSP_PCM5102_WS_GPIO, BSP_PCM5102_DOUT_GPIO);
#endif
    return ESP_OK;
}

esp_err_t bsp_audio_init(void)
{
    ESP_RETURN_ON_ERROR(bsp_audio_force_safe_boot_state(), TAG,
                        "audio safe boot state failed");
    ESP_RETURN_ON_ERROR(bsp_audio_init_i2s_pcm5102(), TAG, "PCM5102A init failed");
    return ESP_OK;
}

esp_err_t bsp_audio_set_speaker_pa_enabled(bool enabled)
{
    s_speaker_pa_enabled = enabled;
    return ESP_OK;
}

bool bsp_audio_get_speaker_pa_enabled(void)
{
    return s_speaker_pa_enabled;
}

esp_err_t bsp_audio_set_monitor_route(bsp_monitor_route_t route)
{
    s_monitor_route = route;
    return ESP_OK;
}

bsp_monitor_route_t bsp_audio_get_monitor_route(void)
{
    return s_monitor_route;
}

esp_err_t bsp_audio_set_output(bsp_audio_out_t out)
{
    s_audio_out = out;
    return ESP_OK;
}

bsp_audio_out_t bsp_audio_get_output(void)
{
    return s_audio_out;
}

esp_err_t bsp_audio_main_i2s_set_sample_rate(uint32_t sample_rate)
{
#if CONFIG_BSP_PCM5102A_MAIN_OUT
    if (!s_i2s_tx_pcm5102 || sample_rate == 0u) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_i2s_tx_pcm5102_enabled) {
        ESP_RETURN_ON_ERROR(i2s_channel_disable(s_i2s_tx_pcm5102), TAG,
                            "pcm5102 disable failed");
        s_i2s_tx_pcm5102_enabled = false;
    }
    i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate);
    ESP_RETURN_ON_ERROR(i2s_channel_reconfig_std_clock(s_i2s_tx_pcm5102, &clk_cfg), TAG, "pcm5102 clock reconfig failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_i2s_tx_pcm5102), TAG, "pcm5102 enable failed");
    s_i2s_tx_pcm5102_enabled = true;
    return ESP_OK;
#else
    (void)sample_rate;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t bsp_audio_main_i2s_abort_write(void)
{
#if CONFIG_BSP_PCM5102A_MAIN_OUT
    if (!s_i2s_tx_pcm5102) return ESP_ERR_INVALID_STATE;
    if (!s_i2s_tx_pcm5102_enabled) return ESP_OK;
    esp_err_t rc = i2s_channel_disable(s_i2s_tx_pcm5102);
    if (rc == ESP_OK) s_i2s_tx_pcm5102_enabled = false;
    return rc;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t bsp_sd_init(void)
{
    if (s_sd_card) {
        return ESP_OK;
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
#if defined(CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE) && \
    ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
    host.init = bsp_sdmmc_host_already_initialized;
#endif
    if (!s_sd_pwr) {
        sd_pwr_ctrl_ldo_config_t ldo_config = {
            .ldo_chan_id = BSP_SD_LDO_CHAN,
        };
        esp_err_t pwr_rc = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &s_sd_pwr);
        if (pwr_rc != ESP_OK) {
            ESP_LOGW(TAG, "SD LDO%d power control unavailable (%s) — /sd unavailable",
                     BSP_SD_LDO_CHAN, esp_err_to_name(pwr_rc));
            return ESP_OK;
        }
    }
    host.pwr_ctrl_handle = s_sd_pwr;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = GPIO_NUM_43;
    slot_config.cmd = GPIO_NUM_44;
    slot_config.d0  = GPIO_NUM_39;
    slot_config.d1  = GPIO_NUM_40;
    slot_config.d2  = GPIO_NUM_41;
    slot_config.d3  = GPIO_NUM_42;
    slot_config.width = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };

    ESP_LOGI(TAG, "mounting SD at /sd (slot=%d width=%d clk=%d cmd=%d d0=%d d1=%d d2=%d d3=%d)",
             host.slot, slot_config.width, slot_config.clk, slot_config.cmd,
             slot_config.d0, slot_config.d1, slot_config.d2, slot_config.d3);

    esp_err_t rc = ESP_FAIL;
    for (int attempt = 1; attempt <= BSP_SD_MOUNT_ATTEMPTS; attempt++) {
        rc = esp_vfs_fat_sdmmc_mount("/sd", &host, &slot_config, &mount_config, &s_sd_card);
        if (rc == ESP_OK) {
            break;
        }
        s_sd_card = NULL;
        ESP_LOGW(TAG, "SD mount attempt %d/%d failed (%s)%s",
                 attempt, BSP_SD_MOUNT_ATTEMPTS, esp_err_to_name(rc),
                 attempt < BSP_SD_MOUNT_ATTEMPTS ? " — retrying" : "");
        if (attempt < BSP_SD_MOUNT_ATTEMPTS) {
            vTaskDelay(pdMS_TO_TICKS(BSP_SD_MOUNT_RETRY_DELAY_MS));
        }
    }
    if (rc != ESP_OK) {
        s_sd_card = NULL;
        ESP_LOGW(TAG, "SD mount skipped (%s) — /sd unavailable; USB media path continues",
                 esp_err_to_name(rc));
        return ESP_OK;
    }

    ESP_LOGI(TAG, "SD card mounted at /sd");
    sdmmc_card_print_info(stdout, s_sd_card);
    return ESP_OK;
}

bool bsp_sd_is_mounted(void)
{
    return s_sd_card != NULL;
}

esp_err_t bsp_sd_get_status(bsp_sd_status_t *out_status)
{
    ESP_RETURN_ON_FALSE(out_status, ESP_ERR_INVALID_ARG, TAG, "missing SD status output");
    memset(out_status, 0, sizeof(*out_status));

    if (!s_sd_card) {
        return ESP_ERR_NOT_FOUND;
    }

    FATFS *fs = NULL;
    DWORD free_clusters = 0;
    if (f_getfree("/sd", &free_clusters, &fs) != FR_OK || !fs) {
        return ESP_ERR_NOT_FOUND;
    }

    uint32_t sector_size = 512u;
#if FF_MAX_SS != FF_MIN_SS
    sector_size = fs->ssize;
#endif
    uint64_t cluster_bytes = (uint64_t)fs->csize * (uint64_t)sector_size;
    uint64_t total_clusters = fs->n_fatent > 2 ? (uint64_t)(fs->n_fatent - 2) : 0;

    out_status->mounted = true;
    out_status->sector_size = sector_size;
    out_status->free_bytes = (uint64_t)free_clusters * cluster_bytes;
    out_status->total_bytes = total_clusters * cluster_bytes;
    return ESP_OK;
}
