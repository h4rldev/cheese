#ifndef CHEESE_BUTTER_RENDERER_H
#define CHEESE_BUTTER_RENDERER_H

#include <butter/types.h>
#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

typedef struct {
  butter_t *butter;

  butter_pipeline_t solid_pipeline;
  butter_pipeline_t textured_pipeline;
  butter_pipeline_t line_pipeline;

  vk_sampler_t default_sampler;

  vk_rect2d_t clip_stack[32];
  u32 clip_stack_depth;
} butter_renderer_t;

cheese_renderer_t cheese_create_butter_renderer(butter_t *butter,
                                                arena_t *arena);

void cheese_destroy_butter_renderer(cheese_renderer_t *renderer);

#endif // !CHEESE_BUTTER_RENDERER_H
