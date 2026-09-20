#ifndef CHEESE_WIDGETS_WIDGET_H
#define CHEESE_WIDGETS_WIDGET_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Begin a widget's interaction scope.
 * @details Resolves identity (@ref cheese_semantics_peek_id), then fills @c out
 * with the frame's bounds, hover/focus/press/capture facts, and the base
 * `HOVERED`/`FOCUSED` state bits. Leaves read @c out instead of re-deriving the
 * pointer/focus handling; draw and layout stay in the leaf.
 *
 * @param cheese The cheese context.
 * @param semantics The widget's descriptor.
 * @param role The widget's default role.
 * @param x,y,w,h The widget's screen rect.
 * @param out Filled with the scope.
 */
void cheese_widget_begin(cheese_t *cheese, cheese_semantics_t semantics,
                         cheese_role_t role, f32 x, f32 y, f32 w, f32 h,
                         cheese_widget_t *out);

//
//
//

/**
 * @brief Whether the scope was activated this frame.
 * @details The single activation path: a left click while hovered, or
 * Enter/Space while focused.
 *
 * @param cheese The cheese context.
 * @param w The scope from @ref cheese_widget_begin.
 *
 * @return True if activated this frame.
 */
b32 cheese_widget_activated(const cheese_t *cheese, const cheese_widget_t *w);

//
//
//

/**
 * @brief Emit the scope's semantics node.
 *
 * @param cheese The cheese context.
 * @param semantics The widget's descriptor.
 * @param role The widget's role.
 * @param name The node's name, or null.
 * @param value The node's stringified value, or null.
 * @param extra_state Extra @c CHEESE_STATE_* bits (e.g. @c CHECKED).
 * @param w The scope from @ref cheese_widget_begin.
 *
 * @return The new node's index, or -1.
 */
i32 cheese_widget_emit(cheese_t *cheese, cheese_semantics_t semantics,
                       cheese_role_t role, const cstr *name, const cstr *value,
                       u32 extra_state, const cheese_widget_t *w);

#endif // !CHEESE_WIDGETS_WIDGET_H
