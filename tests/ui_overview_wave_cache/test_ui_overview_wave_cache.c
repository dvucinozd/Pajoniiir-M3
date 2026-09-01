#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ui_overview_wave_cache.h"

static const uint16_t palette[] = {
    0x0000, 0xF16E, 0x235F, 0x475C, 0xE71D,
    0x1F32, 0xFD66, 0x9ADF, 0x3989,
};

#define TEST_VIEW_W 16
#define TEST_STRIP_W 32
#define TEST_H 12
#define TEST_MARGIN_W 8
#define TEST_EDGE_TRIGGER_MS 4500

static int count_changed_pixels(const uint16_t *a, const uint16_t *b, int count)
{
    int changed = 0;
    for (int i = 0; i < count; i++) {
        if (a[i] != b[i]) {
            changed++;
        }
    }
    return changed;
}

static void compose_visible_view(const uint16_t *strip,
                                 int strip_stride,
                                 int height,
                                 const ui_overview_wave_cache_report_t *report,
                                 uint16_t *view,
                                 int view_stride)
{
    assert(strip);
    assert(report);
    assert(view);
    assert(report->blit_required);

    memset(view, 0, (size_t)view_stride * (size_t)height * sizeof(*view));
    for (uint8_t seg = 0; seg < report->blit_count; seg++) {
        const ui_overview_wave_cache_blit_t *blit = &report->blit[seg];
        assert((int)blit->dst_x_px + (int)blit->width_px <= view_stride);
        for (int y = 0; y < height; y++) {
            memcpy(&view[y * view_stride + blit->dst_x_px],
                   &strip[y * strip_stride + blit->src_x_px],
                   (size_t)blit->width_px * sizeof(*view));
        }
    }
}

static void test_initial_update_renders_full_view(void)
{
    uint8_t samples[64];
    memset(samples, 0x1Fu, sizeof(samples));
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[16 * 12] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind(&cache, pixels, 16, 16, 12,
                                       palette, sizeof(palette) / sizeof(palette[0])));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         32000, 16000, &report));

    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_FULL);
    assert(report.scroll_dx_px == 0);
    assert(report.columns_rendered == 16);
    assert(report.blit_required);
    assert(cache.valid);
}

static void test_small_center_advance_rebuilds_compat_view(void)
{
    uint8_t samples[256];
    for (int i = 0; i < 256; i++) {
        samples[i] = (uint8_t)(0x10u + (i & 0x0Fu));
    }
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[16 * 12] = {0};
    uint16_t before[16 * 12] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind(&cache, pixels, 16, 16, 12,
                                       palette, sizeof(palette) / sizeof(palette[0])));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         32000, 16000, &report));
    memcpy(before, pixels, sizeof(before));

    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         33000, 16000, &report));

    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_FULL);
    assert(report.scroll_dx_px == 0);
    assert(report.columns_rendered == 16);
    assert(report.blit_required);
    assert(count_changed_pixels(before, pixels, 16 * 12) > 0);
}

static void test_steady_advance_uses_offset_without_mutating_pixels(void)
{
    uint8_t samples[256];
    for (int i = 0; i < 256; i++) {
        samples[i] = (uint8_t)(0x20u + (i & 0x0Fu));
    }
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[TEST_STRIP_W * TEST_H] = {0};
    uint16_t before[TEST_STRIP_W * TEST_H] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind_strip(&cache, pixels,
                                             TEST_STRIP_W,
                                             TEST_STRIP_W,
                                             TEST_VIEW_W,
                                             TEST_H,
                                             TEST_MARGIN_W,
                                             palette,
                                             sizeof(palette) / sizeof(palette[0])));

    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10000, 8000, &report));
    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_FULL);
    memcpy(before, pixels, sizeof(before));

    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10500, 8000, &report));

    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_OFFSET);
    assert(report.columns_rendered == 0);
    assert(report.blit_required);
    assert(report.blit_count >= 1);
    assert(report.blit[0].src_x_px != TEST_MARGIN_W);
    assert(memcmp(before, pixels, sizeof(before)) == 0);
}

