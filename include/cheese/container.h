#ifndef CHEESE_CONTAINER_H
#define CHEESE_CONTAINER_H

#include <cheese/types.h>

void cheese_begin_container(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h);

void cheese_begin_container_auto(cheese_t *cheese, f32 w, f32 h);
void cheese_end_container(cheese_t *cheese);

#endif // !CHEESE_CONTAINER_H
