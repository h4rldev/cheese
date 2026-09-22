#ifndef CHEESE_RENDER_TEXT_H
#define CHEESE_RENDER_TEXT_H

/***********************************/

#include <htils/basictypes.h>
#include <htils/darray.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief A laid-out line of text.
 * @details A byte range of the source string; @ref end excludes the trailing
 * newline, so an empty line has `start == end`.
 *
 * @param start First byte of the line.
 * @param end One past the last byte of the line.
 */
typedef struct {
  u32 start, end;
} cheese_text_line_t;

//
//
//

/**
 * @brief Measure the pixel width of a text prefix.
 * @details Measures the first @c len bytes of @c base with @c font.
 * Returns `0` when the font is null or the prefix is empty.
 *
 * @param font The font to measure with.
 * @param base The start of the text.
 * @param len The number of bytes to measure.
 *
 * @return The prefix width in pixels.
 */
f32 cheese_text_prefix_w(cheese_font_t *font, const cstr *base, u32 len);

//
//
//

/**
 * @brief Wrap a string into lines.
 * @details Splits on `\n` (i.e LF), then greedily wraps each segment at the
 * last space that fits on @c wrap_w (hard-breaking words longer than the
 * width). A @c wrap_w of `0` disables wrapping. The returned array is allocated
 * from @c cheese's frame arena and always holds at least one line, so it is
 * safe to index.
 *
 * @param cheese The cheese context, owning the frame arena.
 * @param font The font to measure with.
 * @param s The source string to wrap.
 * @param len The number of bytes to wrap.
 * @param wrap_w The maximum width of a line, or `0` to disable wrapping.
 * @param count The number of lines in the returned array.
 *
 * @return The line array.
 */
cheese_text_line_t *cheese_text_lines(cheese_t *cheese, cheese_font_t *font,
                                      const cstr *s, u32 len, f32 wrap_w,
                                      u32 *count);

//
//
//

/**
 * @brief Find the line that contains a byte offset.
 *
 * @param lines The line array.
 * @param count The number of lines in @c lines.
 * @param byte The byte offset in the source string.
 *
 * @return The index of the containing line, clamped to the array.
 */
u32 cheese_text_line_of(const cheese_text_line_t *lines, u32 count, u32 byte);

//
//
//

/**
 * @brief Map a local x position to the nearest byte offset in a line.
 * @details Walks the codepoints of `s[start..end]`, comparing @c local against
 * the midpoint between each codepoint's prefix width. The line is shaped once
 * and its cumulative advances are read, so the cost is linear in the line
 * length.
 *
 * @param cheese The cheese context (its frame arena holds the temporary
 * cumulative-width array.)
 * @param font Font used for measuring.
 * @param s Source string.
 * @param start First byte of the line.
 * @param end One past the last byte of the line.
 * @param local x offset from the line start, in pixels.
 *
 * @return The byte offset of the nearest caret position.
 */
u32 cheese_text_caret_from_x(cheese_t *cheese, cheese_font_t *font,
                             const cstr *s, u32 start, u32 end, f32 local);

#endif // !CHEESE_RENDER_TEXT_H
