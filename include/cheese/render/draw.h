#ifndef CHEESE_RENDER_DRAW_H
#define CHEESE_RENDER_DRAW_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Submit the frame's queued draws to the renderer.
 *
 * @param cheese The cheese context.
 */
void cheese_flush_draws(cheese_t *cheese);

//
//
//

/**
 * @brief Draw a rounded rectangle.
 *
 * @param cheese The cheese context.
 * @param radius Per-corner radius.
 * @param x,y,w,h The rect.
 * @param color The fill colour.
 */
void cheese_draw_rect(cheese_t *cheese, cheese_corners_t radius, f32 x, f32 y,
                      f32 w, f32 h, cheese_color_t color);

//
//
//

/**
 * @brief Draw a rounded rectangle with a linear gradient.
 *
 * @param cheese The cheese context.
 * @param radius Per-corner radius.
 * @param x,y,w,h The rect.
 * @param colors The gradient colours.
 */
void cheese_draw_rect_gradient(cheese_t *cheese, cheese_corners_t radius, f32 x,
                               f32 y, f32 w, f32 h, cheese_gradient_t colors);

//
//
//

/**
 * @brief Fill a rect with a style's background, gradient-aware.
 * @details Uses the `"bg/gradient"` property (see @ref
 * cheese_core_style_props_t) when present, tinting each of its four corners by
 * the style's `state_layer_color` at the alpha for @p state (see @ref
 * cheese_state_layer_alpha); otherwise draws @p flat. This is the standard
 * widget-background fill: pass the same @p state the caller gave @ref
 * cheese_style_apply_state, and the flat @c bg_color it left behind.
 *
 * @param cheese The cheese context.
 * @param style The resolved style (read for its gradient and state layer).
 * @param x,y,w,h The rect.
 * @param flat The plain fill colour when no gradient is set.
 * @param state The interaction-state bits, for the state-layer tint.
 * @param alpha Multiply the resolved opacity by this.
 */
void cheese_draw_bg(cheese_t *cheese, const cheese_style_t *style, f32 x, f32 y,
                    f32 w, f32 h, cheese_color_t flat, u32 state, f32 alpha);

//
//
//

/**
 * @brief Fill a rect from a specific part's gradient property.
 * @details Like @ref cheese_draw_bg, but reads the gradient from
 * @p gradient_prop rather than the core `"bg/gradient"` and draws @p flat when
 * that property is absent, so a two-part widget's foreground (a progress fill,
 * a slider fill, a selected tab) never inherits the background gradient. Pass
 * the widget's own `"<widget>/<part>/gradient"` id, or `0` for a plain flat
 * fill. The `state_layer_color` tint is applied to the gradient corners at the
 * alpha for @p state, as in @ref cheese_draw_bg.
 *
 * @param cheese The cheese context.
 * @param style The resolved style.
 * @param x,y,w,h The rect.
 * @param flat The fill colour when @p gradient_prop is unset.
 * @param gradient_prop The part's gradient property id, or `0`.
 * @param state The interaction-state bits, for the state-layer tint.
 * @param alpha Multiply the resolved opacity by this.
 */
void cheese_draw_fill(cheese_t *cheese, const cheese_style_t *style, f32 x,
                      f32 y, f32 w, f32 h, cheese_color_t flat,
                      u32 gradient_prop, u32 state, f32 alpha);

//
//
//

/**
 * @brief Draw a widget's border ring from its resolved style.
 * @details One draw: a rounded-rect ring whose thickness, sides and colour come
 * from the `"border/width"`, `"border/sides"` and `"border/color"` properties
 * (see @ref cheese_core_style_props_t), following @c style->corner_radius. A
 * no-op when the width is absent/0 or the renderer has no @c draw_border.
 */
void cheese_draw_border(cheese_t *cheese, const cheese_style_t *style, f32 x,
                        f32 y, f32 w, f32 h);

//
//
//

