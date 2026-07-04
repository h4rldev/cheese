#include <assert.h>
#include <stdlib.h>

#include <ft2build.h>
#include <threads.h>
#include FT_FREETYPE_H

#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/darray.h>
#include <htils/string.h>

#include <cheese/font.h>
#include <cheese/log.h>
#include <cheese/types.h>

static ft_library_t g_ft_library = null;
static b32 g_font_system_initialized = false;

b32 cheese_font_system_init(void) {
  if (g_font_system_initialized)
    return true;

  ft_error_t error;

  if ((error = FT_Init_FreeType(&g_ft_library)) != 0) {
    cheese_log_error("Failed to initialize FreeType: %s",
                     FT_Error_String(error));
    return false;
  }

  g_font_system_initialized = true;
  cheese_log_debug("Font system initialized");
  return true;
}

void cheese_font_system_destroy(void) {
  if (!g_font_system_initialized)
    cheese_log_warning("Destroying font system when it wasn't initialized");

  FT_Done_FreeType(g_ft_library);
  g_font_system_initialized = false;
  g_ft_library = null;
  cheese_log_debug("Font system destroyed");
}

static int compare_glyph_entry(const void *a, const void *b) {
  const cheese_glyph_entry_t *const *pa = a;
  const cheese_glyph_entry_t *const *pb = b;
  const cheese_glyph_entry_t *ga = *pa;
  const cheese_glyph_entry_t *gb = *pb;
  return (ga->glyph_id > gb->glyph_id) - (ga->glyph_id < gb->glyph_id);
}

static cheese_glyph_entry_t *
cheese_variant_find_entry(cheese_font_size_variant_t *variant, u32 id) {
  u64 glyph_count = da_len(variant->glyphs);
  if (glyph_count == 0)
    return null;

  i64 lo = 0;
  i64 hi = (i64)glyph_count - 1;

  while (lo <= hi) {
    i64 mid = lo + (hi - lo) / 2;
    cheese_glyph_entry_t *mid_entry = variant->glyphs[mid];

    if (mid_entry->glyph_id == id)
      return mid_entry;

    if (mid_entry->glyph_id < id)
      lo = mid + 1;
    else
      hi = mid - 1;
  }

  for (u64 i = 0; i < glyph_count; ++i) {
    if (variant->glyphs[i]->glyph_id == id)
      return variant->glyphs[i];
  }

  return null;
}

static b32
cheese_variant_load_and_cache_glyph(cheese_font_size_variant_t *variant,
                                    ft_face_t ft_face, arena_t *arena, u32 id,
                                    b32 is_notdef) {
  if (!id)
    return false;
  if (cheese_variant_find_entry(variant, id))
    return true;

  ft_error_t ft_error;
  if ((ft_error = FT_Load_Glyph(ft_face, id, FT_LOAD_RENDER)) != 0) {
    cheese_log_error("Failed to load glyph %u: %s", id,
                     FT_Error_String(ft_error));
    return false;
  }

  ft_glyph_slot_t slot = ft_face->glyph;
  ft_bitmap_t *bitmap = &slot->bitmap;

  cheese_glyph_entry_t *entry =
      arena_alloc_zeroed(arena, cheese_glyph_entry_t, 1);
  entry->glyph_id = id;
  entry->is_notdef = is_notdef;
  entry->glyph.advance = (f32)slot->advance.x / 64.0f;
  entry->glyph.bearing_x = (f32)slot->bitmap_left;
  entry->glyph.bearing_y = (f32)slot->bitmap_top;
  if (bitmap->width > 0 && bitmap->rows > 0) {
    u32 width = bitmap->width;
    u32 rows = bitmap->rows;
    u32 pitch = bitmap->pitch;

    entry->glyph.width = (f32)width;
    entry->glyph.height = (f32)rows;
    entry->temp_bitmap_data = arena_alloc(arena, u8, width * rows);

    u32 y = 0;
    for (y = 0; y < rows; y++) {
      u8 *src_row = bitmap->buffer + (y * pitch);
      u8 *dst_row = entry->temp_bitmap_data + (y * width);
      memcpy(dst_row, src_row, width);
    }
  }

  da_append(arena, variant->glyphs, entry);

  qsort(variant->glyphs, da_len(variant->glyphs),
        sizeof(cheese_glyph_entry_t *), compare_glyph_entry);

  variant->atlas_dirty = true;

  return true;
}

