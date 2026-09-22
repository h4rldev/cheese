#ifndef CHEESE_WIDGETS_TEXT_H
#define CHEESE_WIDGETS_TEXT_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/** @brief Caret colour; a @ref CHEESE_PROP_COLOR. */
#define CHEESE_PROP_CARET_COLOR "text/caret/color"

/** @brief Caret thickness in pixels; a @ref CHEESE_PROP_F32. */
#define CHEESE_PROP_CARET_WIDTH "text/caret/width"

/** @brief Caret shape, a @ref cheese_text_caret_style_t; a @ref
 * CHEESE_PROP_U32. */
#define CHEESE_PROP_CARET_STYLE "text/caret/style"

/** @brief Caret blink period in seconds, `0` to hold steady; a @ref
 * CHEESE_PROP_F32. */
#define CHEESE_PROP_CARET_BLINK "text/caret/blink"

/**
 * @brief Shared editing core of text widgets.
 * @details Applies this frame's key events to a caller-owned, NUL-terminated
 * UTF-8 buffer: codepoint insertion, backspace/delete, left/right/home/end,
 * shift-extension of the selection, and replace-on-type. A leaf owns the buffer
 * and the drawing; this is the reusable seam that future text variants build
 * on. @c state is updated in place.
 *
 * When @c multiline is false, Enter is ignored. When true, Enter inserts a
 * newline and Up/Down move between lines. Soft-wrap, vertical scrolling and
 * their styling are out of scope (a later meta-widget).
 *
 * @param cheese The cheese context.
 * @param state Persistent caret/selection state, updated in place.
 * @param buf The mutable UTF-8 buffer; NUL-terminated at @c *len.
 * @param cap The buffer's capacity in bytes, including the terminator.
 * @param len In/out byte length, updated on change.
 * @param multiline Whether Enter/Up/Down edit across lines.
 * @param font The font to render with.
 * @param wrap_w The width at which to wrap.
 * @param page_lines Lines a PageUp/PageDown jumps; 0 is treated as 1.
 *
 * @return true if the buffer changed.
 */
b32 cheese_text_edit(cheese_t *cheese, cheese_text_input_t *state, cstr *buf,
                     u32 cap, u32 *len, b32 multiline, cheese_font_t *font,
                     f32 wrap_w, u32 page_lines);

//
//
//

/**
 * @brief Copy the current selection to the clipboard.
 * @details Uses the callbacks set with @ref cheese_set_clipboard; a no-op when
 * none is set or the selection is empty.
 *
 * @param cheese The cheese context.
 * @param state The editing state holding the caret/anchor.
 * @param buf The UTF-8 buffer; NUL-terminated.
 *
 * @return true if there was a selection and it was copied.
 */
b32 cheese_text_edit_copy(cheese_t *cheese, const cheese_text_input_t *state,
                          const cstr *buf);

//
//
//

/**
 * @brief Cut the current selection to the clipboard.
 * @details Copies the selection (@ref cheese_text_edit_copy), then removes the
 * selected bytes and collapses the caret to the selection start.
 *
 * @param cheese The cheese context.
 * @param state The editing state, updated in place.
 * @param buf The mutable UTF-8 buffer.
 * @param cap The buffer's capacity in bytes, including the terminator.
 * @param len In/out byte length, updated on change.
 *
 * @return true if the buffer changed.
 */
b32 cheese_text_edit_cut(cheese_t *cheese, cheese_text_input_t *state,
                         cstr *buf, u32 cap, u32 *len);

//
//
//

/**
 * @brief Paste the clipboard at the caret, replacing any selection.
 * @details Inserts the clipboard text, honouring @c cap and snapping the
 * insertion to a UTF-8 boundary. Newlines are dropped unless @c multiline.
 *
 * @param cheese The cheese context.
 * @param state The editing state, updated in place.
 * @param buf The mutable UTF-8 buffer.
 * @param cap The buffer's capacity in bytes, including the terminator.
 * @param len In/out byte length, updated on change.
 * @param multiline Whether newlines in the pasted text are kept.
 *
 * @return true if the buffer changed.
 */
b32 cheese_text_edit_paste(cheese_t *cheese, cheese_text_input_t *state,
                           cstr *buf, u32 cap, u32 *len, b32 multiline);
//
//
//

/**
 * @brief Draw an editable text field bound to a string value.
 * @details Focus with a click; keys edit only while focused (see
 * @ref cheese_text_edit, the shared core). The caret, selection anchor and
 * horizontal scroll live in the caller-owned @c state, so they persist across
 * frames (cheese keeps no per-widget storage).
 *
 * @c variant selects single-line (@ref CHEESE_TEXT_INPUT_LINE, Enter ignored)
 * or multiline (@ref CHEESE_TEXT_INPUT_MULTILINE, Enter inserts a newline and
 * Up/Down move between lines). Lines break only on explicit newlines.
 *
 * @param cheese The cheese context.
 * @param classes The style classes to apply.
 * @param semantics The semantics to emit.
 * @param x,y The top-left position.
 * @param w,h The field's size.
 * @param text The string binding (read = bind, edit = set).
 * @param state Persistent caret/selection/scroll state, or null for read-only.
 * @param scroll The scroll binding (read = bind, edit = set).
 * @param variant Single-line or multiline.
 * @param font The font to render with.
 *
 * @return true if the text changed this frame.
 */

b32 cheese_text_input(cheese_t *cheese, const cstr *classes,
                      cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                      cheese_value_t text, cheese_text_input_t *state,
                      cheese_value_t scroll,
                      cheese_text_input_variant_t variant, cheese_font_t *font);

//
//
//

/**
 * @brief A text field placed by the current layout, filling its width.
 *
 * @see cheese_text_input
 */

b32 cheese_text_input_auto(cheese_t *cheese, const cstr *classes,
                           cheese_semantics_t semantics, cheese_value_t text,
                           cheese_text_input_t *state, cheese_value_t scroll,
                           cheese_text_input_variant_t variant,
                           cheese_font_t *font);

#endif // !CHEESE_WIDGETS_TEXT_H
