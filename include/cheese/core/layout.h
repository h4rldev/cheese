#ifndef CHEESE_CORE_LAYOUT_H
#define CHEESE_CORE_LAYOUT_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief The zero/base layout.
 *
 * @return A layout with a zero box, horizontal flow, spacing 4 and start
 * alignment.
 */
cheese_layout_t cheese_layout_default(void);

//
//
//

/**
 * @brief The layout currently at the top of the stack.
 * @details Widgets place into this layout via @ref cheese_layout_place.
 *
 * @param cheese The cheese context.
 *
 * @return The current layout.
 */
cheese_layout_t *cheese_current_layout(cheese_t *cheese);

//
//
//

/** @brief A fixed-pixel size spec. */
static inline cheese_size_t cheese_px(f32 v) {
  return (cheese_size_t){.mode = CHEESE_SIZE_FIXED, .value = v};
}

//
//
//

/** @brief A size spec as a fraction of the parent extent. */
static inline cheese_size_t cheese_pct(f32 fraction) {
  return (cheese_size_t){.mode = CHEESE_SIZE_PERCENT, .value = fraction};
}

//
//
//

/** @brief A size spec that fills the parent extent. */
static inline cheese_size_t cheese_fill(void) {
  return (cheese_size_t){.mode = CHEESE_SIZE_FILL};
}

//
//
//

/** @brief A size spec that fits the widget's intrinsic size. */
static inline cheese_size_t cheese_fit(void) {
  return (cheese_size_t){.mode = CHEESE_SIZE_FIT};
}

//
//
//

/**
 * @brief Resolve a size spec against a parent extent and intrinsic size.
 * @details FILL uses @c extent, PERCENT scales @c extent, FIXED uses the
 * spec's value, FIT uses @c intrinsic; the spec's min/max clamp the result.
 *
 * @param size The size spec.
 * @param extent The parent extent along this axis.
 * @param intrinsic The widget's intrinsic size.
 *
 * @return The resolved size.
 */
f32 cheese_size_resolve(cheese_size_t size, f32 extent, f32 intrinsic);

//
//
//

/**
 * @brief Place the next widget in the current layout.
 * @details Resolves the size, computes the widget's rect from the flow cursor,
 * and advances the cursor. An anchored layout places at its anchor instead and
 * does not advance the cursor.
 *
 * @param cheese The cheese context.
 * @param w In/out widget width.
 * @param h In/out widget height.
 * @param out_x,out_y Receives the placed top-left.
 */
void cheese_layout_place(cheese_t *cheese, f32 *w, f32 *h, f32 *out_x,
                         f32 *out_y);

//
//
//

/**
 * @brief Manually advance the flow cursor by @c w,@c h.
 *
 * @param cheese The cheese context.
 * @param w,h The size to advance by.
 */
void cheese_advance_cursor(cheese_t *cheese, f32 w, f32 h);

//
//
//

/**
 * @brief Compute a widget's position from the layout's cross alignment.
 * @details Unused by the widgets today.
 */
void cheese_layout_get_aligned_pos(const cheese_layout_t *layout, f32 *widget_w,
                                   f32 *widget_h, f32 *out_x, f32 *out_y);

//
//
//

/** @brief Set the layout's main axis (horizontal vs vertical). */
void cheese_layout_set_horizontal(cheese_layout_t *layout, b32 horizontal);

//
//
//

/** @brief Set the spacing between placed widgets on the main axis. */
void cheese_layout_set_spacing(cheese_layout_t *layout, f32 spacing);

//
//
//

/** @brief Set the cross-axis alignment for X. */
void cheese_layout_set_align_x(cheese_layout_t *layout,
                               cheese_alignment_t align_x);

//
//
//

/** @brief Set the cross-axis alignment for Y. */
void cheese_layout_set_align_y(cheese_layout_t *layout,
                               cheese_alignment_t align_y);

//
//
//

/** @brief Enable or disable wrapping when a line overflows the main extent. */
void cheese_layout_set_wrap(cheese_layout_t *layout, b32 wrap);

//
//
//

/**
 * @brief Lay children out in a grid of @c columns.
 * @details Implies wrapping and forces each cell's main-axis size to the column
 * width.
 */
void cheese_layout_set_columns(cheese_layout_t *layout, u32 columns);

//
//
//

/**
 * @brief Set the cross-axis size mode (and size for FIXED).
 *
 * @param layout The layout.
 * @param mode FIT | FILL | FIXED.
 * @param size The cross size for FIXED; ignored otherwise.
 */
void cheese_layout_set_cross(cheese_layout_t *layout, cheese_size_mode_t mode,
                             f32 size);

//
//
//

/** @brief Push @c layout as the current layout for the following widgets. */
void cheese_push_layout(cheese_t *cheese, cheese_layout_t layout);

//
//
//

/** @brief Pop the layout pushed by @ref cheese_push_layout. */
void cheese_pop_layout(cheese_t *cheese);

//
//
//

/**
 * @brief Push an anchored child layout over the current one.
 * @details The next widget placed in this layout lands at @c anchor within the
 * parent's content box (inset by @c dx,@c dy) instead of the flow cursor, and
 * leaves that cursor untouched - absolute positioning. Pop with
 * @ref cheese_pop_anchor. Containers reset the anchor for their own children.
 *
 * @param cheese The cheese context.
 * @param anchor The corner/edge to anchor to (@ref CHEESE_ANCHOR_FLOW = none).
 * @param dx,dy Inset from the anchored edges (positive moves inward).
 *
 * @pre @c cheese must be valid and cannot be `null`.
 */
void cheese_push_anchor(cheese_t *cheese, cheese_anchor_t anchor, f32 dx,
                        f32 dy);

//
//
//

/** @brief Pop the layout pushed by @ref cheese_push_anchor. */
void cheese_pop_anchor(cheese_t *cheese);

#endif // !CHEESE_CORE_LAYOUT_H
