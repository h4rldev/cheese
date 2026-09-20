#ifndef CHEESE_WIDGETS_CONTAINER_H
#define CHEESE_WIDGETS_CONTAINER_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Open a styled container at an explicit rect.
 * @details Draws the container background/border, pushes a class scope and a
 * child layout derived from @c layout's flow, anchored at the container's inner
 * origin. Children place into it. Close with @ref cheese_end_container.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The container's descriptor.
 * @param x,y The container's top-left position.
 * @param w,h The container's size (size specs).
 * @param layout The parent layout to derive the child flow from.
 */
void cheese_begin_container(cheese_t *cheese, const cstr *classes,
                            cheese_semantics_t semantics, f32 x, f32 y,
                            cheese_size_t w, cheese_size_t h,
                            const cheese_layout_t *layout);

//
//
//

/**
 * @brief Open a container placed by the current layout.
 * @details Like @ref cheese_begin_container, but resolves @c w/@c h against the
 * current layout, places the container through the flow (or an active anchor)
 * and applies the resolved style's margins.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The container's descriptor.
 * @param w,h The container's size (size specs).
 */
void cheese_begin_container_auto(cheese_t *cheese, const cstr *classes,
                                 cheese_semantics_t semantics, cheese_size_t w,
                                 cheese_size_t h);

//
//
//

/**
 * @brief Close a container.
 *
 * @param cheese The cheese context.
 *
 * @see cheese_begin_container, cheese_begin_container_auto
 */
void cheese_end_container(cheese_t *cheese);

//
//
//

/**
 * @brief Open a scrollable viewport: children are offset by
 * @c (-scroll_x, -scroll_y) and clipped to the viewport.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The viewport's descriptor.
 * @param x,y The viewport's top-left position.
 * @param w,h The viewport's size (size specs).
 * @param scroll_x,scroll_y Bound scroll offsets in pixels.
 */
void cheese_begin_scroll(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, f32 x, f32 y,
                         cheese_size_t w, cheese_size_t h,
                         cheese_value_t scroll_x, cheese_value_t scroll_y);

//
//
//

/** @brief Close the viewport opened by @ref cheese_begin_scroll. */
void cheese_end_scroll(cheese_t *cheese);

#endif // !CHEESE_WIDGETS_CONTAINER_H
