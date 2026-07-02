#include <assert.h>
#include <stdlib.h>

#include <ft2build.h>
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

static cheese_glyph_entry_t *cheese_font_find_entry(cheese_font_t *font,
                                                    u32 id) {
  u64 glyph_count = da_len(font->glyphs);
  if (glyph_count == 0)
    return null;

  i64 lo = 0;
  i64 hi = (i64)glyph_count - 1;

  while (lo <= hi) {
    i64 mid = lo + (hi - lo) / 2;
    // cheese_log_debug("Getting entry of %ld", mid);
    cheese_glyph_entry_t *mid_entry = font->glyphs[mid];

    if (mid_entry->glyph_id == id)
      return mid_entry;

    if (mid_entry->glyph_id < id)
      lo = mid + 1;
    else
      hi = mid - 1;
  }

  return null;
}

static b32 cheese_font_load_and_cache_glyph(cheese_font_t *font, u32 id,
                                            b32 is_notdef) {
  if (!id)
    return false;
  if (cheese_font_find_entry(font, id))
    return true;

  ft_error_t ft_error;
  if ((ft_error = FT_Load_Glyph(font->ft_face, id, FT_LOAD_RENDER)) != 0) {
    cheese_log_error("Failed to load glyph: %s", FT_Error_String(ft_error));
    return false;
  }

  ft_glyph_slot_t slot = font->ft_face->glyph;
  ft_bitmap_t *bitmap = &slot->bitmap;

  if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY)
    cheese_log_debug("Pixel mode is gray");

  cheese_glyph_entry_t *entry =
      arena_alloc_zeroed(font->arena, cheese_glyph_entry_t, 1);
  entry->glyph_id = id;
  entry->is_notdef = is_notdef;
  entry->glyph.advance = (f32)slot->advance.x / 64.0f;
  entry->glyph.bearing_x = (f32)slot->bitmap_left;
  entry->glyph.bearing_y = (f32)slot->bitmap_top;
  if (bitmap->width > 0 && bitmap->rows > 0) {
    u32 width = bitmap->width;
    u32 rows = bitmap->rows;
    u32 pitch = bitmap->pitch;

    entry->glyph.width = (f32)bitmap->width;
    entry->glyph.height = (f32)bitmap->rows;
    entry->temp_bitmap_data = arena_alloc(font->arena, u8, width * rows);

    u32 y = 0;
    for (y = 0; y < rows; y++) {
      u8 *src_row = bitmap->buffer + (y * pitch);
      u8 *dst_row = entry->temp_bitmap_data + (y * width);
      memcpy(dst_row, src_row, width);
    }
    cheese_log_debug("Copied row 0-%u with width %d, pitch %d", y, width,
                     pitch);
  }

  if (id >= 65 && id <= 90)
    cheese_log_debug("Glyph %u: first 4 pixels: %u %u %u %u", id,
                     entry->temp_bitmap_data[0], entry->temp_bitmap_data[1],
                     entry->temp_bitmap_data[2], entry->temp_bitmap_data[3]);

  da_append(font->arena, font->glyphs, entry);

  qsort(font->glyphs, da_len(font->glyphs), sizeof(cheese_glyph_entry_t *),
        compare_glyph_entry);

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

