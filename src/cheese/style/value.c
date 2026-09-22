/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/style/internal.h>
#include <cheese/style/value.h>

/***********************************/

static inline void warn_if_color_will_get_inherited(const cstr *scope) {
  cheese_log_warning("%s: Color set to 0 will be overwritten by inheritance",
                     scope);
}

//
//
//

static inline void warn_if_float_will_get_inherited(const cstr *scope) {
  cheese_log_warning("%s: Negative floats will be overwritten by inheritance",
                     scope);
}

//
//
//

void cheese_style_merge(arena_t *arena, cheese_style_t *dest,
                        const cheese_style_t *src) {
  if (dest->bg_color == 0)
    dest->bg_color = src->bg_color;
  if (dest->hover_color == 0)
    dest->hover_color = src->hover_color;
  if (dest->pressed_color == 0)
    dest->pressed_color = src->pressed_color;
  if (dest->disabled_color == 0)
    dest->disabled_color = src->disabled_color;
  if (dest->focus_color == 0)
    dest->focus_color = src->focus_color;
  if (dest->text_color == 0)
    dest->text_color = src->text_color;
  if (dest->state_layer_color == 0)
    dest->state_layer_color = src->state_layer_color;

  if (dest->corner_radius.top_left < 0.0f)
    dest->corner_radius.top_left = src->corner_radius.top_left;
  if (dest->corner_radius.top_right < 0.0f)
    dest->corner_radius.top_right = src->corner_radius.top_right;
  if (dest->corner_radius.bottom_right < 0.0f)
    dest->corner_radius.bottom_right = src->corner_radius.bottom_right;
  if (dest->corner_radius.bottom_left < 0.0f)
    dest->corner_radius.bottom_left = src->corner_radius.bottom_left;

  if (dest->padding.left < 0.0f)
    dest->padding.left = src->padding.left;
  if (dest->padding.bottom < 0.0f)
    dest->padding.bottom = src->padding.bottom;
  if (dest->padding.top < 0.0f)
    dest->padding.top = src->padding.top;
  if (dest->padding.right < 0.0f)
    dest->padding.right = src->padding.right;

  if (dest->margin.left < 0.0f)
    dest->margin.left = src->margin.left;
  if (dest->margin.right < 0.0f)
    dest->margin.right = src->margin.right;
  if (dest->margin.top < 0.0f)
    dest->margin.top = src->margin.top;
  if (dest->margin.bottom < 0.0f)
    dest->margin.bottom = src->margin.bottom;

  if (dest->font_size == 0)
    dest->font_size = src->font_size;
  if (dest->cursor < 0)
    dest->cursor = src->cursor;
  if (dest->selectable < 0)
    dest->selectable = src->selectable;

  for (u32 i = 0; i < src->prop_count; i++) {
    const cheese_prop_t *prop = &src->props[i];
    if (cheese_style_prop_find(dest, prop->id))
      continue;

    cheese_style_prop_put(arena, dest, *prop);
  }
}

void cheese_style_override(arena_t *arena, cheese_style_t *dest,
                           const cheese_style_t *src) {
  if (src->bg_color)
    dest->bg_color = src->bg_color;
  if (src->hover_color)
    dest->hover_color = src->hover_color;
  if (src->pressed_color)
    dest->pressed_color = src->pressed_color;
  if (src->disabled_color)
    dest->disabled_color = src->disabled_color;
  if (src->focus_color)
    dest->focus_color = src->focus_color;
  if (src->text_color)
    dest->text_color = src->text_color;
  if (src->state_layer_color)
    dest->state_layer_color = src->state_layer_color;

  if (src->corner_radius.top_left >= 0.0f)
    dest->corner_radius.top_left = src->corner_radius.top_left;
  if (src->corner_radius.top_right >= 0.0f)
    dest->corner_radius.top_right = src->corner_radius.top_right;
  if (src->corner_radius.bottom_right >= 0.0f)
    dest->corner_radius.bottom_right = src->corner_radius.bottom_right;
  if (src->corner_radius.bottom_left >= 0.0f)
    dest->corner_radius.bottom_left = src->corner_radius.bottom_left;

  if (src->padding.left >= 0.0f)
    dest->padding.left = src->padding.left;
  if (src->padding.right >= 0.0f)
    dest->padding.right = src->padding.right;
  if (src->padding.top >= 0.0f)
    dest->padding.top = src->padding.top;
  if (src->padding.bottom >= 0.0f)
    dest->padding.bottom = src->padding.bottom;

  if (src->margin.left >= 0.0f)
    dest->margin.left = src->margin.left;
  if (src->margin.right >= 0.0f)
    dest->margin.right = src->margin.right;
  if (src->margin.top >= 0.0f)
    dest->margin.top = src->margin.top;
  if (src->margin.bottom >= 0.0f)
    dest->margin.bottom = src->margin.bottom;

  if (src->font_size)
    dest->font_size = src->font_size;

  if (src->cursor >= 0)
    dest->cursor = src->cursor;

  if (src->selectable >= 0)
    dest->selectable = src->selectable;

  for (u32 i = 0; i < src->prop_count; i++)
    cheese_style_prop_put(arena, dest, src->props[i]);
}

