/***********************************/

#include <htils/basictypes.h>

#include <cheese/core/utf8.h>
#include <cheese/types.h>

/***********************************/

u32 cheese_utf8_next(const cstr *s, u32 len, u32 i) {
  if (i >= len)
    return len;

  u8 c = (u8)s[i];
  if (c < 0x80)
    return i + 1;
  if ((c & 0xE0) == 0xC0)
    return min(i + 2, len);
  if ((c & 0xF0) == 0xE0)
    return min(i + 3, len);
  if ((c & 0xF8) == 0xF0)
    return min(i + 4, len);
  return i + 1;
}

u32 cheese_utf8_prev(const cstr *s, u32 i) {
  if (i == 0)
    return 0;

  i--;
  while (i > 0 && ((u8)s[i] & 0xC0) == 0x80)
    i--;
  return i;
}

u32 cheese_utf8_count(const cstr *s, u32 len) {
  u32 n = 0;
  for (u32 i = 0; i < len; i = cheese_utf8_next(s, len, i))
    n++;
  return n;
}

u32 cheese_utf8_offset(const cstr *s, u32 len, u32 cp) {
  u32 i = 0;
  while (cp > 0 && i < len) {
    i = cheese_utf8_next(s, len, i);
    cp--;
  }
  return i;
}

u32 cheese_utf8_encode(u32 cp, char out[4]) {
  if (cp < 0x80) {
    out[0] = (char)cp;
    return 1;
  }
  if (cp < 0x800) {
    out[0] = (char)(0xC0 | (cp >> 6));
    out[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000) {
    out[0] = (char)(0xE0 | (cp >> 12));
    out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
  }
  out[0] = (char)(0xF0 | (cp >> 18));
  out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
  out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
  out[3] = (char)(0x80 | (cp & 0x3F));
  return 4;
}
