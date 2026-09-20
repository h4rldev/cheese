#ifndef CHEESE_WIDGETS_IMAGE_H
#define CHEESE_WIDGETS_IMAGE_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Draw a textured image.
 * @details Draws @c texture (from @ref cheese_texture_upload) into the box with
 * the given @c fit: stretch, contain (letterbox) or cover (crop, clipped to the
 * box). @c tint modulates the texture (0 = none). A texture with @c id 0 draws
 * the style background instead, so an async decode can show a placeholder.
 *
 * @param semantics @c name is the alt text.
 */
void cheese_image(cheese_t *cheese, const cstr *classes,
                  cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                  cheese_texture_t texture, cheese_fit_t fit,
                  cheese_color_t tint);

//
//
//

/**
 * @brief An image placed by the current layout, sized to the texture aspect.
 *
 * @see cheese_image
 */
void cheese_image_auto(cheese_t *cheese, const cstr *classes,
                       cheese_semantics_t semantics, cheese_texture_t texture,
                       cheese_fit_t fit, cheese_color_t tint);

#endif // !CHEESE_WIDGETS_IMAGE_H
