/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>

#include <cheese/style/prop.h>
#include <cheese/style/resolve.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/widgets/slider.h>
#include <cheese/widgets/widget.h>

/***********************************/

static u32 slider_prop_track_color;
static u32 slider_prop_fill_color;
static u32 slider_prop_thumb_color;
static u32 slider_prop_thumb_radius;
static u32 slider_prop_track_opacity;
static u32 slider_prop_fill_opacity;
static u32 slider_prop_fill_gradient;

static void cheese_slider_props(cheese_t *cheese) {
  if (slider_prop_thumb_color)
    return;

  slider_prop_track_color = cheese_style_prop_register(
      cheese, "slider/track/color", CHEESE_PROP_COLOR);
  slider_prop_fill_color = cheese_style_prop_register(
      cheese, "slider/fill/color", CHEESE_PROP_COLOR);
  slider_prop_thumb_color = cheese_style_prop_register(
      cheese, "slider/thumb/color", CHEESE_PROP_COLOR);
  slider_prop_thumb_radius = cheese_style_prop_register(
      cheese, "slider/thumb/radius", CHEESE_PROP_F32);
  slider_prop_track_opacity = cheese_style_prop_register(
      cheese, "slider/track/opacity", CHEESE_PROP_F32);
  slider_prop_fill_opacity = cheese_style_prop_register(
      cheese, "slider/fill/opacity", CHEESE_PROP_F32);
  slider_prop_fill_gradient = cheese_style_prop_register(
      cheese, "slider/fill/gradient", CHEESE_PROP_PTR);
}

//
//
//

f32 cheese_slider_begin(cheese_t *cheese, const cheese_widget_t *scope,
                        cheese_value_t value, f32 step, b32 editing) {
  if (!cheese || !scope)
    return cheese_value_f32(value);

  f32 v = min(1.0f, max(0.0f, cheese_value_f32(value)));
  if (scope->hovered) {
    u32 clicked = cheese->mouse_buttons & ~cheese->mouse_prev_buttons;
    if (clicked & CHEESE_MOUSE_LEFT)
      cheese_capture(cheese, scope->id);
  }
  if (cheese_captured(cheese, scope->id)) {
    f32 tracked =
        (cheese->mouse_x - scope->x) / (scope->w > 0.0f ? scope->w : 1.0f);
    tracked = min(1.0f, max(0.0f, tracked));
    if (tracked != v) {
      v = tracked;
      cheese_value_set_f32(value, v);
    }
  }
  if (editing) {
    if (cheese_key_pressed(cheese, CHEESE_KEY_LEFT)) {
      v = max(0.0f, v - step);
      cheese_value_set_f32(value, v);
    }
    if (cheese_key_pressed(cheese, CHEESE_KEY_RIGHT)) {
      v = min(1.0f, v + step);
      cheese_value_set_f32(value, v);
    }
  }
  return v;
}

f32 cheese_slider(cheese_t *cheese, const cstr *classes,
                  cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                  cheese_value_t value) {
  if (!cheese || !cheese->renderer)
    return 0.0f;

  cheese_widget_t scope;
  cheese_widget_begin(cheese, semantics, CHEESE_ROLE_SLIDER, x, y, w, h,
                      &scope);

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_SLIDER, classes,
                              null);

  if (scope.hovered)
    cheese_cursor_request(cheese, style.cursor >= 0
                                      ? (cheese_cursor_t)style.cursor
                                      : CHEESE_CURSOR_POINTER);

  b32 editing = cheese->edit_id && cheese->edit_id == scope.id;
  u32 clicked = cheese->mouse_buttons & ~cheese->mouse_prev_buttons;

  if (scope.hovered && (clicked & CHEESE_MOUSE_LEFT)) {
    cheese->focus_id = scope.id;
    cheese->edit_id = scope.id;
    editing = true;
  }

  if (scope.focused && !editing && cheese_widget_activated(cheese, &scope)) {
    cheese->edit_id = scope.id;
    editing = true;
  }

  if (editing && cheese_key_pressed(cheese, CHEESE_KEY_ESCAPE)) {
    cheese->edit_id = 0;
    editing = false;
  }

  f32 v = cheese_slider_begin(cheese, &scope, value, 0.05f, editing);

  u32 extra = 0;
  if (cheese_captured(cheese, scope.id))
    extra |= CHEESE_STATE_ACTIVE | CHEESE_STATE_PRESSED;

  const cstr *value_str =
      cheese_semantics_format(cheese, "%d%%", (i32)(v * 100.0f + 0.5f));

  cheese_widget_emit(cheese, semantics, CHEESE_ROLE_SLIDER, null, value_str,
                     extra, &scope);

  cheese_slider_props(cheese);

  cheese_color_t track = cheese_style_prop_get_color(
      &style, slider_prop_track_color, style.bg_color);
  if (track == 0)
    track = cheese_color_rgba(40, 40, 40, 255);

  cheese_color_t fill = cheese_style_prop_get_color(
      &style, slider_prop_fill_color, style.text_color);
  if (fill == 0)
    fill = cheese_color_rgba(255, 255, 255, 255);

  cheese_color_t thumb =
      cheese_style_prop_get_color(&style, slider_prop_thumb_color, fill);

  f32 whole =
      cheese_style_prop_get_f32(&style, cheese->core_props.opacity, 1.0f);
  f32 track_alpha =
      cheese_style_prop_get_f32(&style, slider_prop_track_opacity, whole);
  f32 fill_alpha =
      cheese_style_prop_get_f32(&style, slider_prop_fill_opacity, whole);

  cheese_corners_t thumb_radius = style.corner_radius;
  f32 radius =
      cheese_style_prop_get_f32(&style, slider_prop_thumb_radius, -1.0f);
  if (radius >= 0.0f)
    thumb_radius = (cheese_corners_t){radius, radius, radius, radius};

  cheese_draw_bg(cheese, &style, x, y, w, h, track, 0, track_alpha);

  if (v > 0.0f)
    cheese_draw_fill(cheese, &style, x, y, w * v, h, fill,
                     slider_prop_fill_gradient, 0, fill_alpha);

  f32 knob = h * 1.6f;
  cheese_draw_rect(cheese, thumb_radius, x + w * v - knob * 0.5f,
                   y + (h - knob) * 0.5f, knob, knob, thumb);

  return v;
}

f32 cheese_slider_auto(cheese_t *cheese, const cstr *classes,
                       cheese_semantics_t semantics, cheese_value_t value) {
  cheese_layout_t *layout = cheese_current_layout(cheese);

  f32 w = layout->width > 0.0f ? layout->width : 100.0f;
  f32 h = 16.0f;

  f32 x, y;
  cheese_layout_place(cheese, &w, &h, &x, &y);

  return cheese_slider(cheese, classes, semantics, x, y, w, h, value);
}

void cheese_slider_set_fill_gradient(cheese_t *cheese, cheese_style_t *style,
                                     cheese_gradient_t gradient) {
  if (!cheese || !style)
    return;

  cheese_slider_props(cheese);
  cheese_style_prop_set_gradient(cheese, style, slider_prop_fill_gradient,
                                 gradient);
}
