/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/widgets/button.h>
#include <cheese/widgets/widget.h>

/***********************************/

u32 cheese_button_ex(cheese_t *cheese, const cstr *classes,
                     cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                     cheese_value_t label, cheese_font_t *font,
                     cheese_color_t bg_color, cheese_color_t hover_color,
                     cheese_color_t text_color) {
  if (!cheese || !cheese->renderer) {
    cheese_log_error("Invalid cheese or renderer");
    return 0;
  }

  cheese_widget_t scope;
  cheese_widget_begin(cheese, semantics, CHEESE_ROLE_BUTTON, x, y, w, h,
                      &scope);

  u32 state = scope.state;
  if (scope.pressed)
    state |= CHEESE_STATE_PRESSED;

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_BUTTON, classes,
                              null);
  cheese_style_apply_state(&style, state);

  if (scope.hovered)
    cheese_cursor_request(cheese, style.cursor >= 0
                                      ? (cheese_cursor_t)style.cursor
                                      : CHEESE_CURSOR_POINTER);

  if (text_color == 0)
    text_color = style.text_color;

  cheese_color_t draw_bg = bg_color != 0 ? bg_color : style.bg_color;
  if (scope.hovered && hover_color != 0)
    draw_bg = hover_color;

  u32 result = 0;
  if (scope.hovered) {
    u32 clicked = cheese->mouse_buttons & ~cheese->mouse_prev_buttons;
    if (clicked & CHEESE_MOUSE_RIGHT)
      result |= CHEESE_BUTTON_CLICK_RIGHT;
    if (clicked & CHEESE_MOUSE_MIDDLE)
      result |= CHEESE_BUTTON_CLICK_MIDDLE;
  }

  if (cheese_widget_activated(cheese, &scope))
    result |= CHEESE_BUTTON_CLICK_LEFT;

  f32 alpha =
      cheese_style_get_prop_f32(&style, cheese->core_props.opacity, 1.0f);
  cheese_draw_bg(cheese, &style, x, y, w, h, draw_bg, state, alpha);
  cheese_draw_border(cheese, &style, x, y, w, h);

  const cstr *label_cstr = cheese_value_str(label);
  cheese_widget_emit(cheese, semantics, CHEESE_ROLE_BUTTON, label_cstr, null,
                     state, &scope);

  if (label_cstr[0] != '\0' && font) {
    string *label_str = string_from_cstr(cheese->frame_arena, label_cstr);

    u32 target_size = style.font_size ? style.font_size : font->default_size;
    cheese_font_set_size(font, target_size);

    f32 text_w = cheese_font_measure_text(font, label_str);
    f32 text_h = font->active_variant->line_height;
    f32 center_x = x + (w - text_w) / 2.0f;
    f32 center_y = y + (h - text_h) / 2.0f + (text_h * 0.75f);

    cheese_draw_text(cheese, center_x, center_y, label_str, font, text_color,
                     1.0f);
  }

  if (scope.focused)
    cheese_draw_focus_ring(cheese, &style, x, y, w, h);

  return result;
}

b32 cheese_button(cheese_t *cheese, const cstr *classes,
                  cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                  cheese_value_t label, cheese_font_t *font,
                  cheese_color_t bg_color, cheese_color_t hover_color,
                  cheese_color_t text_color) {
  return (cheese_button_ex(cheese, classes, semantics, x, y, w, h, label, font,
                           bg_color, hover_color, text_color) &
          CHEESE_BUTTON_CLICK_LEFT) != 0;
}

b32 cheese_button_auto(cheese_t *cheese, const cstr *classes,
                       cheese_semantics_t semantics, cheese_value_t label,
                       cheese_font_t *font) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_BUTTON, classes,
                              null);

  const cstr *label_cstr = cheese_value_str(label);
  string *label_str = string_from_cstr(cheese->frame_arena, label_cstr);

  u32 target_size = style.font_size ? style.font_size : font->default_size;
  cheese_font_set_size(font, target_size);

  f32 text_w =
      label_cstr[0] != '\0' ? cheese_font_measure_text(font, label_str) : 0.0f;
  f32 text_h = font->active_variant->line_height;

  f32 pad_w =
      cheese_style_get_pad_left(&style) + cheese_style_get_pad_right(&style);
  f32 pad_h =
      cheese_style_get_pad_top(&style) + cheese_style_get_pad_bottom(&style);

  f32 bw = text_w + (pad_w > 0.0f ? pad_w : 20.0f);
  f32 bh = text_h + (pad_h > 0.0f ? pad_h : 10.0f);

  f32 x, y;
  cheese_layout_place(cheese, &bw, &bh, &x, &y);

  return cheese_button(cheese, classes, semantics, x, y, bw, bh, label, font, 0,
                       0, 0);
}
