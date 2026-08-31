#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int x;
    int y;
    int w;
    int h;
} ui_overlay_rect_t;

bool ui_overlay_map_ppa270(ui_overlay_rect_t logical,
                           int logical_w,
                           int logical_h,
                           ui_overlay_rect_t *physical);

bool ui_overlay_map_ppa0(ui_overlay_rect_t logical,
                         int logical_w,
                         int logical_h,
                         ui_overlay_rect_t *physical);

/* Landscape mapping for a panel whose active image is horizontally mirrored.
 * The PPA mirror_x flag flips pixels inside this destination rectangle. */
bool ui_overlay_map_ppa0_mirror_x(ui_overlay_rect_t logical,
                                  int logical_w,
                                  int logical_h,
                                  ui_overlay_rect_t *physical);

void ui_overlay_i8_to_rgb565(const uint8_t *src,
                             int src_stride_px,
                             uint16_t *dst,
                             int dst_stride_px,
                             int width_px,
                             int height_px,
                             const uint16_t *palette,
                             size_t palette_count);

#ifdef __cplusplus
}
#endif
