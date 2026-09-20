#ifndef CHEESE_WIDGETS_TOGGLE_H
#define CHEESE_WIDGETS_TOGGLE_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Shared core of toggle widgets (checkbox, radio, switch, ...).
 * @details Resolves style and metrics, hit-tests, handles focus and activation
 * (click / Enter / Space), and fills @c out. A leaf then does:
 * `cheese_toggle_begin` -> apply the value rule -> `cheese_toggle_emit` ->
 * draw its own indicator -> `cheese_toggle_label`. Tolerates a null @c font
 * (16px fallback, no label).
 */
void cheese_toggle_begin(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, cheese_role_t role,
                         f32 x, f32 y, cheese_value_t label,
                         cheese_font_t *font, cheese_toggle_t *out);

//
//
//

/** @brief Emit the toggle's semantics node (call before drawing). */
i32 cheese_toggle_emit(cheese_t *cheese, cheese_semantics_t semantics,
                       const cheese_toggle_t *toggle, cheese_role_t role,
                       b32 chosen);

//
//
//

/** @brief Draw the toggle's label after the indicator. */
void cheese_toggle_label(cheese_t *cheese, const cheese_toggle_t *toggle,
                         cheese_font_t *font, cheese_color_t color);

//
//
//

/**
 * @brief Draw a checkbox.
 * @details @c checked is a data binding: reading it marks it draw-relevant, and
 * a click writes the toggled value back when it is state-bound.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The semantics to emit.
 * @param x,y The top-left position.
 * @param checked The checked binding.
 * @param label The label binding.
 * @param font The font to render the label with.
 *
 * @return The (possibly toggled) value.
 */
b32 cheese_checkbox(cheese_t *cheese, const cstr *classes,
                    cheese_semantics_t semantics, f32 x, f32 y,
                    cheese_value_t checked, cheese_value_t label,
                    cheese_font_t *font);

//
//
//

/**
 * @brief A checkbox placed by the current layout.
 *
 * @see cheese_checkbox
 */
b32 cheese_checkbox_auto(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, cheese_value_t checked,
                         cheese_value_t label, cheese_font_t *font);

//
//
//

b32 cheese_radio(cheese_t *cheese, const cstr *classes,
                 cheese_semantics_t semantics, f32 x, f32 y,
                 cheese_value_t selected, i32 index, cheese_value_t label,
                 cheese_font_t *font);

//
//
//

b32 cheese_radio_auto(cheese_t *cheese, const cstr *classes,
                      cheese_semantics_t semantics, cheese_value_t selected,
                      i32 index, cheese_value_t label, cheese_font_t *font);

#endif // !CHEESE_WIDGETS_TOGGLE_H
