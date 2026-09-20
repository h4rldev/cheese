/***********************************/

#include <htils/basictypes.h>
#include <htils/darray.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/render/atlas.h>

/***********************************/

/**
 * @brief Remove an element from the free-list.
 * @details Shifts the elements after @c index left by one, then pops the last
 * slot.
 *
 * @param rects The free-list array.
 * @param index The index to remove.
 *
 * @pre
 * - @c rects must be a valid free-list array.
 * - @c index must be less than the length of @c rects.
 */
static void atlas_remove(cheese_rect_t *rects, u32 index) {
  u64 len = da_len(rects);

  for (u64 i = index; i + 1 < len; i++)
    rects[i] = rects[i + 1];

  da_pop(rects);
}

//
//
//

b32 cheese_atlas_alloc(cheese_font_size_variant_t *variant, u32 w, u32 h,
                       cheese_rect_t *out) {
  if (!variant || !out || w == 0 || h == 0) {
    cheese_log_error("cheese_atlas_alloc: Invalid parameters");
    return false;
  }

  for (u64 i = 0; i < da_len(variant->free_rects); i++) {
    cheese_rect_t *fr = &variant->free_rects[i];
    if (fr->w < w || fr->h < h)
      continue;

    out->x = fr->x;
    out->y = fr->y;
    out->w = w;
    out->h = h;

    if (fr->w == w && fr->h == h)
      atlas_remove(variant->free_rects, i);
    else if (fr->w == w) {
      fr->y += (i32)h;
      fr->h -= h;
    } else if (fr->h == h) {
      fr->x += (i32)w;
      fr->w -= w;
    } else {
      u32 orig_h = fr->h;
      cheese_rect_t right = {fr->x + (i32)w, fr->y, fr->w - w, orig_h};
      da_append(variant->arena, variant->free_rects, right);
      fr->w = w;
      fr->h = orig_h - h;
      fr->y += (i32)h;
    }

    return true;
  }
  return false;
}

void cheese_atlas_free(cheese_font_size_variant_t *variant, i32 x, i32 y, u32 w,
                       u32 h) {
  if (!variant) {
    cheese_log_error("cheese_atlas_free: Missing variant");
    return;
  }

  for (u64 i = 0; i < da_len(variant->free_rects); i++) {
    cheese_rect_t *fr = &variant->free_rects[i];

    if (fr->y == y && fr->h == h && fr->x == x + (i32)w) {
      fr->x = x;
      fr->w += w;
      return;
    }

    if (fr->x == x && fr->w == w && fr->y == y + (i32)h) {
      fr->y = y;
      fr->h += h;
      return;
    }

    if (fr->y == y && fr->h == h && x == fr->x + (i32)fr->w) {
      fr->w += w;
      return;
    }

    if (fr->x == x && fr->w == w && y == fr->y + (i32)fr->h) {
      fr->h += h;
      return;
    }
  }

  cheese_rect_t temp = {x, y, w, h};
  da_append(variant->arena, variant->free_rects, temp);
}
