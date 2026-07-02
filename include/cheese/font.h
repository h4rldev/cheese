#ifndef CHEESE_FONT_H
#define CHEESE_FONT_H

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

#include <cheese/types.h>

b32 cheese_font_system_init(void);
void cheese_font_system_destroy(void);
void cheese_font_repopulate_default_glyphs(cheese_font_t *font);
void cheese_font_set_size(cheese_font_t *font, u32 pixel_size);

f32 cheese_font_measure_text(const cheese_font_t *font, const string *label);
cheese_font_t *cheese_load_font(cheese_renderer_t *renderer, arena_t *arena,
                                const string *path, u32 font_size);
cheese_glyph_t *cheese_font_get_glyph(cheese_font_t *font, u32 glyph_id);

void cheese_font_rebuild_atlas(cheese_font_t *font);
void cheese_font_destroy(cheese_renderer_t *renderer, cheese_font_t *font);
#endif // !CHEESE_FONT_H
