#ifndef CHEESE_STYLE_INTERNAL_H
#define CHEESE_STYLE_INTERNAL_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

typedef struct {
  u32 id;
  u8 kind;
} cheese_style_prop_def_t;

//
//
//

/**
 * @brief Find a property in a style's bag by id.
 *
 * @param style The style to search.
 * @param prop The property id.
 *
 * @return A pointer to the entry, or @c null when absent.
 */
cheese_prop_t *cheese_style_prop_find(cheese_style_t *style, u32 prop);

//
//
//

/**
 * @brief Insert or replace a property in a style's bag.
 * @details Allocates the grown array from @p arena; replaces any entry with the
 * same id. A zero id is ignored.
 *
 * @param arena The arena to allocate from.
 * @param style The style to write.
 * @param value The entry to store.
 */
void cheese_style_prop_put(arena_t *arena, cheese_style_t *style,
                           cheese_prop_t value);

#endif // !CHEESE_STYLE_INTERNAL_H