cheese_style_t cheese_style_new(void) {
  return (cheese_style_t){
      .bg_color = 0,
      .hover_color = 0,
      .pressed_color = 0,
      .disabled_color = 0,
      .focus_color = 0,
      .text_color = 0,
      .state_layer_color = 0,

      .corner_radius = {-1.0f, -1.0f, -1.0f, -1.0f},
      .padding = {-1.0f, -1.0f, -1.0f, -1.0f},
      .margin = {-1.0f, -1.0f, -1.0f, -1.0f},

      .font_size = 0,

      .cursor = -1,
      .selectable = -1,

      .props = null,
      .prop_count = 0,
  };
}

cheese_style_t *cheese_current_style(cheese_t *cheese) {
  if (cheese->style_dirty) {
    cheese_style_t resolved = cheese_style_new();
    for (u32 i = cheese->scope_depth; i-- > 0;)
      cheese_style_merge(cheese->frame_arena, &resolved,
                         &cheese->scope_stack[i].style);

    cheese->resolved_style = resolved;
    cheese->style_dirty = false;
  }

  return &cheese->resolved_style;
}

f32 cheese_style_state_layer_alpha(u32 state) {
  if (state & CHEESE_STATE_DISABLED)
    return 0.12f;
  if (state & CHEESE_STATE_PRESSED)
    return 0.12f;
  if (state & CHEESE_STATE_HOVERED)
    return 0.08f;
  if (state & CHEESE_STATE_FOCUSED)
    return 0.10f;

  return 0.0f;
}

void cheese_style_set_bg_color(cheese_style_t *style, cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_bg_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_bg_color");

  style->bg_color = color;
}

void cheese_style_set_hover_color(cheese_style_t *style, cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_hover_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_hover_color");

  style->hover_color = color;
}

void cheese_style_set_pressed_color(cheese_style_t *style,
                                    cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_pressed_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_pressed_color");

  style->pressed_color = color;
}

void cheese_style_set_disabled_color(cheese_style_t *style,
                                     cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_disabled_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_disabled_color");

  style->disabled_color = color;
}

void cheese_style_set_focus_color(cheese_style_t *style, cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_focus_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_focus_color");

  style->focus_color = color;
}

void cheese_style_set_text_color(cheese_style_t *style, cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_text_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_text_color");

  style->text_color = color;
}

void cheese_style_set_state_layer_color(cheese_style_t *style,
                                        cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_state_layer_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_state_layer_color");

  style->state_layer_color = color;
}

void cheese_style_set_corner_radius_inherit(cheese_style_t *style) {
  if (!style) {
    cheese_log_error("cheese_style_set_corner_radius_inherit: Invalid style");
    return;
  }

  style->corner_radius = (cheese_corners_t){-1.0f, -1.0f, -1.0f, -1.0f};
}

