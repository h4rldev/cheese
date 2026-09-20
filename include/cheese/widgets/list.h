#ifndef CHEESE_WIDGETS_LIST_H
#define CHEESE_WIDGETS_LIST_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief A virtualized vertical list bound to a scroll offset.
 * @details Only the rows intersecting the viewport are drawn (via @c
 * draw_item), so the item count can be large. The wheel scrolls when the
 * pointer is over the viewport (positive @c scroll_y scrolls toward the start);
 * @c scroll_y is clamped to the content. It emits a `CONTAINER` node around the
 * visible rows; rows may emit their own semantics from @c draw_item.
 *
 * @param x,y The viewport's top-left position.
 * @param w,h The viewport's size.
 * @param scroll_y Bound vertical scroll offset in pixels.
 * @param count Number of items.
 * @param row_height The fixed row height in pixels.
 * @param draw_item Draws one visible row, or null to skip drawing.
 * @param userdata Passed through to @c draw_item.
 *
 * @return The clicked row index, or -1.
 */
i32 cheese_list(cheese_t *cheese, const cstr *classes,
                cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                cheese_value_t scroll_y, u32 count, f32 row_height,
                cheese_list_item_fn draw_item, void *userdata);

//
//
//

/**
 * @brief A list placed by the current layout (fills width, height from layout).
 *
 * @see cheese_list
 */
i32 cheese_list_auto(cheese_t *cheese, const cstr *classes,
                     cheese_semantics_t semantics, cheese_value_t scroll_y,
                     u32 count, f32 row_height, cheese_list_item_fn draw_item,
                     void *userdata);

#endif // !CHEESE_WIDGETS_LIST_H
