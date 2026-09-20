#ifndef CHEESE_CORE_INIT_H
#define CHEESE_CORE_INIT_H

/***********************************/

#include <cheese/types.h>

/***********************************/

/**
 * @brief Create a cheese context backed by a persistent arena.
 * @details Creates the context's state store (`cheese->store`); call once.
 *
 * @param persistent The persistent arena the context and store live in.
 *
 * @return The new context.
 */
cheese_t cheese_default(arena_t *persistent);

//
//
//

/**
 * @brief Set the cursor-output callback.
 * @details The callback is invoked on the render thread during a frame with the
 * cursor a hovered widget wants; the consumer marshals it to whichever thread
 * owns the window. It is the only output from cheese back to the window.
 *
 * @param cheese The cheese context.
 * @param cb The callback, or null to clear it.
 * @param userdata Passed through to @c cb.
 */
void cheese_set_cursor_callback(cheese_t *cheese, cheese_cursor_callback_t cb,
                                void *userdata);

//
//
//

/**
 * @brief Set the clipboard callbacks.
 * @details The clipboard lives in the platform layer (SDL, the window layer,
 * ...); cheese only calls these. @c get returns the current text (borrowed, may
 * be null); @c set copies @c text into the platform clipboard. With no
 * callbacks set, text copy/cut/paste silently do nothing.
 */
void cheese_set_clipboard(cheese_t *cheese, cheese_clipboard_get_fn get,
                          cheese_clipboard_set_fn set, void *userdata);

//
//
//

/**
 * @brief Request the mouse cursor for the rest of the frame.
 * @details Widgets call this while hovering; the last request in a frame wins.
 * The callback set with @ref cheese_set_cursor_callback runs once in
 * @ref cheese_end with the frame's request (or the default if none), and only
 * when it differs from the previously emitted cursor - so a consumer can apply
 * it directly without de-duplicating or filtering a transient default.
 *
 * @param cheese The cheese context.
 * @param cursor The cursor to request.
 */
void cheese_cursor_request(cheese_t *cheese, cheese_cursor_t cursor);

//
//
//

/**
 * @brief The cursor currently in effect (the last one emitted).
 * @details A window layer that drops a cursor change - e.g. an X11 backend
 * that ignores it while it has no mouse focus - can re-apply this on a focus
 * change.
 *
 * @param cheese The cheese context.
 *
 * @return The last requested cursor, or @c CHEESE_CURSOR_DEFAULT.
 */
cheese_cursor_t cheese_cursor_current(const cheese_t *cheese);
//
//
//

/**
 * @brief Hand cheese this frame's input, on the app's thread.
 * @details Diffs against the previous call and, on any change (first call
 * included), marks a frame needed - so the consumer no longer tracks
 * `needs_redraw` for pointer/keyboard motion. @ref cheese_begin still receives
 * the input by value for the frame.
 *
 * @param cheese The cheese context.
 * @param input The frame's input.
 *
 * @pre @c cheese must be valid and cannot be `null`.
 */
void cheese_input(cheese_t *cheese, cheese_input_t input);

//
//
//

/**
 * @brief Begin a frame.
 * @details Resets the per-frame state (overlays, stacks, styles) and reads the
 * frame's input. Runs on the render thread; transient data is allocated from
 * @c frame_arena.
 *
 * @param cheese The cheese context.
 * @param frame_arena The per-frame arena for transient allocations.
 * @param font The font for this frame, or null.
 * @param renderer The renderer to draw through.
 * @param input The frame's input.
 * @param delta_time Seconds since the previous frame.
 */
void cheese_begin(cheese_t *cheese, arena_t *frame_arena, cheese_font_t *font,
                  cheese_renderer_t *renderer, cheese_input_t input,
                  f32 delta_time);

//
//
//

/**
 * @brief End a frame.
 * @details Flushes queued overlays, finalises the semantics tree and runs
 * end-of-frame input handling (e.g. Tab focus movement).
 *
 * @param cheese The cheese context.
 */
void cheese_end(cheese_t *cheese);

#endif // !CHEESE_CORE_INIT_H
