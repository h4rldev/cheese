#ifndef CHEESE_CORE_OVERLAY_H
#define CHEESE_CORE_OVERLAY_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Queue a deferred draw at the top of the z-order.
 * @details The callback runs at @ref cheese_end, after the widget tree, with a
 * clip reset to the overlay's rect and a fresh layout anchored at its top-left.
 * Use it for anything that must escape its parent's clip and draw above its
 * siblings (popovers, menus, dropdowns, dialogs); the callback may use
 * containers and the normal widget API.
 *
 * The overlay's bounds block pointer input to the tree beneath it on the next
 * frame: while the pointer sits over an overlay the main pass sees a muted
 * cursor, so hit-test inside the callback with @c cheese->raw_mouse_x /
 * @c cheese->raw_mouse_y instead.
 *
 * @param cheese The cheese context.
 * @param x,y The overlay's top-left position.
 * @param w,h The overlay's size; content is clipped to it.
 * @param draw The deferred draw callback.
 * @param userdata Passed through to @c draw.
 *
 * @pre @c cheese must be valid and cannot be `null`.
 * @pre @c draw must be valid and cannot be `null`.
 */
void cheese_overlay(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                    cheese_overlay_draw_fn draw, void *userdata);

//
//
//

/**
 * @brief Queue a modal overlay: a full-window scrim that occludes all input.
 * @details Like @ref cheese_overlay, but the queued draw is registered over the
 * whole window (so the tree beneath is fully blocked next frame) and a dimming
 * scrim is drawn first. @c x,y,w,h is the modal panel the callback should draw;
 * the callback receives a window-sized layout, and can use
 * @ref cheese_begin_container + widgets for its content.
 *
 * @param cheese The cheese context.
 * @param x,y The panel's top-left position.
 * @param w,h The panel's size.
 * @param draw The deferred draw callback.
 * @param userdata Passed through to @c draw.
 *
 * @pre @c cheese must be valid and cannot be `null`.
 * @pre @c draw must be valid and cannot be `null`.
 */
void cheese_overlay_modal(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                          cheese_overlay_draw_fn draw, void *userdata);

//
//
//

/**
 * @brief Mark a popup open (stamping the frame).
 * @details Use this rather than writing @c popup.open directly, so
 * @ref cheese_popup_dismissed can tell the opening frame apart and avoid
 * dismissing on it.
 *
 * @param cheese The cheese context.
 * @param popup The popup's state.
 */
void cheese_popup_open(cheese_t *cheese, cheese_popup_t *popup);

//
//
//

/**
 * @brief Mark a popup closed.
 *
 * @param popup The popup's state.
 */
void cheese_popup_close(cheese_popup_t *popup);

//
//
//

/**
 * @brief Test whether an open popup should dismiss this frame.
 * @details The behaviour every popover repeats: Escape, a click outside
 * @c bounds, and the open-this-frame guard. Call it from the popup's overlay
 * draw callback (where the cursor is live) with the popup's screen rect. On a
 * dismissal it clears @c popup->open and returns true.
 *
 * @param cheese The cheese context.
 * @param popup The popup's state.
 * @param bounds The popup's screen rect.
 *
 * @return True if the popup was dismissed this frame.
 */
b32 cheese_popup_dismissed(cheese_t *cheese, cheese_popup_t *popup,
                           cheese_rect_t bounds);

//
//
//

/**
 * @brief Run and clear the queued overlays.
 * @details Called by @ref cheese_end; the consumer never calls it directly.
 *
 * @internal
 *
 * @param cheese The cheese context.
 */
void cheese_overlay_flush(cheese_t *cheese);

#endif // !CHEESE_CORE_OVERLAY_H
