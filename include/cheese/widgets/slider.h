#ifndef CHEESE_WIDGETS_SLIDER_H
#define CHEESE_WIDGETS_SLIDER_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Shared slider behaviour: press to capture, drag to seek, arrows to
 * step.
 * @details Maps the pointer's x across the scope's `[x, x + w]` to a value in
 * `[0, 1]` while the scope owns the pointer, and steps by @c step on Left/Right
 * while the scope is focused. Writes the bound value and returns it, so a leaf
 * can supply its own track/knob/marker drawing without re-deriving the
 * interaction.
 */
f32 cheese_slider_begin(cheese_t *cheese, const cheese_widget_t *scope,
                        cheese_value_t value, f32 step, b32 editing);

//
//
//

/**
 * @brief A slider bound to an f32 state in @c 0.0f..1.0f.
 * @details Drag (pointer capture), a click anywhere on the track, and
 * left/right arrows when focused all write the bound state. The value is
 * clamped and reported on the semantics node.
 *
 * @return The (possibly changed) value.
 */
f32 cheese_slider(cheese_t *cheese, const cstr *classes,
                  cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                  cheese_value_t value);

//
//
//

/** @brief A slider placed by the current layout, filling its width. */
f32 cheese_slider_auto(cheese_t *cheese, const cstr *classes,
                       cheese_semantics_t semantics, cheese_value_t value);

//
//
//

/**
 * @brief Set the slider fill's gradient.
 * @details Sets the widget-owned `"slider/fill/gradient"` property on
 * @p style; the filled portion draws it instead of its flat colour. The track
 * keeps the core `"bg/gradient"`, so a background gradient never colours the
 * fill. The gradient spans the fill rect.
 *
 * @param cheese The cheese context.
 * @param style The style to write.
 * @param gradient The four-corner gradient.
 */
void cheese_slider_set_fill_gradient(cheese_t *cheese, cheese_style_t *style,
                                     cheese_gradient_t gradient);

#endif // !CHEESE_WIDGETS_SLIDER_H
