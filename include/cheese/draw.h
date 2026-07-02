#ifndef CHEESE_DRAW_H
#define CHEESE_DRAW_H

#include <cheese/font.h>
#include <cheese/types.h>

void cheese_draw_rect(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                      cheese_color_t color);
void cheese_draw_texture(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                         u32 texture_id, cheese_color_t color);
void cheese_draw_line(cheese_t *cheese, f32 x1, f32 y1, f32 x2, f32 y2,
                      f32 thickness, cheese_color_t color);
void cheese_draw_arc(cheese_t *cheese, f32 cx, f32 cy, f32 radius,
                     f32 start_angle, f32 end_angle, f32 thickness,
                     cheese_color_t color);
void cheese_draw_text(cheese_t *cheese, f32 x, f32 y, const string *text,
                      cheese_font_t *font, cheese_color_t color, f32 scale);

void cheese_push_clip(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h);
void cheese_pop_clip(cheese_t *cheese);

#endif // !CHEESE_DRAW_H