typedef struct {
  i32 x, y;
} skyline_node_t;

static int compare_glyph_height_desc(const void *a, const void *b) {
  const cheese_glyph_entry_t *ga = *(const cheese_glyph_entry_t **)a;
  const cheese_glyph_entry_t *gb = *(const cheese_glyph_entry_t **)b;

  if (ga->glyph.height > gb->glyph.height)
    return -1;
  if (ga->glyph.height < gb->glyph.height)
    return 1;
  return 0;
}

static inline void get_best(skyline_node_t *sl, u32 sl_len, u32 pad, u32 w,
                            u32 h, i32 gw, i32 gh, i32 *best_x, i32 *best_y) {
  *best_y = INT32_MAX;
  *best_x = -1;

  for (u32 si = 0; si < sl_len; si++) {
    i32 cur_x = sl[si].x;
    if (cur_x + gw + (i32)pad > (i32)w)
      continue;

    i32 max_y = sl[si].y;
    for (u32 sj = si; sj < sl_len && sl[sj].x < cur_x + gw + (i32)pad; sj++)
      if (sl[sj].y > max_y)
        max_y = sl[sj].y;

    if (max_y + gh + (i32)pad <= (i32)h && max_y < *best_y) {
      *best_y = max_y;
      *best_x = cur_x;
    }
  }
}
static b32 cheese_skyline_pack(cheese_glyph_entry_t **entries, u64 count,
                               u32 *out_width, u32 *out_height, u8 **out_pixels,
                               arena_t *arena) {
  u32 width = 512, height = 512;
  u8 *pixels = calloc(width * height * 4, sizeof(u8));

  skyline_node_t skyline[2048];
  u32 skyline_len = 1;
  skyline[0] = (skyline_node_t){0, 0};
  u32 padding = 2;

  qsort(entries, count, sizeof(cheese_glyph_entry_t *),
        compare_glyph_height_desc);

  for (u64 i = 0; i < count; i++) {
    cheese_glyph_entry_t *entry = entries[i];
    if (entry->glyph_id == 48)
      cheese_log_debug("Packing M.");

    i32 gw = (i32)entry->glyph.width, gh = (i32)entry->glyph.height;
    if (gw <= 0 || gh <= 0 || !entry->temp_bitmap_data) {
      entry->atlas_x = -1;
      continue;
    }

    i32 best_x = -1, best_y = INT32_MAX;
    get_best(skyline, skyline_len, padding, width, height, gw, gh, &best_x,
             &best_y);

    while (best_x < 0 || best_y + gh + (i32)padding > (i32)height ||
           best_x + gw + (i32)padding > (i32)width) {
      u32 new_width = width, new_height = height * 2;
      if (new_height > 8192) {
        free(pixels);
        return false;
      }

      u8 *new_pixels = calloc(new_width * new_height * 4, sizeof(u8));

      for (u32 y = 0; y < height; y++)
        memcpy(&new_pixels[y * new_width * 4], &pixels[y * width * 4],
               width * 4);

      free(pixels);

      pixels = new_pixels;
      width = new_width;
      height = new_height;
      skyline_len = 1;
      skyline[0] = (skyline_node_t){0, 0};
      best_x = -1;
      best_y = INT32_MAX;
      get_best(skyline, skyline_len, padding, width, height, gw, gh, &best_x,
               &best_y);
    }

    for (i32 y = 0; y < gh; y++)
      for (i32 x = 0; x < gw; x++) {
        u8 pixel = entry->temp_bitmap_data[y * gw + x];
        u32 idx = ((best_y + y) * width + (best_x + x)) * 4;
        pixels[idx] = pixels[idx + 1] = pixels[idx + 2] = 255;
        pixels[idx + 3] = pixel;
      }

    entry->atlas_x = best_x;
    entry->atlas_y = best_y;

    i32 shelf_y = best_y + gh + padding;
    i32 end_x = best_x + gw + padding;
    i32 floor_y = 0;

    for (u32 j = 0; j < skyline_len; j++)
      if (skyline[j].x < end_x &&
          (j + 1 == skyline_len || skyline[j + 1].x >= end_x)) {
        floor_y = skyline[j].y;
        break;
      }

    u32 insert_idx = 0;
    while (insert_idx < skyline_len && skyline[insert_idx].x < best_x)
      insert_idx++;

    for (u32 j = skyline_len - 1; (i32)j >= (i32)insert_idx; j--)
      skyline[j + 2] = skyline[j];

    skyline[insert_idx] = (skyline_node_t){best_x, shelf_y};
    skyline[insert_idx + 1] = (skyline_node_t){end_x, floor_y};
    skyline_len += 2;

    u32 write_idx = 1;
    for (u32 j = 1; j < skyline_len; j++) {
      if (skyline[j].x < end_x && skyline[j].y <= shelf_y)
        continue;
      skyline[write_idx++] = skyline[j];
    }
    skyline_len = write_idx;
  }

  *out_pixels = arena_alloc(arena, u8, width * height * 4);
  memcpy(*out_pixels, pixels, width * height * 4);
  free(pixels);

  for (u64 k = 0; k < count; k++) {
    cheese_glyph_entry_t *entry = entries[k];
    if (entry->atlas_x >= 0) {
      f32 u0 = (f32)entry->atlas_x;
      f32 v0 = (f32)entry->atlas_y;
      f32 u1 = u0 + (f32)entry->glyph.width;
      f32 v1 = v0 + (f32)entry->glyph.height;

      entry->glyph.u0 = (u0) / (f32)width;
      entry->glyph.v0 = (v0) / (f32)height;
      entry->glyph.u1 = (u1) / (f32)width;
      entry->glyph.v1 = (v1) / (f32)height;
    }
  }

  *out_width = width;
  *out_height = height;
  return true;
}