static void test_visible_shape_translates_without_deforming(void)
{
    uint8_t samples[512];
    for (int i = 0; i < (int)sizeof(samples); i++) {
        uint8_t amp = (uint8_t)(((i * 13) ^ (i >> 2)) & 0x1Fu);
        samples[i] = (uint8_t)((((unsigned)i >> 3) & 0x07u) << 5) | amp;
    }
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[TEST_STRIP_W * TEST_H] = {0};
    uint16_t before[TEST_VIEW_W * TEST_H] = {0};
    uint16_t after[TEST_VIEW_W * TEST_H] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    assert(ui_overview_wave_cache_bind_strip(&cache, pixels,
                                             TEST_STRIP_W,
                                             TEST_STRIP_W,
                                             TEST_VIEW_W,
                                             TEST_H,
                                             TEST_MARGIN_W,
                                             palette,
                                             sizeof(palette) / sizeof(palette[0])));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10000, 8000, &report));
    compose_visible_view(pixels, TEST_STRIP_W, TEST_H, &report,
                         before, TEST_VIEW_W);

    /* 8 s / 16 px = 500 ms per pixel. A 500-ms center advance must be a
     * rigid one-column translation, not a re-sampled or deformed view. */
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10500, 8000, &report));
    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_OFFSET);
    assert(report.scroll_dx_px == 1);
    compose_visible_view(pixels, TEST_STRIP_W, TEST_H, &report,
                         after, TEST_VIEW_W);

    for (int y = 0; y < TEST_H; y++) {
        assert(memcmp(&before[y * TEST_VIEW_W + 1],
                      &after[y * TEST_VIEW_W],
                      (TEST_VIEW_W - 1u) * sizeof(uint16_t)) == 0);
    }
}

static void test_long_scroll_stays_rigid_across_edges_and_ring_wraps(void)
{
    uint8_t samples[2048];
    for (int i = 0; i < (int)sizeof(samples); i++) {
        uint8_t amp = (uint8_t)(((i * 29) ^ (i >> 1) ^ (i >> 5)) & 0x1Fu);
        samples[i] = (uint8_t)((((unsigned)i >> 4) & 0x07u) << 5) | amp;
    }
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[TEST_STRIP_W * TEST_H] = {0};
    uint16_t before[TEST_VIEW_W * TEST_H] = {0};
    uint16_t after[TEST_VIEW_W * TEST_H] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;
    unsigned edge_updates = 0;
    unsigned wrapped_views = 0;

    assert(ui_overview_wave_cache_bind_strip(&cache, pixels,
                                             TEST_STRIP_W,
                                             TEST_STRIP_W,
                                             TEST_VIEW_W,
                                             TEST_H,
                                             TEST_MARGIN_W,
                                             palette,
                                             sizeof(palette) / sizeof(palette[0])));
    assert(ui_overview_wave_cache_update(&cache, &source, 256000, NULL,
                                         20000, 8000, &report));
    compose_visible_view(pixels, TEST_STRIP_W, TEST_H, &report,
                         before, TEST_VIEW_W);

    /* Advance by exactly one visible pixel for long enough to repeatedly refill
     * both physical ends of the strip and wrap the ring-buffer view. Every
     * overlapping column must remain bit-identical to the preceding view. */
    for (unsigned step = 1; step <= 160; step++) {
        uint32_t center_ms = 20000u + (step * 500u);
        assert(ui_overview_wave_cache_update(&cache, &source, 256000, NULL,
                                             center_ms, 8000, &report));
        assert(report.kind == UI_OVERVIEW_WAVE_CACHE_OFFSET ||
               report.kind == UI_OVERVIEW_WAVE_CACHE_EDGE);
        assert(report.scroll_dx_px == 1);
        if (report.kind == UI_OVERVIEW_WAVE_CACHE_EDGE) {
            edge_updates++;
        }
        if (report.blit_count == 2) {
            wrapped_views++;
        }

        compose_visible_view(pixels, TEST_STRIP_W, TEST_H, &report,
                             after, TEST_VIEW_W);
        for (int y = 0; y < TEST_H; y++) {
            assert(memcmp(&before[y * TEST_VIEW_W + 1],
                          &after[y * TEST_VIEW_W],
                          (TEST_VIEW_W - 1u) * sizeof(uint16_t)) == 0);
        }
        memcpy(before, after, sizeof(before));
    }

    assert(edge_updates > 1);
    assert(wrapped_views > 1);
}

