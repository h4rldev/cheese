#ifndef CHEESE_RENDER_ATLAS_H
#define CHEESE_RENDER_ATLAS_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Allocate a region from the font atlas free-list.
 * @details Finds a free region that fits @c (w, h) and carves it out, splitting
 * the source into leftover strips. The caller owns the returned rect and must
 * return it via @ref cheese_atlas_free.
 *
 * @param variant The font size variant whose atlas is being allocated from.
 * @param w The requested width.
 * @param h The requested height.
 * @param out The rect to fill with the allocated position and size.
 *
 * @pre
 * - @c variant must be a valid font size variant with an initialized free-list.
 * - @c w and @c h must be greater than 0.
 * - @c out must be a valid pointer.
 *
 * @return true if a region was allocated, false if no free region fits.
 */
b32 cheese_atlas_alloc(cheese_font_size_variant_t *variant, u32 w, u32 h,
                       cheese_rect_t *out);

//
//
//

/**
 * @brief Return a region to the font atlas free-list.
 * @details Adds the freed @c (x, y, w, h) rect back to the free-list, merging
 * it with any immediately adjacent free rect to reduce fragmentation.
 *
 * @param variant The font size variant whose atlas is being returned to.
 * @param x The x position of the region.
 * @param y The y position of the region.
 * @param w The width of the region.
 * @param h The height of the region.
 *
 * @pre
 * - @c variant must be a valid font size variant with an initialized free-list.
 * - The region must have been previously allocated via @ref cheese_atlas_alloc.
 */
void cheese_atlas_free(cheese_font_size_variant_t *variant, i32 x, i32 y, u32 w,
                       u32 h);

#endif // !CHEESE_RENDER_ATLAS_H