static thread_local b32 g_atlas_rebuilding = false;

static b32 cheese_build_atlas(cheese_renderer_t *renderer,
                              cheese_font_size_variant_t *variant,
                              arena_t *arena) {
  u64 count = da_len(variant->glyphs);
  u32 atlas_width, atlas_height;
  u8 *atlas_data;

  if (!cheese_skyline_pack(variant->glyphs, count, &atlas_width, &atlas_height,
                           &atlas_data, arena)) {
    cheese_log_error("Failed to pack glyphs into skyline");
    return false;
  }

  qsort(variant->glyphs, da_len(variant->glyphs),
        sizeof(cheese_glyph_entry_t *), compare_glyph_entry);

  i32 texture_id = renderer->create_texture(renderer->userdata, atlas_width,
                                            atlas_height, atlas_data);
  if (texture_id <= 0) {
    cheese_log_error("Failed to create font atlas texture with size %ux%u",
                     atlas_width, atlas_height);
    return false;
  }

  cheese_log_debug("Created font atlas texture with id %d", texture_id);
  variant->atlas_texture_id = texture_id;
  variant->atlas_width = atlas_width;
  variant->atlas_height = atlas_height;

  cheese_log_debug("Built font atlas of %dx%d with %lu glyphs", atlas_width,
                   atlas_height, count);
  return true;
}

