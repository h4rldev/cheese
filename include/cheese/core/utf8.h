#ifndef CHEESE_CORE_UTF8_H
#define CHEESE_CORE_UTF8_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Step to the next UTF-8 codepoint boundary.
 * @details Advances one codepoint, tolerating malformed bytes (an invalid lead
 *          advances a single byte) and clamping to @p len.
 *
 * @param s The UTF-8 buffer.
 * @param len The buffer length in bytes.
 * @param i The current byte offset.
 *
 * @return The byte offset of the next codepoint, or @p len at the end.
 */
u32 cheese_utf8_next(const cstr *s, u32 len, u32 i);

//
//
//

/**
 * @brief Step back to the previous UTF-8 codepoint boundary.
 *
 * @param s The UTF-8 buffer.
 * @param i The current byte offset.
 *
 * @return The byte offset of the previous codepoint start.
 */
u32 cheese_utf8_prev(const cstr *s, u32 i);

//
//
//

/**
 * @brief Count the codepoints in a UTF-8 buffer.
 *
 * @param s The UTF-8 buffer.
 * @param len The buffer length in bytes.
 *
 * @return The number of codepoints.
 */
u32 cheese_utf8_count(const cstr *s, u32 len);

//
//
//

/**
 * @brief Byte offset of the @p cp-th codepoint.
 *
 * @param s The UTF-8 buffer.
 * @param len The buffer length in bytes.
 * @param cp The codepoint index.
 *
 * @return The byte offset, clamped to @p len.
 */
u32 cheese_utf8_offset(const cstr *s, u32 len, u32 cp);

//
//
//

/**
 * @brief Encode a codepoint as UTF-8.
 *
 * @param cp The codepoint.
 * @param out A buffer of at least 4 bytes.
 *
 * @return The number of bytes written, `1..4`.
 */
u32 cheese_utf8_encode(u32 cp, char out[4]);

#endif // !CHEESE_CORE_UTF8_H
