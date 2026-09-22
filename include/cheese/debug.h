#ifndef CHEESE_DEBUG_H
#define CHEESE_DEBUG_H

/***********************************/

#include <cheese/types.h>

/***********************************/

/**
 * @brief A display-ready snapshot of process/renderer metrics.
 * @details Renderer- and platform-agnostic: the app fills it from whatever
 * source it has (e.g. `/proc` for RSS/PSS/CPU, the renderer's stats for frame
 * time, FPS, GPU load and VRAM). @ref cheese_debug_monitor only formats and
 * draws it. Fields are all display-ready:
 * - @c rss_mib, @c pss_mib: resident/proportional set size in MiB;
 * - @c cpu_pct: process CPU load, percent;
 * - @c frame_ms: last frame's render-thread CPU time;
 * - @c fps: frames per second;
 * - @c gpu_pct: GPU frame time as a percent of the frame budget;
 * - @c vram_used_mib, @c vram_total_mib: device-local heap usage (device-wide,
 *   not per-app).
 */
typedef struct {
  f32 rss_mib;
  f32 pss_mib;
  f32 cpu_pct;
  f32 frame_ms;
  f32 fps;
  f32 gpu_pct;
  f32 vram_used_mib;
  f32 vram_total_mib;
  f32 vram_budget_mib;
  b32 memory_budget_valid;
} cheese_debug_metrics_t;

//
//
//

/**
 * @brief Draw an overlay of the current frame's semantics tree.
 * @details Outlines every node's bounds and (when @c font is given) labels it
 * with its role and name/key. It draws directly, so it emits no nodes; call it
 * after your UI and before @ref cheese_end, outside any container so it isn't
 * clipped.
 *
 * @param cheese The cheese context.
 * @param font The font for labels, or null for outlines only.
 *
 * @pre @c cheese must be valid and cannot be `null`.
 */
void cheese_debug_overlay(cheese_t *cheese, cheese_font_t *font);

//
//
//

/**
 * @brief Draw a resource-monitor panel aligned inside a bounding box.
 * @details A small translucent panel with one metric per row, sized to its
 * widest row. The panel is placed inside the box whose top-left is @c x,@c y
 * and whose size is @c box_w,@c box_h according to @p align_x / @p align_y (the
 * usual `CHEESE_ALIGN_START` / `_CENTER` / `_END` / `_FILL` semantics, resolved
 * against the measured panel size), inset by @p margin on the aligned edges.
 * Pass @c box_w/@c box_h as the drawable size to anchor it to a corner of the
 * window; pass a small box to pin it. Draws directly (it emits no semantics
 * nodes), so call it after your UI and before @ref cheese_end, outside any
 * container so it isn't clipped.
 *
 * @param cheese The cheese context.
 * @param font The font for the panel.
 * @param x,y The bounding box's top-left position.
 * @param box_w,box_h The bounding box's size to align within.
 * @param align_x,align_y Alignment of the panel within the box (cross-axis
 * semantics; `CHEESE_ALIGN_FILL` behaves as `START`).
 * @param margin Inset from the aligned edge, in pixels.
 * @param metrics The values to display.
 *
 * @pre
 * - @c cheese must be valid and cannot be `null`.
 * - @c font must be valid and cannot be `null`.
 * - @c metrics must be valid and cannot be `null`.
 */
void cheese_debug_monitor(cheese_t *cheese, cheese_font_t *font, f32 x, f32 y,
                          f32 box_w, f32 box_h, cheese_alignment_t align_x,
                          cheese_alignment_t align_y, f32 margin,
                          const cheese_debug_metrics_t *metrics);

#endif // !CHEESE_DEBUG_H
