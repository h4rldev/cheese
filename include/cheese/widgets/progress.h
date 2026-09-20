#ifndef CHEESE_WIDGETS_PROGRESS_H
#define CHEESE_WIDGETS_PROGRESS_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief A progress bar bound to an f32 value in @c 0.0f..1.0f.
 * @details The value is clamped to the range.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The semantics to emit.
 * @param x,y The top-left position.
 * @param w,h The bar's size.
 * @param value The value binding.
 */
void cheese_progress_bar(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, f32 x, f32 y, f32 w,
                         f32 h, cheese_value_t value);

//
//
//

/**
 * @brief A progress bar placed by the current layout, filling its width.
 *
 * @see cheese_progress_bar
 */
void cheese_progress_bar_auto(cheese_t *cheese, const cstr *classes,
                              cheese_semantics_t semantics,
                              cheese_value_t value);

#endif // !CHEESE_WIDGETS_PROGRESS_H