void cheese_style_set_corner_radius_uniform(cheese_style_t *style, f32 radius) {
  if (!style) {
    cheese_log_error("cheese_style_set_corner_radius_uniform: Invalid style");
    return;
  }

  if (radius < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_corner_radius_uniform");

  style->corner_radius.top_left = radius;
  style->corner_radius.top_right = radius;
  style->corner_radius.bottom_left = radius;
  style->corner_radius.bottom_right = radius;
}

void cheese_style_set_corner_radius(cheese_style_t *style, f32 top_left,
                                    f32 top_right, f32 bottom_left,
                                    f32 bottom_right) {
  if (!style) {
    cheese_log_error("cheese_style_set_corner_radius: Invalid style");
    return;
  }

  if (top_left < 0.0f || top_right < 0.0f || bottom_left < 0.0f ||
      bottom_right < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_corner_radius");

  style->corner_radius.top_left = top_left;
  style->corner_radius.top_right = top_right;
  style->corner_radius.bottom_left = bottom_left;
  style->corner_radius.bottom_right = bottom_right;
}

void cheese_style_set_padding_inherit(cheese_style_t *style) {
  if (!style) {
    cheese_log_error("cheese_style_set_padding_inherit: Invalid style");
    return;
  }

  style->padding = (cheese_edges_t){-1.0f, -1.0f, -1.0f, -1.0f};
}

void cheese_style_set_padding_uniform(cheese_style_t *style, f32 padding) {
  if (!style) {
    cheese_log_error("cheese_style_set_padding_uniform: Invalid style");
    return;
  }

  if (padding < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_padding_uniform");

  style->padding.left = padding;
  style->padding.right = padding;
  style->padding.top = padding;
  style->padding.bottom = padding;
}

void cheese_style_set_padding(cheese_style_t *style, f32 padding_left,
                              f32 padding_bottom, f32 padding_top,
                              f32 padding_right) {
  if (!style) {
    cheese_log_error("cheese_style_set_padding: Invalid style");
    return;
  }

  if (padding_left < 0.0f || padding_bottom < 0.0f || padding_top < 0.0f ||
      padding_right < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_padding");

  style->padding.left = padding_left;
  style->padding.right = padding_right;
  style->padding.top = padding_top;
  style->padding.bottom = padding_bottom;
}

void cheese_style_set_margin_inherit(cheese_style_t *style) {
  if (!style) {
    cheese_log_error("cheese_style_set_margin_inherit: Invalid style");
    return;
  }

  style->margin = (cheese_edges_t){-1.0f, -1.0f, -1.0f, -1.0f};
}

void cheese_style_set_margin_uniform(cheese_style_t *style, f32 margin) {
  if (!style) {
    cheese_log_error("cheese_style_set_margin_uniform: Invalid style");
    return;
  }

  if (margin < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_margin_uniform");

  style->margin.left = margin;
  style->margin.right = margin;
  style->margin.top = margin;
  style->margin.bottom = margin;
}

void cheese_style_set_margin(cheese_style_t *style, f32 margin_left,
                             f32 margin_bottom, f32 margin_top,
                             f32 margin_right) {
  if (!style) {
    cheese_log_error("cheese_style_set_margin: Invalid style");
    return;
  }

  if (margin_left < 0.0f || margin_bottom < 0.0f || margin_top < 0.0f ||
      margin_right < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_margin");

  style->margin.left = margin_left;
  style->margin.bottom = margin_bottom;
  style->margin.top = margin_top;
  style->margin.right = margin_right;
}

void cheese_style_set_cursor(cheese_style_t *style, cheese_cursor_t cursor) {
  if (!style)
    return;

  if (cursor < CHEESE_CURSOR_DEFAULT || cursor >= CHEESE_CURSOR_MAX)
    cheese_log_warning("cheese_style_set_cursor: Invalid cursor");

  style->cursor = (i32)cursor;
}

void cheese_style_set_cursor_inherit(cheese_style_t *style) {
  if (!style)
    return;

  style->cursor = -1;
}

void cheese_style_set_selectable(cheese_style_t *style, b32 selectable) {
  if (!style)
    return;

  style->selectable = selectable ? 1 : 0;
}

void cheese_style_set_selectable_inherit(cheese_style_t *style) {
  if (!style)
    return;

  style->selectable = -1;
}

void cheese_style_set_font_size(cheese_style_t *style, u32 font_size) {
  if (!style) {
    cheese_log_error("cheese_style_set_font_size: Invalid style");
    return;
  }

  if (font_size == 0)
    cheese_log_warning("cheese_style_set_font_size: A font-size of 0 will be "
                       "overwritten by inheritance");

  style->font_size = font_size;
}
