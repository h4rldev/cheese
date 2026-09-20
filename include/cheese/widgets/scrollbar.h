#ifndef CHEESE_WIDGETS_SCROLLBAR_H
#define CHEESE_WIDGETS_SCROLLBAR_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Shared scrollbar behaviour: press to jump, capture, drag, page keys.
 * @details Maps the thumb position along the scope's track to a pixel scroll
 * offset in `[0, content - viewport]` while the scope owns the pointer, and
 * steps by ~10% of the viewport on arrow keys / a full viewport on
 * PageUp/PageDown while focused. The pointer centres the thumb on press (no
 * carried grab offset), so the interaction needs no caller state. Writes the
 * bound value and returns it, so a leaf can supply its own thumb drawing.
 *
 * @param cheese The cheese context.
 * @param scope The widget scope whose bounds are the track.
 * @param scroll The scroll binding, in pixels.
 * @param viewport The visible extent, in pixels.
 * @param content The total scrollable extent, in pixels.
 * @param axis The scrollbar's orientation.
 * @param editing Whether the scope is focused and the pointer is captured.
 *
 * @return The (possibly changed) scroll offset.
 */
f32 cheese_scrollbar_begin(cheese_t *cheese, const cheese_widget_t *scope,
                           cheese_value_t scroll, f32 viewport, f32 content,
                           cheese_scrollbar_axis_t axis, b32 editing);

//
//
//

/**
 * @brief A scrollbar bound to an f32 state in pixels.
 * @details Drag (pointer capture), a click anywhere on the track, and
 * arrow/Page keys when focused all write the bound state, clamped to
 * `0..content - viewport`. The thumb is sized from the viewport/content ratio
 * (never smaller than 24px). Draws no scrollbar when `content <= viewport` is
 * the caller's choice: just skip the call.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The semantics to emit.
 * @param x,y The top-left position.
 * @param w,h The track size.
 * @param scroll The scroll binding, in pixels.
 * @param viewport The visible extent, in pixels.
 * @param content The total scrollable extent, in pixels.
 * @param axis The scrollbar's orientation.
 *
 * @return The (possibly changed) scroll offset.
 */
f32 cheese_scrollbar(cheese_t *cheese, const cstr *classes,
                     cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                     cheese_value_t scroll, f32 viewport, f32 content,
                     cheese_scrollbar_axis_t axis);

//
//
//

/**
 * @brief A scrollbar placed by the current layout.
 * @details Vertical places a 12px-wide track filling the layout height;
 * horizontal fills the width at 12px tall.
 *
 * @return The (possibly changed) scroll offset.
 */
f32 cheese_scrollbar_auto(cheese_t *cheese, const cstr *classes,
                          cheese_semantics_t semantics, cheese_value_t scroll,
                          f32 viewport, f32 content,
                          cheese_scrollbar_axis_t axis);

#endif // !CHEESE_WIDGETS_SCROLLBAR_H
