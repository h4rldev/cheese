/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>

#include <cheese/style/resolve.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/widgets/toggle.h>
#include <cheese/widgets/widget.h>

/***********************************/

#define CHEESE_TOGGLE_GAP 8.0f

//
//
//

void cheese_toggle_begin(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, cheese_role_t role,
                         f32 x, f32 y, cheese_value_t label,
                         cheese_font_t *font, cheese_toggle_t *out) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, role, classes, null);

  f32 line_height = 16.0f;
  if (font) {
    u32 target_size = style.font_size ? style.font_size : font->default_size;
    cheese_font_set_size(font, target_size);
    line_height = font->active_variant->line_height;
  }

  const cstr *label_cstr = cheese_value_str(label);
  string *label_str = string_from_cstr(cheese->frame_arena, label_cstr);

  f32 indicator = line_height * 0.8f;
  f32 text_w = (font && label_cstr[0] != '\0')
                   ? cheese_font_measure_text(font, label_str)
                   : 0.0f;

  out->x = x;
  out->y = y;
  out->indicator = indicator;
  out->total_w =
      indicator + (text_w > 0.0f ? text_w + CHEESE_TOGGLE_GAP : 0.0f);
  out->line_height = line_height;
  out->label = label_cstr;
  out->label_str = label_str;

  cheese_widget_t scope;
  cheese_widget_begin(cheese, semantics, role, x, y, out->total_w, indicator,
                      &scope);

  out->hovered = scope.hovered;
  out->focused = scope.focused;
  out->activated = cheese_widget_activated(cheese, &scope);
  out->state = scope.state;

  if (scope.hovered)
    cheese_cursor_request(cheese, style.cursor >= 0
                                      ? (cheese_cursor_t)style.cursor
                                      : CHEESE_CURSOR_POINTER);
}

i32 cheese_toggle_emit(cheese_t *cheese, cheese_semantics_t semantics,
                       const cheese_toggle_t *toggle, cheese_role_t role,
                       b32 chosen) {
  cheese_widget_t scope = {
      .x = toggle->x,
      .y = toggle->y,
      .w = toggle->total_w,
      .h = toggle->indicator,
      .state = toggle->state,
  };

  return cheese_widget_emit(cheese, semantics, role, toggle->label, null,
                            chosen ? CHEESE_STATE_CHECKED : 0, &scope);
}

void cheese_toggle_label(cheese_t *cheese, const cheese_toggle_t *toggle,
                         cheese_font_t *font, cheese_color_t color) {
  if (!font || !toggle->label_str || toggle->label_str->len == 0)
    return;

  f32 text_x = toggle->x + toggle->indicator + CHEESE_TOGGLE_GAP;
  f32 text_y = toggle->y + (toggle->indicator - toggle->line_height) * 0.5f +
               toggle->line_height * 0.75f;

  cheese_draw_text(cheese, text_x, text_y, toggle->label_str, font, color,
                   1.0f);
}

b32 cheese_checkbox(cheese_t *cheese, const cstr *classes,
                    cheese_semantics_t semantics, f32 x, f32 y,
                    cheese_value_t checked, cheese_value_t label,
                    cheese_font_t *font) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_CHECKBOX, classes,
                              null);

  b32 value = cheese_value_b32(checked);
  b32 result = value;

  cheese_toggle_t toggle;
  cheese_toggle_begin(cheese, classes, semantics, CHEESE_ROLE_CHECKBOX, x, y,
                      label, font, &toggle);

  if (toggle.activated)
    result = !value;

  if (result != value)
    cheese_value_set_b32(checked, result);

  cheese_toggle_emit(cheese, semantics, &toggle, CHEESE_ROLE_CHECKBOX, result);

  f32 box = toggle.indicator;

  cheese_color_t box_color = style.bg_color;
  if (box_color == 0)
    box_color = cheese_color_rgba(255, 255, 255, 255);

  cheese_draw_rect(cheese, style.corner_radius, x, y, box, box, box_color);
  cheese_draw_border(cheese, &style, x, y, box, box);

  if (result) {
    cheese_color_t check_color = style.text_color;
    if (check_color == 0)
      check_color = 0x000000FF;

    string *check_str = string_from_cstr(cheese->frame_arena, "\u2713");
    f32 check_w = cheese_font_measure_text(font, check_str);
    f32 check_h = font->active_variant->line_height;
    f32 check_x = x + (box - check_w) / 2.0f;
    f32 check_y = y + (box - check_h) / 2.0f + check_h * 0.75f;

    cheese_draw_text(cheese, check_x, check_y, check_str, font, check_color,
                     1.0f);
  }

  cheese_color_t text_color = style.text_color;
  if (text_color == 0)
    text_color = cheese_color_rgba(0, 0, 0, 255);

  cheese_toggle_label(cheese, &toggle, font, text_color);

  if (toggle.focused)
    cheese_draw_focus_ring(cheese, &style, x, y, toggle.total_w,
                           toggle.indicator);

  return result;
}