static void test_edge_advance_renders_small_batch_without_full_rebuild(void)
{
    uint8_t samples[256];
    for (int i = 0; i < 256; i++) {
        samples[i] = (uint8_t)(0x30u + (i & 0x0Fu));
    }
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[TEST_STRIP_W * TEST_H] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind_strip(&cache, pixels,
                                             TEST_STRIP_W,
                                             TEST_STRIP_W,
                                             TEST_VIEW_W,
                                             TEST_H,
                                             TEST_MARGIN_W,
                                             palette,
                                             sizeof(palette) / sizeof(palette[0])));

    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10000, 8000, &report));
    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_FULL);

    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10000 + TEST_EDGE_TRIGGER_MS, 8000, &report));

    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_EDGE);
    assert(report.columns_rendered > 0);
    assert(report.columns_rendered <= UI_OVERVIEW_WAVE_CACHE_EDGE_BATCH_PX);
    assert(report.blit_count >= 1);
}

static void test_wrap_reports_two_blit_segments(void)
{
    uint8_t samples[256];
    for (int i = 0; i < 256; i++) {
        samples[i] = (uint8_t)(0x40u + (i & 0x0Fu));
    }
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[TEST_STRIP_W * TEST_H] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind_strip(&cache, pixels,
                                             TEST_STRIP_W,
                                             TEST_STRIP_W,
                                             TEST_VIEW_W,
                                             TEST_H,
                                             TEST_MARGIN_W,
                                             palette,
                                             sizeof(palette) / sizeof(palette[0])));

    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10000, 8000, &report));
    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_FULL);

    ui_overview_wave_cache_test_force_view_origin(&cache, TEST_STRIP_W - 4);
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10000, 8000, &report));

    assert(report.blit_count == 2);
    assert((uint16_t)(report.blit[0].width_px + report.blit[1].width_px) == TEST_VIEW_W);
    assert(report.blit[1].src_x_px == 0);
}

static void test_window_change_forces_full_redraw(void)
{
    uint8_t samples[64];
    memset(samples, 0x1Fu, sizeof(samples));
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[TEST_STRIP_W * TEST_H] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind_strip(&cache, pixels,
                                             TEST_STRIP_W,
                                             TEST_STRIP_W,
                                             TEST_VIEW_W,
                                             TEST_H,
                                             TEST_MARGIN_W,
                                             palette,
                                             sizeof(palette) / sizeof(palette[0])));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         32000, 16000, &report));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         32000, 8000, &report));

    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_FULL);
    assert(report.columns_rendered == TEST_STRIP_W);
}

static void test_subpixel_advance_accumulates_until_visible_scroll(void)
{
    uint8_t samples[256];
    for (int i = 0; i < 256; i++) {
        samples[i] = (uint8_t)(0x10u + (i & 0x0Fu));
    }
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[TEST_STRIP_W * TEST_H] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind_strip(&cache, pixels,
                                             TEST_STRIP_W,
                                             TEST_STRIP_W,
                                             TEST_VIEW_W,
                                             TEST_H,
                                             TEST_MARGIN_W,
                                             palette,
                                             sizeof(palette) / sizeof(palette[0])));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         32000, 16000, &report));
    assert(!ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                          32250, 16000, &report));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         32550, 16000, &report));

    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_OFFSET);
    assert(report.scroll_dx_px == 1);
    assert(report.columns_rendered == 0);
    assert(report.blit_required);
}

static void test_large_jump_forces_full_redraw(void)
{
    uint8_t samples[64];
    memset(samples, 0x1Fu, sizeof(samples));
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[16 * 12] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind(&cache, pixels, 16, 16, 12,
                                       palette, sizeof(palette) / sizeof(palette[0])));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         8000, 16000, &report));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         56000, 16000, &report));

    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_FULL);
    assert(report.columns_rendered == 16);
}

