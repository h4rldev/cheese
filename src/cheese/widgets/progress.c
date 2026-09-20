/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/widgets/progress.h>

/***********************************/

void cheese_progress_bar(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, f32 x, f32 y, f32 w,
                         f32 h, cheese_value_t value) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_PROGRESS_BAR, classes,
                              null);

  f32 progress = cheese_value_f32(value);
  progress = min(1.0f, max(0.0f, progress));

  const cstr *value_str =
      cheese_semantics_format(cheese, "%d%%", (i32)(progress * 100.0f + 0.5f));

  cheese_semantics_emit(cheese, semantics, CHEESE_ROLE_PROGRESS_BAR, null,
                        value_str, 0,
                        (cheese_rect_t){(i32)x, (i32)y, (u32)w, (u32)h});

  cheese_color_t track_color = style.bg_color;
  if (track_color == 0)
    track_color = cheese_color_rgba(40, 40, 40, 255);

  cheese_color_t fill_color = style.text_color;
  if (fill_color == 0)
    fill_color = cheese_color_rgba(255, 255, 255, 255);

  cheese_draw_rect(cheese, style.corner_radius, x, y, w, h, track_color);

  if (progress > 0.0f)
    cheese_draw_rect(cheese, style.corner_radius, x, y, w * progress, h,
                     fill_color);

  cheese_draw_border(cheese, &style, x, y, w, h);
}

void cheese_progress_bar_auto(cheese_t *cheese, const cstr *classes,
                              cheese_semantics_t semantics,
                              cheese_value_t value) {
  cheese_layout_t *layout = cheese_current_layout(cheese);

  f32 w = layout->width > 0.0f ? layout->width : 100.0f;
  f32 h = 8.0f;

  f32 x, y;
  cheese_layout_place(cheese, &w, &h, &x, &y);

  cheese_progress_bar(cheese, classes, semantics, x, y, w, h, value);
}