b32 cheese_checkbox_auto(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, cheese_value_t checked,
                         cheese_value_t label, cheese_font_t *font) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_CHECKBOX, classes,
                              null);

  u32 target_size = style.font_size ? style.font_size : font->default_size;
  cheese_font_set_size(font, target_size);

  const cstr *label_cstr = cheese_value_str(label);
  string *label_str = string_from_cstr(cheese->frame_arena, label_cstr);

  f32 line_height = font->active_variant->line_height;
  f32 box_size = line_height * 0.8f;

  f32 text_w =
      label_cstr[0] != '\0' ? cheese_font_measure_text(font, label_str) : 0.0f;
  f32 spacing = 0.0f;

  f32 total_w = box_size + (text_w > 0 ? text_w + spacing : 0.0f);
  f32 total_h = box_size;

  f32 x, y;
  cheese_layout_place(cheese, &total_w, &total_h, &x, &y);

  return cheese_checkbox(cheese, classes, semantics, x, y, checked, label,
                         font);
}

b32 cheese_radio(cheese_t *cheese, const cstr *classes,
                 cheese_semantics_t semantics, f32 x, f32 y,
                 cheese_value_t selected, i32 index, cheese_value_t label,
                 cheese_font_t *font) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_RADIO, classes, null);

  i32 current = cheese_value_i32(selected);
  b32 chosen = current == index;

  cheese_toggle_t toggle;
  cheese_toggle_begin(cheese, classes, semantics, CHEESE_ROLE_RADIO, x, y,
                      label, font, &toggle);

  if (toggle.activated) {
    cheese_value_set_i32(selected, index);
    chosen = true;
  }

  cheese_toggle_emit(cheese, semantics, &toggle, CHEESE_ROLE_RADIO, chosen);

  f32 dot = toggle.indicator;

  cheese_corners_t round = {dot * 0.5f, dot * 0.5f, dot * 0.5f, dot * 0.5f};

  cheese_color_t box_color = style.bg_color;
  if (box_color == 0)
    box_color = cheese_color_rgba(255, 255, 255, 255);

  cheese_draw_rect(cheese, round, x, y, dot, dot, box_color);

  cheese_color_t check_color = style.text_color;
  if (check_color == 0)
    check_color = 0x000000FF;

  if (chosen) {
    cheese_corners_t inner = {dot * 0.25f, dot * 0.25f, dot * 0.25f,
                              dot * 0.25f};
    cheese_draw_rect(cheese, inner, x + dot * 0.25f, y + dot * 0.25f,
                     dot * 0.5f, dot * 0.5f, check_color);
  }

  cheese_color_t text_color = style.text_color;
  if (text_color == 0)
    text_color = cheese_color_rgba(0, 0, 0, 255);

  cheese_toggle_label(cheese, &toggle, font, text_color);

  if (toggle.focused)
    cheese_draw_focus_ring(cheese, &style, x, y, toggle.total_w,
                           toggle.indicator);

  return chosen;
}

b32 cheese_radio_auto(cheese_t *cheese, const cstr *classes,
                      cheese_semantics_t semantics, cheese_value_t selected,
                      i32 index, cheese_value_t label, cheese_font_t *font) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_RADIO, classes, null);

  f32 line_height = 16.0f;
  if (font) {
    u32 target_size = style.font_size ? style.font_size : font->default_size;
    cheese_font_set_size(font, target_size);
    line_height = font->active_variant->line_height;
  }

  f32 dot = line_height * 0.8f;

  const cstr *label_cstr = cheese_value_str(label);
  string *label_str = string_from_cstr(cheese->frame_arena, label_cstr);
  f32 text_w = (font && label_cstr[0] != '\0')
                   ? cheese_font_measure_text(font, label_str)
                   : 0.0f;

  f32 total_w = dot + (text_w > 0 ? text_w : 0.0f);
  f32 total_h = dot;

  f32 x, y;
  cheese_layout_place(cheese, &total_w, &total_h, &x, &y);

  return cheese_radio(cheese, classes, semantics, x, y, selected, index, label,
                      font);
}
