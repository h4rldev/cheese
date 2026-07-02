#ifndef CHEESE_INIT_H
#define CHEESE_INIT_H

#include <cheese/types.h>

cheese_t cheese_default(void);
void cheese_begin(cheese_t *cheese, cheese_renderer_t *renderer, f32 mouse_x,
                  f32 mouse_y, u32 mouse_buttons, f32 scroll_x, f32 scroll_y,
                  u32 key_mods, f32 delta_time);

void cheese_end(cheese_t *cheese);

#endif // !CHEESE_INIT_H
