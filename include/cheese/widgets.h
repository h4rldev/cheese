#ifndef CHEESE_WIDGETS_H
#define CHEESE_WIDGETS_H

#include <cheese/style.h>
#include <cheese/types.h>

b32 cheese_button(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                  const string *label, cheese_font_t *font,
                  cheese_color_t bg_color, cheese_color_t hover_color,
                  cheese_color_t text_color);
b32 cheese_button_auto(cheese_t *cheese, const string *label,
                       cheese_font_t *font);
u32 cheese_button_ex(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                     const string *label, cheese_font_t *font,
                     cheese_color_t bg_color, cheese_color_t hover_color,
                     cheese_color_t text_color);

#endif // !CHEESE_WIDGETS_H
