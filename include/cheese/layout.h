#ifndef CHEESE_LAYOUT_H
#define CHEESE_LAYOUT_H

#include <cheese/types.h>

cheese_layout_t cheese_layout_default(void);
cheese_layout_t *cheese_current_layout(cheese_t *cheese);
void cheese_push_layout(cheese_t *cheese, cheese_layout_t layout);
void cheese_pop_layout(cheese_t *cheese);

void cheese_advance_cursor(cheese_t *cheese, f32 w, f32 h);

#endif // !CHEESE_LAYOUT_H
