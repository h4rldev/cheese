/***********************************/

#include <math.h>

#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/anim.h>

/***********************************/

void cheese_anim_f32_to(cheese_anim_f32_t *anim, f32 target) {
  if (anim)
    anim->target = target;
}

b32 cheese_anim_f32_update(cheese_anim_f32_t *anim, f32 dt) {
  if (!anim || anim->speed <= 0.0f)
    return false;

  f32 delta = anim->target - anim->current;
  if (fabsf(delta) < 0.0001f) {
    anim->current = anim->target;
    return false;
  }

  anim->current += delta * (1.0f - expf(-anim->speed * dt));

  if (fabsf(anim->target - anim->current) < 0.0001f) {
    anim->current = anim->target;
    return false;
  }
  return true;
}

cheese_color_t cheese_color_lerp(cheese_color_t from, cheese_color_t to,
                                 f32 t) {
  if (t <= 0.0f)
    return from;
  if (t >= 1.0f)
    return to;

  u8 a0 = (from >> 24) & 0xFF, r0 = (from >> 16) & 0xFF,
     g0 = (from >> 8) & 0xFF, b0 = from & 0xFF;
  u8 a1 = (to >> 24) & 0xFF, r1 = (to >> 16) & 0xFF, g1 = (to >> 8) & 0xFF,
     b1 = to & 0xFF;

  u8 a = (u8)((f32)a0 + ((f32)a1 - (f32)a0) * t);
  u8 r = (u8)((f32)r0 + ((f32)r1 - (f32)r0) * t);
  u8 g = (u8)((f32)g0 + ((f32)g1 - (f32)g0) * t);
  u8 b = (u8)((f32)b0 + ((f32)b1 - (f32)b0) * t);

  return (a << 24) | (r << 16) | (g << 8) | b;
}
