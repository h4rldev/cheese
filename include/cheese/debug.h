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
 * @brief Draw a resource-monitor panel.
 * @details A small translucent panel with one metric per row, top-left at
 * @c x,@c y. Draws directly (it emits no semantics nodes), so call it after
 * your UI and before @ref cheese_end, outside any container so it isn't
 * clipped.
 *
 * @param cheese The cheese context.
 * @param font The font for the panel.
 * @param x,y The panel's top-left position.
 * @param metrics The values to display.
 *
 * @pre
 * - @c cheese must be valid and cannot be `null`.
 * - @c font must be valid and cannot be `null`.
 * - @c metrics must be valid and cannot be `null`.
 */
void cheese_debug_monitor(cheese_t *cheese, cheese_font_t *font, f32 x, f32 y,
                          const cheese_debug_metrics_t *metrics);

#endif // !CHEESE_DEBUG_H
