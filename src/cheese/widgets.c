#include <cheese/draw.h>
#include <cheese/font.h>
#include <cheese/layout.h>
#include <cheese/log.h>
#include <cheese/style.h>
#include <cheese/types.h>
#include <cheese/widgets.h>

static inline cheese_color_t cheese_lighten_color(cheese_color_t color,
                                                  f32 amount) {
  u8 r = (u8)((color >> 16) & 0xFF);
  u8 g = (u8)((color >> 8) & 0xFF);
  u8 b = (u8)((color >> 0) & 0xFF);
  u8 a = (u8)((color >> 24) & 0xFF);

  r = (u8)min(255, (f32)r * amount * 255.0f);
  g = (u8)min(255, (f32)g * amount * 255.0f);
  b = (u8)min(255, (f32)b * amount * 255.0f);

  return (a << 24) | (r << 16) | (g << 8) | b;
}

u32 cheese_button_ex(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                     const string *label, cheese_font_t *font,
                     cheese_color_t bg_color, cheese_color_t hover_color,
                     cheese_color_t text_color) {
  if (!cheese || !cheese->renderer) {
    cheese_log_error("Invalid cheese or renderer");
    return 0;
  }

  cheese_style_t *style = cheese_current_style(cheese);
  if (bg_color == 0)
    bg_color = style->bg_color;
  if (text_color == 0)
    text_color = style->text_color;

  if (hover_color == 0) {
    if (style->hover_color != 0)
      hover_color = style->hover_color;
    else
      hover_color = cheese_lighten_color(bg_color, 0.25f);
  }

  b32 hovered = (cheese->mouse_x >= x && cheese->mouse_x <= x + w &&
                 cheese->mouse_y >= y && cheese->mouse_y <= y + h);

  if (cheese->cursor_callback)
    cheese->cursor_callback(cheese->cursor_callback_userdata,
                            hovered ? CHEESE_CURSOR_POINTER
                                    : CHEESE_CURSOR_DEFAULT);

  u32 result = 0;
  if (hovered) {
    u32 clicked = cheese->mouse_buttons & ~cheese->mouse_prev_buttons;
    if (clicked & CHEESE_MOUSE_LEFT)
      result |= CHEESE_BUTTON_CLICK_LEFT;
    if (clicked & CHEESE_MOUSE_RIGHT)
      result |= CHEESE_BUTTON_CLICK_RIGHT;
    if (clicked & CHEESE_MOUSE_MIDDLE)
      result |= CHEESE_BUTTON_CLICK_MIDDLE;
  }

  cheese_color_t draw_bg = hovered ? hover_color : bg_color;
  cheese_draw_rect(cheese, x, y, w, h, draw_bg);

  if (style->border_width > 0.0f) {
    cheese_color_t border_color = style->border_color;
    f32 border_width = style->border_width;

    cheese_draw_line(cheese, x, y, x + w, y, border_width, border_color);
    cheese_draw_line(cheese, x, y + h, x + w, y + h, border_width,
                     border_color);
    cheese_draw_line(cheese, x, y, x, y + h, border_width, border_color);
    cheese_draw_line(cheese, x + w, y, x + w, y + h, border_width,
                     border_color);
  }

  if (label && label->len > 0 && font) {
    u32 target_size = style->font_size ? style->font_size : font->default_size;
    cheese_font_set_size(font, target_size);

    f32 text_w = cheese_font_measure_text(font, label);
    f32 text_h = font->active_variant->line_height;
    f32 center_x = x + (w - text_w) / 2.0f;
    f32 center_y = y + (h - text_h) / 2.0f + (text_h * 0.75f);

    cheese_draw_text(cheese, center_x, center_y, label, font, text_color, 1.0f);
  }

  return result;
}

b32 cheese_button(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                  const string *label, cheese_font_t *font,
                  cheese_color_t bg_color, cheese_color_t hover_color,
                  cheese_color_t text_color) {

  return (cheese_button_ex(cheese, x, y, w, h, label, font, bg_color,
                           hover_color, text_color) &
          CHEESE_BUTTON_CLICK_LEFT) != 0;
}

b32 cheese_button_auto(cheese_t *cheese, const string *label,
                       cheese_font_t *font) {
  cheese_layout_t *layout = cheese_current_layout(cheese);
  cheese_style_t *style = cheese_current_style(cheese);

  u32 target_size = style->font_size ? style->font_size : font->default_size;

  cheese_font_set_size(font, target_size);

  f32 text_w = cheese_font_measure_text(font, label);
  f32 text_h = font->active_variant->line_height;

  f32 pad_w =
      cheese_style_get_pad_left(style) + cheese_style_get_pad_right(style);
  f32 pad_h = cheese_style_get_pad_up(style) + cheese_style_get_pad_down(style);

  f32 bw = text_w + (pad_w > 0.0f ? pad_w : 20.0f);
  f32 bh = text_h + (pad_h > 0.0f ? pad_h : 10.0f);

  f32 x = layout->x;
  f32 y = layout->y;

  b32 result = cheese_button(cheese, x, y, bw, bh, label, font, 0, 0, 0);
  cheese_advance_cursor(cheese, bw, bh);
  return result;
}
