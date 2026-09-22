/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/style/internal.h>

/***********************************/

cheese_prop_t *cheese_style_prop_find(cheese_style_t *style, u32 prop) {
  if (!style)
    return null;

  for (u32 i = 0; i < style->prop_count; i++)
    if (style->props[i].id == prop)
      return &style->props[i];

  return null;
}

void cheese_style_prop_put(arena_t *arena, cheese_style_t *style,
                           cheese_prop_t value) {
  if (!arena || !style || value.id == 0)
    return;

  u32 n = style->prop_count;
  u32 out = 0;

  cheese_prop_t *grown = arena_alloc(arena, cheese_prop_t, n + 1);
  for (u32 i = 0; i < n; i++) {
    if (style->props[i].id == value.id)
      continue;

    grown[out++] = style->props[i];
  }

  grown[out++] = value;

  style->props = grown;
  style->prop_count = out;
}
