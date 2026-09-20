#ifndef CHEESE_RENDER_FONT_H
#define CHEESE_RENDER_FONT_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Initialise the shared font system (FreeType + HarfBuzz).
 * @details Idempotent; call before loading any font.
 *
 * @return True on success.
 */
b32 cheese_font_system_init(void);

//
//
//

/** @brief Tear down the shared font system. */
void cheese_font_system_destroy(void);

//
//
//

/**
 * @brief Load a font at a base pixel size.
 *
 * @param renderer The renderer the font's atlas is uploaded through.
 * @param arena The persistent arena the font lives in.
 * @param path The font file path.
 * @param font_size The base pixel size.
 *
 * @return The font, or null on failure.
 */
cheese_font_t *cheese_load_font(cheese_renderer_t *renderer, arena_t *arena,
                                const string *path, u32 font_size);

//
//
//

/**
 * @brief Destroy a font and release its atlas.
 *
 * @param renderer The renderer the font was loaded through.
 * @param font The font to destroy.
 */
void cheese_font_destroy(cheese_renderer_t *renderer, cheese_font_t *font);

//
//
//

/**
 * @brief Set the font's active logical pixel size.
 * @details One base atlas serves every size; this rescales how it is drawn.
 *
 * @param font The font.
 * @param pixel_size The new logical size.
 */
void cheese_font_set_size(cheese_font_t *font, u32 pixel_size);

//
//
//

/**
 * @brief Get a glyph by codepoint, rasterizing and packing it on demand.
 *
 * @param font The font.
 * @param glyph_id The glyph index.
 *
 * @return The glyph, or null if it can't be produced.
 */
cheese_glyph_t *cheese_font_get_glyph(cheese_font_t *font, u32 glyph_id);

//
//
//

/**
 * @brief Measure the advance width of @c label at the font's active size.
 *
 * @param font The font.
 * @param label The text to measure.
 *
 * @return The width in pixels.
 */
f32 cheese_font_measure_text(const cheese_font_t *font, const string *label);

//
//
//

/**
 * @brief Shape @c text and emit one quad per glyph.
 * @details Runs HarfBuzz once and calls @c emit for each glyph with its
 * destination quad and atlas coordinates, then advances by the glyph positions.
 * This is the shared text path for renderer backends: a backend maps each quad
 * to its own textured draw and never links HarfBuzz. A no-op when the font has
 * no ready atlas, or when @c text is empty.
 *
 * @param font The font (its active variant and atlas are used).
 * @param text The UTF-8 text to shape.
 * @param x,y The pen origin (baseline, matching @ref cheese_draw_text).
 * @param scale An extra scale on top of the font's logical size.
 * @param color The tint applied to every glyph.
 * @param emit Called once per glyph, in order.
 * @param userdata Passed through to @c emit.
 */
void cheese_font_shape_run(cheese_font_t *font, const string *text, f32 x,
                           f32 y, f32 scale, cheese_color_t color,
                           cheese_glyph_emit_fn emit, void *userdata);

//
//
//

/**
 * @brief Rebuild the active variant's atlas.
 * @details Repacks and re-uploads after the atlas was marked dirty.
 *
 * @param font The font.
 */
void cheese_font_rebuild_atlas(cheese_font_t *font);

//
//
//

/**
 * @brief Ensure the default glyph set is present in the active atlas.
 *
 * @param font The font.
 */
void cheese_font_repopulate_default_glyphs(cheese_font_t *font);

#endif // !CHEESE_RENDER_FONT_H
