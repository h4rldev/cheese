/***********************************/

#include <assert.h>

#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/anim.h>

/***********************************/

int main(void) {
  cheese_anim_f32_t a = {.current = 0.0f, .target = 1.0f, .speed = 10.0f};

  assert(cheese_anim_f32_update(&a, 0.016f) && "should be moving");
  assert(a.current > 0.0f && a.current < 1.0f && "eases, does not jump");

  for (int i = 0; i < 500; i++)
    cheese_anim_f32_update(&a, 0.016f);

  assert(a.current == 1.0f && "settles exactly at the target");
  assert(!cheese_anim_f32_update(&a, 0.016f) && "settled is not moving");

  cheese_anim_f32_to(&a, 0.5f);
  assert(a.target == 0.5f && "retargeting points the animation");
  assert(cheese_anim_f32_update(&a, 0.0f) &&
         "a zero step still reports motion toward a new target");
  assert(a.current == 1.0f && "a zero step does not move");
  cheese_anim_f32_to(&a, 0.0f);

  cheese_anim_f32_t still = {.current = 2.0f, .target = 2.0f, .speed = 10.0f};
  assert(!cheese_anim_f32_update(&still, 0.016f) &&
         "an at-target animation is not moving");

  cheese_anim_f32_t frozen = {.current = 0.0f, .target = 1.0f, .speed = 0.0f};
  assert(!cheese_anim_f32_update(&frozen, 0.016f) &&
         "a zero-speed animation never moves");
  assert(frozen.current == 0.0f);

  assert(!cheese_anim_f32_update(null, 0.016f) && "a null animation is safe");
  cheese_anim_f32_to(null, 1.0f);

  cheese_color_t mid =
      cheese_color_lerp(cheese_color_rgba(0, 0, 0, 255),
                        cheese_color_rgba(255, 255, 255, 255), 0.5f);
  u8 r = (mid >> 16) & 0xFF;
  assert((r == 127 || r == 128) && "midpoint lerp");

  assert(cheese_color_lerp(1, 2, 0.0f) == 1);
  assert(cheese_color_lerp(1, 2, 1.0f) == 2);

  return 0;
}
