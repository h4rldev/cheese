#ifndef CHEESE_WIDGETS_DROPDOWN_H
#define CHEESE_WIDGETS_DROPDOWN_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief An anchored dropdown (combo box) bound to an i32 selection.
 * @details The trigger draws as a button labelled with the current item; a
 * click (or Enter/Space while focused) opens the list, drawn as an overlay
 * (see @ref cheese_overlay) below the trigger. Clicking an item writes the
 * bound @c selected and closes; clicking outside or Escape closes. @c items
 * must stay valid for the frame (literals are fine); @c state is caller-owned
 * and persists the open flag across frames.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The semantics to emit.
 * @param x,y The trigger's top-left position.
 * @param w,h The trigger's size (also the popover's width).
 * @param selected The i32 selection binding.
 * @param items The item labels.
 * @param count Number of items.
 * @param state Persistent open flag.
 * @param font The font to render with.
 *
 * @return The (possibly changed) selection.
 */
i32 cheese_dropdown(cheese_t *cheese, const cstr *classes,
                    cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                    cheese_value_t selected, const cstr *const *items,
                    u32 count, cheese_popup_t *state, cheese_font_t *font);

//
//
//

/**
 * @brief A dropdown placed by the current layout, filling its width.
 *
 * @see cheese_dropdown
 */
i32 cheese_dropdown_auto(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, cheese_value_t selected,
                         const cstr *const *items, u32 count,
                         cheese_popup_t *state, cheese_font_t *font);

#endif // !CHEESE_WIDGETS_DROPDOWN_H
