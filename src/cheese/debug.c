/***********************************/

#include <htils/darray.h>
#include <htils/string.h>

#include <cheese/debug.h>
#include <cheese/types.h>

#include <cheese/core/semantics.h>
#include <cheese/render/font.h>

#include <cheese/render/draw.h>

/***********************************/

void cheese_debug_overlay(cheese_t *cheese, cheese_font_t *font) {
  if (!cheese || !cheese->semantics_nodes)
    return;

  cheese_color_t outline_color = cheese_color_rgba(255, 0, 200, 255);
  cheese_color_t text_color = cheese_color_rgba(255, 255, 0, 255);

  u64 count = da_len(cheese->semantics_nodes);
  for (u64 i = 1; i < count; i++) {
    cheese_semantics_node_t *node = &cheese->semantics_nodes[i];
    if (node->bounds.w == 0 || node->bounds.h == 0)
      continue;

    f32 x = (f32)node->bounds.x;
    f32 y = (f32)node->bounds.y;
    f32 w = (f32)node->bounds.w;
    f32 h = (f32)node->bounds.h;

    cheese_draw_line(cheese, x, y, x + w, y, 1.0f, outline_color);
    cheese_draw_line(cheese, x, y + h, x + w, y + h, 1.0f, outline_color);
    cheese_draw_line(cheese, x, y, x, y + h, 1.0f, outline_color);
    cheese_draw_line(cheese, x + w, y, x + w, y + h, 1.0f, outline_color);

    if (!font)
      continue;

    const cstr *name = node->name ? node->name : node->key;
    const cstr *label = cheese_semantics_format(
        cheese, "%s %s", cheese_semantics_role_name(node->role),
        name ? name : "");
    string *text = string_from_cstr(cheese->frame_arena, label);

    cheese_draw_text(cheese, x, y - 2.0f, text, font, text_color, 0.8f);
  }
}

void cheese_debug_monitor(cheese_t *cheese, cheese_font_t *font, f32 x, f32 y,
                          f32 box_w, f32 box_h, cheese_alignment_t align_x,
                          cheese_alignment_t align_y, f32 margin,
                          const cheese_debug_metrics_t *metrics) {
  if (!cheese || !font || !metrics)
    return;

  char row_buf[8][48];
  snprintf(row_buf[0], sizeof(row_buf[0]), "CPU               %.1f %%",
           metrics->cpu_pct);
  snprintf(row_buf[1], sizeof(row_buf[1]), "GPU               %.1f %%",
           metrics->gpu_pct);
  snprintf(row_buf[2], sizeof(row_buf[2]), "FPS               %.1f",
           metrics->fps);
  snprintf(row_buf[3], sizeof(row_buf[3]), "frame             %.2f ms",
           metrics->frame_ms);
  snprintf(row_buf[4], sizeof(row_buf[4]), "GPU heap (global) %.1f / %.1f MiB",
           metrics->vram_used_mib, metrics->vram_total_mib);
  snprintf(row_buf[5], sizeof(row_buf[5]), "GPU budget        %.1f MiB%s",
           metrics->vram_budget_mib,
           metrics->memory_budget_valid ? "" : " (n/a)");
  snprintf(row_buf[6], sizeof(row_buf[5]), "RSS               %.1f MiB",
           metrics->rss_mib);
  snprintf(row_buf[7], sizeof(row_buf[6]), "PSS               %.1f MiB",
           metrics->pss_mib);

  const u32 count = 8;
  string *rows[8];

  f32 s = font->scale > 0.0f ? font->scale : 1.0f;
  f32 asc = (f32)font->base_ascender * s;
  f32 desc = (f32)font->base_descender * s;
  f32 line_h = (f32)font->base_line_height * s;
  if (line_h <= 0.0f)
    line_h = 16.0f;
  if (asc <= 0.0f)
    asc = line_h;

  f32 max_w = 0.0f;
  for (u32 i = 0; i < count; i++) {
    rows[i] = string_from_cstr(cheese->frame_arena, row_buf[i]);
    f32 w = cheese_font_measure_text(font, rows[i]);
    if (w > max_w)
      max_w = w;
  }

  f32 pad = 8.0f;
  f32 w = max_w + pad * 2.0f;
  f32 h = asc - desc + (f32)(count - 1) * line_h + pad * 2.0f;

  f32 px = x, py = y;
  switch (align_x) {
  case CHEESE_ALIGN_CENTER:
    px = x + (box_w - w) * 0.5f;
    break;
  case CHEESE_ALIGN_END:
    px = x + box_w - w - margin;
    break;
  default:
    px = x + margin;
    break;
  }
  switch (align_y) {
  case CHEESE_ALIGN_CENTER:
    py = y + (box_h - h) * 0.5f;
    break;
  case CHEESE_ALIGN_END:
    py = y + box_h - h - margin;
    break;
  default:
    py = y + margin;
    break;
  }

  f32 baseline = py + pad + asc;

  cheese_draw_rect(cheese, (cheese_corners_t){4.0f, 4.0f, 4.0f, 4.0f}, px, py,
                   w, h, cheese_color_rgba(0, 0, 0, 51));

  cheese_color_t text_color = cheese_color_rgba(230, 230, 230, 255);
  for (u32 i = 0; i < count; i++)
    cheese_draw_text(cheese, px + pad, baseline + (f32)i * line_h, rows[i],
                     font, text_color, 1.0f);
}