/**
 * @brief Draw a widget's focus ring from its resolved style.
 * @details Reads the `"focus_ring/width"`, `"focus_ring/color"` and
 * `"focus_ring/offset"` properties (see @ref cheese_core_style_props_t); a
 * no-op unless the width is positive and the colour is set. Drawn that many
 * pixels outside `[x,y,w,h]`, following the widget's corner radius grown by the
 * offset. Call it while the widget is focused, after drawing its content.
 */
void cheese_draw_focus_ring(cheese_t *cheese, const cheese_style_t *style,
                            f32 x, f32 y, f32 w, f32 h);

//
//
//

/**
 * @brief Draw a line between two points.
 *
 * @param cheese The cheese context.
 * @param x1,y1,x2,y2 The endpoints.
 * @param thickness The line width.
 * @param color The line colour.
 */
void cheese_draw_line(cheese_t *cheese, f32 x1, f32 y1, f32 x2, f32 y2,
                      f32 thickness, cheese_color_t color);

//
//
//

/**
 * @brief Draw an arc.
 * @details Filled when @c thickness is `0`, otherwise a ring band of that
 * width.
 *
 * @param cheese The cheese context.
 * @param cx,cy The centre.
 * @param radius The outer radius.
 * @param start_angle,end_angle The swept angles in radians.
 * @param thickness The band width, or `0` for a filled sector.
 * @param color The colour.
 */
void cheese_draw_arc(cheese_t *cheese, f32 cx, f32 cy, f32 radius,
                     f32 start_angle, f32 end_angle, f32 thickness,
                     cheese_color_t color);

//
//
//

/**
 * @brief Draw text.
 * @details @c y is the glyph baseline, not the top of the text.
 *
 * @param cheese The cheese context.
 * @param x,y The pen position (baseline).
 * @param text The string to draw.
 * @param font The font.
 * @param color The text colour.
 * @param scale An extra scale on top of the font's logical size.
 */
void cheese_draw_text(cheese_t *cheese, f32 x, f32 y, const string *text,
                      cheese_font_t *font, cheese_color_t color, f32 scale);

//
//
//

/**
 * @brief Upload RGBA8 pixels to the renderer and return a texture handle.
 * @details The pixels are non-premultiplied, row-major, top-left origin. This
 * is the seam an external decoder (Wuffs/SDL_image/stb/...) plugs into: it
 * produces an RGBA8 buffer, the consumer uploads once, then draws the handle
 * every frame. On failure @c id is 0.
 */
cheese_texture_t cheese_texture_upload(cheese_t *cheese, u32 width, u32 height,
                                       const u8 *rgba8);

//
//
//

/**
 * @brief Draw a texture into a box.
 *
 * @param cheese The cheese context.
 * @param x,y,w,h The destination box.
 * @param texture_id The texture returned by @ref cheese_texture_upload.
 * @param color A tint, or `0` for the texture's own colours.
 */
void cheese_draw_texture(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                         u32 texture_id, cheese_color_t color);

//
//
//

/**
 * @brief Update a sub-rectangle of an uploaded texture.
 *
 * @param cheese The cheese context.
 * @param texture_id The texture.
 * @param x,y,w,h The region to replace.
 * @param data The pixel data.
 * @param data_size The size of @c data in bytes.
 */
void cheese_update_texture_region(cheese_t *cheese, u32 texture_id, i32 x,
                                  i32 y, u32 w, u32 h, const void *data,
                                  u64 data_size);

//
//
//

/**
 * @brief Release a texture returned by @ref cheese_texture_upload.
 */
void cheese_texture_destroy(cheese_t *cheese, cheese_texture_t texture);

//
//
//

/**
 * @brief Push a scissor rect that clips subsequent draws.
 *
 * @param cheese The cheese context.
 * @param x,y,w,h The clip rect.
 */
void cheese_push_clip(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h);

//
//
//

/** @brief Pop the clip pushed by @ref cheese_push_clip. */
void cheese_pop_clip(cheese_t *cheese);

#endif // !CHEESE_RENDER_DRAW_H
