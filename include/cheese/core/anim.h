#ifndef CHEESE_CORE_ANIM_H
#define CHEESE_CORE_ANIM_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Point an f32 animation at a new target.
 *
 * @param anim The animation.
 * @param target The value to approach.
 */
void cheese_anim_f32_to(cheese_anim_f32_t *anim, f32 target);

//
//
//

/**
 * @brief Advance an f32 animation.
 * @details Moves @c current toward @c target by an exponential ease over
 * @c dt seconds, snapping when close.
 *
 * @param anim The animation.
 * @param dt Seconds since the last update.
 *
 * @return True while still moving, false once settled.
 */
b32 cheese_anim_f32_update(cheese_anim_f32_t *anim, f32 dt);

//
//
//

/**
 * @brief Linearly interpolate between two colours, alpha included.
 *
 * @param from The colour at @c t <= 0.
 * @param to The colour at @c t >= 1.
 * @param t The interpolation factor, clamped to `0..1`.
 *
 * @return The interpolated colour.
 */
cheese_color_t cheese_color_lerp(cheese_color_t from, cheese_color_t to, f32 t);

#endif // !CHEESE_CORE_ANIM_H