void cheese_font_rebuild_atlas(cheese_font_t *font) {
  cheese_font_size_variant_t *variant = font->active_variant;
  if (!variant || !font->renderer || !font->arena) {
    cheese_log_error(
        "cheese_font_rebuild_atlas: Invalid font, renderer, or arena");
    return;
  }

  if (!variant->atlas_dirty)
    return;

  if (g_atlas_rebuilding)
    return;

  g_atlas_rebuilding = true;

  if (!variant->atlas_dirty)
    return;

  if (variant->atlas_texture_id) {
    font->renderer->delete_texture(font->renderer->userdata,
                                   variant->atlas_texture_id);
    variant->atlas_texture_id = 0;
  }

  if (!cheese_build_atlas(font->renderer, variant, font->arena)) {
    cheese_log_error("Failed to rebuild font atlas");
    return;
  }

  variant->atlas_dirty = false;
  g_atlas_rebuilding = false;
}

void cheese_font_repopulate_default_glyphs(cheese_font_t *font) {
  cheese_font_size_variant_t *variant = font->active_variant;
  if (!variant)
    return;

  font->notdef_glyph_id = FT_Get_Char_Index(font->ft_face, 0);
  cheese_variant_load_and_cache_glyph(variant, font->ft_face, font->arena,
                                      font->notdef_glyph_id, true);

  for (u32 code_point = 32; code_point <= 255; code_point++) {
    u32 gid = FT_Get_Char_Index(font->ft_face, code_point);
    cheese_log_debug("Loading glyph with id %d", gid);
    if (gid)
      cheese_variant_load_and_cache_glyph(variant, font->ft_face, font->arena,
                                          gid, false);
  }
}

f32 cheese_font_measure_text(const cheese_font_t *font, const string *label) {
  if (!font || !font->active_variant || !label || label->len == 0)
    return 0.0f;

  hb_buffer_t *buf = hb_buffer_create();
  hb_buffer_add_utf8(buf, (const cstr *)label->base, (int)label->len, 0, -1);
  hb_buffer_guess_segment_properties(buf);

  hb_shape(font->active_variant->hb_font, buf, NULL, 0);

  u32 glyph_count;
  hb_glyph_position_t *glyph_pos =
      hb_buffer_get_glyph_positions(buf, &glyph_count);

  f32 total_width = 0.0f;
  for (u32 i = 0; i < glyph_count; i++) {
    total_width += (f32)glyph_pos[i].x_advance / 64.0f;
  }

  hb_buffer_destroy(buf);
  return total_width;
}

void cheese_font_set_size(cheese_font_t *font, u32 pixel_size) {
  if (!font)
    return;
  if (font->active_variant && font->active_variant->font_size == pixel_size)
    return;

  // Search existing variants
  for (u64 i = 0; i < da_len(font->variants); i++) {
    if (font->variants[i]->font_size == pixel_size) {
      font->active_variant = font->variants[i];
      hb_font_set_scale(font->active_variant->hb_font, pixel_size * 64,
                        pixel_size * 64);
      if (font->active_variant->atlas_dirty)
        cheese_font_rebuild_atlas(font);
      return;
    }
  }

  cheese_font_size_variant_t *var =
      arena_alloc_zeroed(font->arena, cheese_font_size_variant_t, 1);
  var->font_size = pixel_size;

  FT_Set_Pixel_Sizes(font->ft_face, 0, pixel_size);
  var->hb_font = hb_ft_font_create(font->ft_face, NULL);
  var->ascender = font->ft_face->size->metrics.ascender >> 6;
  var->descender = font->ft_face->size->metrics.descender >> 6;
  var->line_height = font->ft_face->size->metrics.height >> 6;
  var->atlas_dirty = true;
  da_new(font->arena, var->glyphs, 64);

  u32 nd = FT_Get_Char_Index(font->ft_face, 0);
  cheese_variant_load_and_cache_glyph(var, font->ft_face, font->arena, nd,
                                      true);
  for (u32 cp = 32; cp <= 255; cp++) {
    u32 gid = FT_Get_Char_Index(font->ft_face, cp);
    if (gid)
      cheese_variant_load_and_cache_glyph(var, font->ft_face, font->arena, gid,
                                          false);
  }

  if (!cheese_build_atlas(font->renderer, var, font->arena)) {
    cheese_log_error("Failed to build atlas for size %u", pixel_size);
    return;
  }
  var->atlas_dirty = false;

  da_append(font->arena, font->variants, var);
  hb_font_set_scale(var->hb_font, pixel_size * 64, pixel_size * 64);
  font->active_variant = var;
  cheese_log_debug("Created variant size %u (%llu glyphs)", pixel_size,
                   (unsigned long long)da_len(var->glyphs));
}

