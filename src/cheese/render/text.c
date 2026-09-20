/***********************************/

#include <htils/basictypes.h>
#include <htils/darray.h>

#include <cheese/types.h>

#include <cheese/core/utf8.h>

#include <cheese/render/font.h>
#include <cheese/render/text.h>

/***********************************/

f32 cheese_text_prefix_w(cheese_font_t *font, const cstr *base, u32 len) {
  if (!font || len == 0)
    return 0.0f;

  string_slice s = string_slice_from_cstr((u8 *)base, len);
  return cheese_font_measure_text(font, &s);
}

cheese_text_line_t *cheese_text_lines(cheese_t *cheese, cheese_font_t *font,
                                      const cstr *s, u32 len, f32 wrap_w,
                                      u32 *count) {
  cheese_text_line_t *lines = null;
  da_new(cheese->frame_arena, lines, 8);

  b32 wrap = font && wrap_w > 0.0f;
  u32 hard_start = 0;

  for (u32 i = 0; i <= len; i++) {
    if (i < len && s[i] != '\n')
      continue;

    u32 hard_end = i;
    u32 start = hard_start;

    if (!wrap || start == hard_end) {
      cheese_text_line_t line = {start, hard_end};
      da_append(cheese->frame_arena, lines, line);
    } else {
      while (start < hard_end) {
        u32 fit = start;
        u32 last_space = 0;
        b32 have_space = false;

        u32 j = start;

        while (j < hard_end) {
          u32 next = cheese_utf8_next(s, hard_end, j);
          f32 w = cheese_text_prefix_w(font, s + start, next - start);
          if (w > wrap_w && j > start)
            break;

          if (s[j] == ' ') {
            last_space = j;
            have_space = true;
          }

          fit = next;
          j = next;
        }

        u32 line_end = fit;
        if (j >= hard_end)
          line_end = hard_end;

        else if (have_space && last_space > start)
          line_end = last_space;

        cheese_text_line_t line = {start, line_end};
        da_append(cheese->frame_arena, lines, line);

        if (line_end >= hard_end)
          break;

        start = (line_end == last_space) ? last_space + 1 : line_end;
      }
    }

    hard_start = i + 1;
  }

  *count = (u32)da_len(lines);
  return lines;
}

u32 cheese_text_line_of(const cheese_text_line_t *lines, u32 count, u32 byte) {
  for (u32 i = 0; i < count; i++)
    if (byte <= lines[i].end)
      return i;

  return count > 0 ? count - 1 : 0;
}

u32 cheese_text_caret_from_x(cheese_font_t *font, const cstr *s, u32 start,
                             u32 end, f32 local) {
  if (!font || local <= 0.0f)
    return start;

  u32 i = start;
  f32 prev = 0.0f;

  while (i < end) {
    u32 next = cheese_utf8_next(s, end, i);
    f32 w = cheese_text_prefix_w(font, s + start, next - start);

    if (local < (prev + w) * 0.5f)
      break;

    prev = w;
    i = next;
  }

  return i;
}
