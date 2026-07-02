#include <cheese/container.h>
#include <cheese/draw.h>
#include <cheese/layout.h>
#include <cheese/style.h>
#include <cheese/types.h>

void cheese_begin_container(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h) {
  cheese_style_t *style = cheese_current_style(cheese);
  cheese_push_style(cheese, *style);

  f32 pad_left = cheese_style_get_pad_left(style);
  f32 pad_down = cheese_style_get_pad_down(style);
  f32 pad_up = cheese_style_get_pad_up(style);
  f32 pad_right = cheese_style_get_pad_right(style);

  cheese_draw_rect(cheese, x, y, w, h, style->bg_color);

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

  cheese_push_clip(cheese, x + pad_left, y + pad_up, w - pad_left - pad_right,
                   h - pad_up - pad_down);

  cheese_layout_t layout = {0};
  layout.x = x + pad_left;
  layout.y = y + pad_up;
  layout.width = w - pad_left - pad_right;
  layout.horizontal = true;
  layout.spacing = 4.0f;

  cheese_push_layout(cheese, layout);
}

void cheese_begin_container_auto(cheese_t *cheese, f32 w, f32 h) {
  cheese_layout_t *layout = cheese_current_layout(cheese);
  cheese_style_t *style = cheese_current_style(cheese);

  f32 margin_left = cheese_style_get_margin_left(style);
  f32 margin_down = cheese_style_get_margin_down(style);
  f32 margin_up = cheese_style_get_margin_up(style);
  f32 margin_right = cheese_style_get_margin_right(style);

  f32 x = layout->x + margin_left;
  f32 y = layout->y + margin_up;
  f32 inner_w = w - margin_left - margin_right;
  f32 inner_h = h - margin_down - margin_up;

  cheese_advance_cursor(cheese, w, h);

  cheese_begin_container(cheese, x, y, inner_w, inner_h);
}

void cheese_end_container(cheese_t *cheese) {
  cheese_pop_layout(cheese);
  cheese_pop_clip(cheese);
  cheese_pop_style(cheese);
}
