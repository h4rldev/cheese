#ifndef CHEESE_WIDGETS_TABS_H
#define CHEESE_WIDGETS_TABS_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief A horizontal tab header bound to an i32 selection.
 * @details Draws one tab per label and writes the clicked (or Enter/Space
 * activated, while focused) index to the bound @c selected. It is only the
 * header: the caller switches on the returned index and draws the tab's content
 * itself, so any content works. Each tab emits a `TAB` node (`CHECKED` when
 * selected). Tabs are content-sized, then stretched to fill @c w when there is
 * room.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The semantics to emit.
 * @param x,y The bar's top-left position.
 * @param w,h The bar's size (0 @c w = content width).
 * @param selected The i32 selection binding.
 * @param labels The tab labels.
 * @param count Number of tabs.
 * @param font The font to render with.
 *
 * @return The (possibly changed) selection.
 */
i32 cheese_tab_bar(cheese_t *cheese, const cstr *classes,
                   cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                   cheese_value_t selected, const cstr *const *labels,
                   u32 count, cheese_font_t *font);

//
//
//

/**
 * @brief A tab bar placed by the current layout, filling its width.
 *
 * @see cheese_tab_bar
 */
i32 cheese_tab_bar_auto(cheese_t *cheese, const cstr *classes,
                        cheese_semantics_t semantics, cheese_value_t selected,
                        const cstr *const *labels, u32 count,
                        cheese_font_t *font);

#endif // !CHEESE_WIDGETS_TABS_H