cheese_glyph_t *cheese_font_get_glyph(cheese_font_t *font, u32 glyph_id) {
  cheese_font_size_variant_t *var = font->active_variant;
  if (!var)
    return null;

  cheese_glyph_entry_t *entry = cheese_variant_find_entry(var, glyph_id);
  if (entry)
    return &entry->glyph;

  if (cheese_variant_load_and_cache_glyph(var, font->ft_face, font->arena,
                                          glyph_id, false)) {
    entry = cheese_variant_find_entry(var, glyph_id);
    if (entry)
      return &entry->glyph;
  }

  u32 notdef_glyph_id = font->notdef_glyph_id;
  if (notdef_glyph_id && glyph_id != notdef_glyph_id) {
    entry = cheese_variant_find_entry(var, notdef_glyph_id);

    if (!entry && cheese_variant_load_and_cache_glyph(
                      var, font->ft_face, font->arena, notdef_glyph_id, true)) {
      entry = cheese_variant_find_entry(var, notdef_glyph_id);
    }

    if (entry)
      return &entry->glyph;
  }

  return null;
}

cheese_font_t *cheese_load_font(cheese_renderer_t *renderer, arena_t *arena,
                                const string *path, u32 font_size) {
  if (!g_font_system_initialized || !g_ft_library) {
    cheese_log_info(
        "Font system not initialized before loading font, starting it");
    cheese_font_system_init();
  }

  ft_face_t ft_face;
  cstr *path_cstr = string_to_cstr(path);
  ft_error_t error;

  cheese_log_debug("Loading font with path '%s'", path_cstr);

  cheese_log_debug("Creating font face");
  if ((error = FT_New_Face(g_ft_library, path_cstr, 0, &ft_face)) != 0) {
    cheese_log_error("Failed to load font: %s", FT_Error_String(error));
    return null;
  }

  cheese_log_debug("Setting font size");
  if ((error = FT_Set_Pixel_Sizes(ft_face, 0, font_size)) != 0) {
    cheese_log_error("Failed to set font size: %s", FT_Error_String(error));
    return null;
  }

  cheese_log_debug("Creating font");
  cheese_font_t *font = arena_alloc_zeroed(arena, cheese_font_t, 1);
  font->renderer = renderer;
  font->ft_face = ft_face;
  font->arena = arena;
  font->default_size = font_size;

  da_new(arena, font->variants, 4);
  FT_Set_Pixel_Sizes(ft_face, 0, font_size);
  font->notdef_glyph_id = FT_Get_Char_Index(ft_face, 0);

  cheese_font_set_size(font, font_size);

  cheese_log_debug("Loaded font '%s' with size %u", path_cstr, font_size);
  return font;
}

void cheese_font_destroy(cheese_renderer_t *renderer, cheese_font_t *font) {
  if (!font)
    return;

  for (u64 i = 0; i < da_len(font->variants); i++) {
    cheese_font_size_variant_t *var = font->variants[i];
    if (var->atlas_texture_id)
      renderer->delete_texture(renderer->userdata, var->atlas_texture_id);
    if (var->hb_font)
      hb_font_destroy(var->hb_font);
  }

  if (font->ft_face)
    FT_Done_Face(font->ft_face);
}
