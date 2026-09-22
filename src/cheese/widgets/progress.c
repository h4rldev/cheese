/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>

#include <cheese/style/prop.h>
#include <cheese/style/resolve.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/widgets/progress.h>

/***********************************/

static u32 progress_prop_track_opacity;
static u32 progress_prop_fill_opacity;
static u32 progress_prop_fill_gradient;

static void cheese_progress_props(cheese_t *cheese) {
  if (progress_prop_track_opacity)
    return;

  progress_prop_track_opacity = cheese_style_prop_register(
      cheese, "progress/track/opacity", CHEESE_PROP_F32);
  progress_prop_fill_opacity = cheese_style_prop_register(
      cheese, "progress/fill/opacity", CHEESE_PROP_F32);
  progress_prop_fill_gradient = cheese_style_prop_register(
      cheese, "progress/fill/gradient", CHEESE_PROP_PTR);
}

//
//
//

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

  cheese_progress_props(cheese);

  cheese_color_t track_color = style.bg_color;
  if (track_color == 0)
    track_color = cheese_color_rgba(40, 40, 40, 255);

  cheese_color_t fill_color = style.text_color;
  if (fill_color == 0)
    fill_color = cheese_color_rgba(255, 255, 255, 255);

  f32 whole =
      cheese_style_prop_get_f32(&style, cheese->core_props.opacity, 1.0f);
  f32 track_alpha =
      cheese_style_prop_get_f32(&style, progress_prop_track_opacity, whole);
  f32 fill_alpha =
      cheese_style_prop_get_f32(&style, progress_prop_fill_opacity, whole);

  cheese_draw_bg(cheese, &style, x, y, w, h, track_color, 0, track_alpha);

  if (progress > 0.0f)
    cheese_draw_fill(cheese, &style, x, y, w * progress, h, fill_color,
                     progress_prop_fill_gradient, 0, fill_alpha);

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

void cheese_progress_set_fill_gradient(cheese_t *cheese, cheese_style_t *style,
                                       cheese_gradient_t gradient) {
  if (!cheese || !style)
    return;

  cheese_progress_props(cheese);
  cheese_style_prop_set_gradient(cheese, style, progress_prop_fill_gradient,
                                 gradient);
}