static inline void get_best(skyline_node_t *skyline, u32 skyline_len,
                            u32 padding, u32 width, u32 height, i32 gw, i32 gh,
                            i32 *best_x, i32 *best_y) {
  *best_y = INT32_MAX;
  *best_x = -1;

  for (u32 si = 0; si < skyline_len; si++) {
    i32 cur_x = skyline[si].x;
    i32 cur_y = skyline[si].y;

    if (cur_x + gw + padding <= width) {
      i32 max_y = cur_y;
      for (u32 sj = si;
           sj < skyline_len && skyline[sj].x < cur_x + gw + (i32)padding; sj++)
        if (skyline[sj].y > max_y)
          max_y = skyline[sj].y;

      if (max_y + gh + padding <= height)
        if (max_y < *best_y) {
          *best_y = max_y;
          *best_x = cur_x;
        }
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
    cheese_log_debug("Packing glyph %u (h: %f)", entry->glyph_id,
                     entry->glyph.height);

    i32 gw = (i32)entry->glyph.width, gh = (i32)entry->glyph.height;
    if (gw <= 0 || gh <= 0 || !entry->temp_bitmap_data) {
      entry->atlas_x = -1;
      continue;
    }

    i32 best_x = -1, best_y = INT32_MAX;
    get_best(skyline, skyline_len, padding, width, height, gw, gh, &best_x,
             &best_y);

    while (best_x == -1 || best_y + gh + padding > height ||
           best_x + gw + padding > width) {
      u32 new_width = width, new_height = height * 2;
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
      if (height > 8192) {
        free(pixels);
        return false;
      }
    }

    cheese_log_debug("Glyph %u placed at (%d, %d) [w:%d, h:%d]",
                     entry->glyph_id, best_x, best_y, gw, gh);

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

static b32 cheese_build_atlas(cheese_renderer_t *renderer,
                              cheese_font_t *font) {
  u64 count = da_len(font->glyphs);
  u32 atlas_width, atlas_height;
  u8 *atlas_data;

  if (!cheese_skyline_pack(font->glyphs, count, &atlas_width, &atlas_height,
                           &atlas_data, font->arena)) {
    cheese_log_error("Failed to pack glyphs into skyline");
    return false;
  }

  if (atlas_data) {
    cheese_log_debug("Atlas first 16 pixels: %u %u %u %u %u %u %u %u %u %u %u "
                     "%u %u %u %u %u",
                     atlas_data[0], atlas_data[1], atlas_data[2], atlas_data[3],
                     atlas_data[4], atlas_data[5], atlas_data[6], atlas_data[7],
                     atlas_data[8], atlas_data[9], atlas_data[10],
                     atlas_data[11], atlas_data[12], atlas_data[13],
                     atlas_data[14], atlas_data[15]);
  }

  cheese_glyph_entry_t *entry = cheese_font_find_entry(font, 77);
  if (entry) {
    u32 ux = (u32)(entry->glyph.u0 * atlas_width);
    u32 uy = (u32)(entry->glyph.v0 * atlas_height);
    u32 idx = (uy * atlas_width + ux) * 4;
    cheese_log_debug("Glyph 77 atlas pixel at (%u,%u): %u %u %u %u", ux, uy,
                     atlas_data[idx], atlas_data[idx + 1], atlas_data[idx + 2],
                     atlas_data[idx + 3]);
  }

  i32 texture_id = renderer->create_texture(renderer->userdata, atlas_width,
                                            atlas_height, atlas_data);
  if (texture_id <= 0) {
    cheese_log_error("Failed to create font atlas texture with size %ux%u",
                     atlas_width, atlas_height);
    return false;
  }

  cheese_log_debug("Created font atlas texture with id %d", texture_id);
  font->atlas_texture_id = texture_id;
  font->atlas_width = atlas_width;
  font->atlas_height = atlas_height;

  cheese_log_debug("Built font atlas of %dx%d with %lu glyphs", atlas_width,
                   atlas_height, count);
  return true;
}

void cheese_font_rebuild_atlas(cheese_font_t *font) {
  if (!font || !font->renderer || !font->arena) {
    cheese_log_error(
        "cheese_font_rebuild_atlas: Invalid font, renderer, or arena");
    return;
  }

  if (font->atlas_baking)
    return;

  font->atlas_baking = true;

  if (!font->atlas_dirty)
    return;

  if (font->atlas_texture_id) {
    font->renderer->delete_texture(font->renderer->userdata,
                                   font->atlas_texture_id);
    font->atlas_texture_id = 0;
  }

  if (!cheese_build_atlas(font->renderer, font)) {
    cheese_log_error("Failed to rebuild font atlas");
    return;
  }

  cheese_log_debug("Setting font atlas to no longer be dirty");
  font->atlas_dirty = false;
  font->atlas_baking = false;
}

f32 cheese_font_measure_text(const cheese_font_t *font, const string *label) {
  if (!font || !label || label->len == 0)
    return 0.0f;

  hb_buffer_t *buf = hb_buffer_create();
  hb_buffer_add_utf8(buf, (const cstr *)label->base, (int)label->len, 0, -1);
  hb_buffer_guess_segment_properties(buf);

  hb_shape(font->hb_font, buf, NULL, 0);

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
  if (font->font_size == pixel_size)
    return;

  FT_Set_Pixel_Sizes(font->ft_face, 0, pixel_size);
  font->font_size = pixel_size;

  da_clear(font->glyphs);
  cheese_font_repopulate_default_glyphs(font);
  hb_font_set_scale(font->hb_font, pixel_size * 64, pixel_size * 64);

  font->atlas_dirty = true;
  cheese_font_rebuild_atlas(font);
}

void cheese_font_repopulate_default_glyphs(cheese_font_t *font) {
  font->notdef_glyph_id = FT_Get_Char_Index(font->ft_face, 0);
  cheese_font_load_and_cache_glyph(font, font->notdef_glyph_id, true);

  cheese_log_debug("Loading glyphs with ids [32, 255]");
  for (u32 code_point = 32; code_point <= 255; code_point++) {
    u32 gid = FT_Get_Char_Index(font->ft_face, code_point);
    cheese_log_debug("Loading glyph with id %d", gid);
    if (gid)
      cheese_font_load_and_cache_glyph(font, gid, false);
  }
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

  cheese_log_debug("Creating harfbuzz font");
  hb_font_t *hb_font = hb_ft_font_create(ft_face, null);

  cheese_log_debug("Creating font");
  cheese_font_t *font = arena_alloc_zeroed(arena, cheese_font_t, 1);
  font->renderer = renderer;
  font->ft_face = ft_face;
  font->hb_font = hb_font;

  cheese_log_debug("Setting font size to %d", font_size);
  font->font_size = font_size;
  cheese_log_debug("Setting ascender to %d",
                   font->ft_face->size->metrics.ascender >> 6);
  font->ascender = font->ft_face->size->metrics.ascender >> 6;
  cheese_log_debug("Setting descender to %d",
                   font->ft_face->size->metrics.descender >> 6);
  font->descender = font->ft_face->size->metrics.descender >> 6;
  cheese_log_debug("Setting line height to %d",
                   font->ft_face->size->metrics.height >> 6);
  font->line_height = font->ft_face->size->metrics.height >> 6;
  font->arena = arena;

  cheese_log_debug("Creating glyphs array");
  da_new(arena, font->glyphs, 64);

  cheese_log_debug("Getting notdef glyph id");
  font->notdef_glyph_id = FT_Get_Char_Index(ft_face, 0);
  cheese_font_load_and_cache_glyph(font, font->notdef_glyph_id, true);

  cheese_log_debug("Loading glyphs with ids [32, 255]");
  for (u32 code_point = 32; code_point <= 255; code_point++) {
    u32 gid = FT_Get_Char_Index(ft_face, code_point);
    cheese_log_debug("Loading glyph with id %d", gid);
    if (gid)
      cheese_font_load_and_cache_glyph(font, gid, false);
  }

  if (!cheese_build_atlas(renderer, font)) {
    cheese_log_error("Failed to build font atlas");
    cheese_font_destroy(renderer, font);
    return null;
  }

  font->atlas_dirty = false;

  qsort(font->glyphs, da_len(font->glyphs), sizeof(cheese_glyph_entry_t *),
        compare_glyph_entry);

  cheese_log_debug("Loaded font '%s' with %lu glyphs in atlas", path_cstr,
                   da_len(font->glyphs));
  return font;
}

cheese_glyph_t *cheese_font_get_glyph(cheese_font_t *font, u32 glyph_id) {
  cheese_glyph_entry_t *entry = cheese_font_find_entry(font, glyph_id);
  if (entry)
    return &entry->glyph;

  if (cheese_font_load_and_cache_glyph(font, glyph_id, false)) {
    // font->atlas_dirty = true;
    entry = cheese_font_find_entry(font, glyph_id);
    if (entry)
      return &entry->glyph;
  }

  u32 notdef_glyph_id = font->notdef_glyph_id;
  if (notdef_glyph_id && glyph_id != notdef_glyph_id) {
    entry = cheese_font_find_entry(font, notdef_glyph_id);

    if (!entry &&
        cheese_font_load_and_cache_glyph(font, notdef_glyph_id, true)) {
      // font->atlas_dirty = true;
      entry = cheese_font_find_entry(font, notdef_glyph_id);
    }

    if (entry)
      return &entry->glyph;
  }

  return null;
}

void cheese_font_destroy(cheese_renderer_t *renderer, cheese_font_t *font) {
  if (!font)
    return;
  if (font->atlas_texture_id)
    renderer->delete_texture(renderer->userdata, font->atlas_texture_id);

  if (font->hb_font)
    hb_font_destroy(font->hb_font);

  if (font->ft_face)
    FT_Done_Face(font->ft_face);
}