static void test_backward_jump_loop_wrap_forces_full_redraw(void)
{
    uint8_t samples[256];
    for (int i = 0; i < 256; i++) {
        samples[i] = (uint8_t)(0x10u + (i & 0x0Fu));
    }
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[TEST_STRIP_W * TEST_H] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind_strip(&cache, pixels,
                                             TEST_STRIP_W,
                                             TEST_STRIP_W,
                                             TEST_VIEW_W,
                                             TEST_H,
                                             TEST_MARGIN_W,
                                             palette,
                                             sizeof(palette) / sizeof(palette[0])));

    /* 1. Initial full render at 20000 ms */
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         20000, 8000, &report));
    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_FULL);

    /* 2. Advance to 24000 ms (near loop end) */
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         24000, 8000, &report));

    /* 3. Loop wraps back to 20000 ms (jump of -4000 ms) */
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         20000, 8000, &report));

    /* Must trigger full rebuild so entire strip is refreshed cleanly without collapse/stale data */
    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_FULL);
    assert(report.columns_rendered == TEST_STRIP_W);
    assert(report.blit_required);
}

static void test_missing_source_returns_false_without_blit(void)
{
    uint16_t pixels[16 * 12] = {0};
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_NONE,
        .samples = NULL,
        .sample_count = 0,
    };
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind(&cache, pixels, 16, 16, 12,
                                       palette, sizeof(palette) / sizeof(palette[0])));
    assert(!ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                          32000, 16000, &report));
    assert(report.kind == UI_OVERVIEW_WAVE_CACHE_NONE);
    assert(!report.blit_required);
}

static void test_stats_count_update_kinds_columns_and_blits(void)
{
    uint8_t samples[256];
    for (int i = 0; i < 256; i++) {
        samples[i] = (uint8_t)(0x20u + (i & 0x0Fu));
    }
    ui_waveform_source_t source = {
        .kind = UI_WAVEFORM_SOURCE_HIGH,
        .samples = samples,
        .sample_count = sizeof(samples),
    };
    uint16_t pixels[TEST_STRIP_W * TEST_H] = {0};
    ui_overview_wave_cache_t cache = {0};
    ui_overview_wave_cache_report_t report;

    ui_overview_wave_cache_reset(&cache);
    assert(ui_overview_wave_cache_bind_strip(&cache, pixels,
                                             TEST_STRIP_W,
                                             TEST_STRIP_W,
                                             TEST_VIEW_W,
                                             TEST_H,
                                             TEST_MARGIN_W,
                                             palette,
                                             sizeof(palette) / sizeof(palette[0])));

    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10000, 8000, &report));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10500, 8000, &report));
    assert(ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                         10000 + TEST_EDGE_TRIGGER_MS, 8000, &report));
    uint16_t edge_columns = report.columns_rendered;
    assert(!ui_overview_wave_cache_update(&cache, &source, 64000, NULL,
                                          10000 + TEST_EDGE_TRIGGER_MS, 8000, &report));

    ui_overview_wave_cache_stats_t stats;
    ui_overview_wave_cache_get_stats(&cache, &stats);
    assert(stats.update_count[UI_OVERVIEW_WAVE_CACHE_FULL] == 1);
    assert(stats.update_count[UI_OVERVIEW_WAVE_CACHE_OFFSET] == 1);
    assert(stats.update_count[UI_OVERVIEW_WAVE_CACHE_EDGE] == 1);
    assert(stats.update_count[UI_OVERVIEW_WAVE_CACHE_NONE] == 1);
    assert(stats.total_columns_rendered == (uint32_t)TEST_STRIP_W + (uint32_t)edge_columns);
    assert(stats.total_blits >= 3);

    ui_overview_wave_cache_reset_stats(&cache);
    ui_overview_wave_cache_get_stats(&cache, &stats);
    assert(stats.update_count[UI_OVERVIEW_WAVE_CACHE_FULL] == 0);
    assert(stats.total_columns_rendered == 0);
    assert(stats.total_blits == 0);
}

int main(void)
{
    test_initial_update_renders_full_view();
    test_small_center_advance_rebuilds_compat_view();
    test_steady_advance_uses_offset_without_mutating_pixels();
    test_visible_shape_translates_without_deforming();
    test_long_scroll_stays_rigid_across_edges_and_ring_wraps();
    test_edge_advance_renders_small_batch_without_full_rebuild();
    test_wrap_reports_two_blit_segments();
    test_window_change_forces_full_redraw();
    test_subpixel_advance_accumulates_until_visible_scroll();
    test_large_jump_forces_full_redraw();
    test_backward_jump_loop_wrap_forces_full_redraw();
    test_missing_source_returns_false_without_blit();
    test_stats_count_update_kinds_columns_and_blits();
    puts("ui_overview_wave_cache tests passed");
    return 0;
}
