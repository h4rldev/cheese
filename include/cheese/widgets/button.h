#ifndef CHEESE_WIDGETS_BUTTON_H
#define CHEESE_WIDGETS_BUTTON_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Draw a button, returning which mouse buttons were clicked.
 * @details @c label is a data binding (see @ref cheese_value_t): a literal or a
 * state-bound value. Colors override the resolved style when non-zero.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The semantics to emit.
 * @param x,y The top-left position.
 * @param w,h The button's size.
 * @param label The label binding.
 * @param font The font to render the label with.
 * @param bg_color Base background, or 0 for the style's.
 * @param hover_color Hover background, or 0 to derive from @c bg_color.
 * @param text_color Text color, or 0 for the style's.
 *
 * @return The CHEESE_BUTTON_CLICK_* flags for this frame.
 */
u32 cheese_button_ex(cheese_t *cheese, const cstr *classes,
                     cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                     cheese_value_t label, cheese_font_t *font,
                     cheese_color_t bg_color, cheese_color_t hover_color,
                     cheese_color_t text_color);

//
//
//

/**
 * @brief Draw a button.
 *
 * @return true if the left mouse button was clicked on it.
 *
 * @see cheese_button_ex
 */
b32 cheese_button(cheese_t *cheese, const cstr *classes,
                  cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                  cheese_value_t label, cheese_font_t *font,
                  cheese_color_t bg_color, cheese_color_t hover_color,
                  cheese_color_t text_color);

//
//
//

/**
 * @brief A button sized to its label and placed by the current layout.
 *
 * @see cheese_button
 */
b32 cheese_button_auto(cheese_t *cheese, const cstr *classes,
                       cheese_semantics_t semantics, cheese_value_t label,
                       cheese_font_t *font);

#endif // !CHEESE_WIDGETS_BUTTON_H
