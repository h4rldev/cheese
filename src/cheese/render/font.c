/***********************************/

#include <assert.h>
#include <stdlib.h>

#include <ft2build.h>
#include <threads.h>

#include FT_MODULE_H
#include FT_FREETYPE_H

#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/darray.h>
#include <htils/string.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/render/atlas.h>
#include <cheese/render/font.h>

/***********************************/

#define CHEESE_SDF_BASE_SIZE 48
#define CHEESE_FONT_WIDTH_CACHE_SLOTS 128
#define CHEESE_FONT_WIDTH_CACHE_MAX_LEN 8192
#define CHEESE_FONT_WIDTH_CACHE_BUDGET (MiB(4))

static ft_library_t g_ft_library = null;
static b32 g_font_system_initialized = false;

static thread_local b32 g_atlas_rebuilding = false;

typedef struct {
  i32 x, y;
} skyline_node_t;

//
//
//

static int compare_glyph_entry(const void *a, const void *b) {
  const cheese_glyph_entry_t *const *pa = a;
  const cheese_glyph_entry_t *const *pb = b;
  const cheese_glyph_entry_t *ga = *pa;
  const cheese_glyph_entry_t *gb = *pb;
  return (ga->glyph_id > gb->glyph_id) - (ga->glyph_id < gb->glyph_id);
}

//
//
//

static int compare_glyph_height_desc(const void *a, const void *b) {
  const cheese_glyph_entry_t *ga = *(const cheese_glyph_entry_t **)a;
  const cheese_glyph_entry_t *gb = *(const cheese_glyph_entry_t **)b;

  if (ga->glyph.height > gb->glyph.height)
    return -1;
  if (ga->glyph.height < gb->glyph.height)
    return 1;
  return 0;
}

//
//
//

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

//
//
//

static b32 cheese_atlas_try_insert(cheese_font_size_variant_t *variant,
                                   cheese_renderer_t *renderer,
                                   cheese_glyph_entry_t *entry) {
  u32 gw = (u32)entry->glyph.width, gh = (u32)entry->glyph.height;
  if (gw == 0 || gh == 0 || !entry->temp_bitmap_data)
    return true;

  cheese_rect_t rect;
  if (!cheese_atlas_alloc(variant, gw, gh, &rect)) {
    cheese_log_error("cheese_atlas_try_insert: Failed to allocate rect");
    return false;
  }

  u64 bytes = variant->sdf ? (u64)gw * gh : (u64)gw * gh * 4;
  u8 *pixels = arena_alloc_zeroed(variant->arena, u8, bytes);

  if (variant->sdf)
    for (u32 i = 0; i < gw * gh; i++)
      pixels[i] = entry->temp_bitmap_data[i];
  else {
    for (u32 i = 0; i < gw * gh; i++) {
      pixels[i * 4] = 255;
      pixels[i * 4 + 1] = 255;
      pixels[i * 4 + 2] = 255;
      pixels[i * 4 + 3] = entry->temp_bitmap_data[i];
    }
  }

  renderer->update_texture_region(renderer->userdata, variant->atlas_texture_id,
                                  rect.x, rect.y, gw, gh, pixels, bytes);

  entry->atlas_x = rect.x;
  entry->atlas_y = rect.y;
  entry->glyph.u0 = (f32)rect.x / (f32)variant->atlas_width;
  entry->glyph.v0 = (f32)rect.y / (f32)variant->atlas_height;
  entry->glyph.u1 = (f32)(rect.x + (i32)gw) / (f32)variant->atlas_width;
  entry->glyph.v1 = (f32)(rect.y + (i32)gh) / (f32)variant->atlas_height;

  return true;
}

//
//
//

static b32 cheese_variant_load_and_cache_glyph(
    cheese_font_size_variant_t *variant, ft_face_t ft_face, arena_t *arena,
    arena_t *bitmap_arena, u32 id, b32 is_notdef) {
  if (!id)
    return false;
  if (cheese_variant_find_entry(variant, id))
    return true;

  ft_error_t ft_error;
  u32 load_flags = variant->sdf ? FT_LOAD_DEFAULT : FT_LOAD_RENDER;
  if ((ft_error = FT_Load_Glyph(ft_face, id, load_flags)) != 0) {
    cheese_log_error("Failed to load glyph %u: %s", id,
                     FT_Error_String(ft_error));
    return false;
  }

  if (variant->sdf &&
      (ft_error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_SDF)) != 0) {
    cheese_log_error("Failed to render SDF glyph %u: %s", id,
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
    entry->temp_bitmap_data = arena_alloc(bitmap_arena, u8, width * rows);

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

//
//
//

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

//
//
//

static b32 cheese_skyline_pack(cheese_glyph_entry_t **entries, u64 count,
                               b32 sdf, u32 min_dim, u32 *out_width,
                               u32 *out_height, u8 **out_pixels, arena_t *arena,
                               skyline_node_t *out_skyline,
                               u32 *out_skyline_len) {
  u64 total_area = 0;
  for (u64 i = 0; i < count; i++) {
    if (entries[i]->glyph.width > 0 && entries[i]->glyph.height > 0)
      total_area += (u64)entries[i]->glyph.width * entries[i]->glyph.height;
  }

  u32 width = min_dim ? min_dim : 64;
  u32 height = min_dim ? min_dim : 64;
  while ((u64)width * height < total_area * 2) {
    if (width == height)
      width *= 2;
    else
      height *= 2;
    if (width > 8192 || height > 8192)
      break;
  }

  u8 *pixels;
pack_again:
  pixels = arena_alloc_zeroed(arena, u8, width * height * (sdf ? 1u : 4u));
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
        return false;
      }

      width = new_width;
      height = new_height;
      goto pack_again;
    }

    for (i32 y = 0; y < gh; y++)
      for (i32 x = 0; x < gw; x++) {
        u8 pixel = entry->temp_bitmap_data[y * gw + x];
        u32 at = (u32)(best_y + y) * width + (u32)(best_x + x);
        if (sdf)
          pixels[at] = pixel;
        else {
          pixels[at * 4] = pixels[at * 4 + 1] = pixels[at * 4 + 2] = 255;
          pixels[at * 4 + 3] = pixel;
        }
      }

    entry->atlas_x = best_x;
    entry->atlas_y = best_y;

    i32 shelf_y = best_y + gh + padding;
    i32 end_x = best_x + gw + padding;

    u32 new_len = 0;
    skyline_node_t rebuilt[2048];
    for (u32 j = 0; j < skyline_len; j++) {
      i32 x0 = skyline[j].x;
      i32 x1 = (j + 1 < skyline_len) ? skyline[j + 1].x : (i32)width;

      if (x1 <= best_x || x0 >= end_x) {
        rebuilt[new_len++] = skyline[j];
        continue;
      }

      if (x0 < best_x)
        rebuilt[new_len++] = (skyline_node_t){x0, skyline[j].y};

      rebuilt[new_len++] =
          (skyline_node_t){(x0 > best_x ? x0 : best_x), shelf_y};

      if (x1 > end_x)
        rebuilt[new_len++] = (skyline_node_t){end_x, skyline[j].y};
    }

    skyline_len = 0;
    for (u32 j = 0; j < new_len; j++) {
      if (skyline_len && skyline[skyline_len - 1].y == rebuilt[j].y)
        continue;
      skyline[skyline_len++] = rebuilt[j];
    }
  }

  *out_skyline_len = skyline_len;
  memcpy(out_skyline, skyline, sizeof(skyline_node_t) * skyline_len);

  *out_pixels = pixels;

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

//
//
//

static b32 cheese_build_atlas(cheese_renderer_t *renderer,
                              cheese_font_size_variant_t *variant,
                              arena_t *arena) {
  u64 count = da_len(variant->glyphs);
  u32 atlas_width, atlas_height;
  u8 *atlas_data;

  skyline_node_t skyline[2048];
  u32 skyline_len = 0;

  if (!cheese_skyline_pack(variant->glyphs, count, variant->sdf,
                           variant->sdf ? 1024 : 0, &atlas_width, &atlas_height,
                           &atlas_data, arena, skyline, &skyline_len)) {
    cheese_log_error("cheese_build_atlas: Failed to pack glyphs into skyline");
    return false;
  }

  qsort(variant->glyphs, da_len(variant->glyphs),
        sizeof(cheese_glyph_entry_t *), compare_glyph_entry);

  variant->free_rects = null;
  da_new(variant->arena, variant->free_rects, 16);
  for (u32 i = 0; i < skyline_len; i++) {
    i32 x0 = skyline[i].x;
    i32 x1 = (i + 1 < skyline_len) ? skyline[i + 1].x : (i32)atlas_width;
    i32 y = skyline[i].y;
    if (y < (i32)atlas_height) {
      cheese_rect_t free = {x0, y, (u32)(x1 - x0), atlas_height - (u32)y};
      da_append(variant->arena, variant->free_rects, free);
    }
  }

  i32 texture_id = renderer->create_texture(
      renderer->userdata, atlas_width, atlas_height,
      variant->sdf ? CHEESE_TEXTURE_R8 : CHEESE_TEXTURE_RGBA8, atlas_data);
  if (texture_id <= 0) {
    cheese_log_error("cheese_build_atlas: Failed to create font atlas texture "
                     "with size %ux%u",
                     atlas_width, atlas_height);
    return false;
  }

  cheese_log_debug("cheese_build_atlas: Created font atlas texture with id %d",
                   texture_id);
  variant->atlas_texture_id = texture_id;
  variant->atlas_width = atlas_width;
  variant->atlas_height = atlas_height;

  cheese_log_debug(
      "cheese_built_atlas: Built font atlas of %dx%d with %lu glyphs",
      atlas_width, atlas_height, count);

  arena_clear(arena);
  return true;
}

//
//
//

static void cheese_font_build_sdf_variant(cheese_font_t *font) {
  u32 base = font->default_size ? font->default_size : CHEESE_SDF_BASE_SIZE;

  cheese_font_size_variant_t *var =
      arena_alloc_zeroed(font->arena, cheese_font_size_variant_t, 1);
  var->font_size = base;
  var->arena = font->arena;
  var->sdf = true;

  FT_Set_Pixel_Sizes(font->ft_face, 0, base);
  var->hb_font = hb_ft_font_create(font->ft_face, NULL);
  var->ascender = font->ft_face->size->metrics.ascender >> 6;
  var->descender = font->ft_face->size->metrics.descender >> 6;
  var->line_height = font->ft_face->size->metrics.height >> 6;
  var->atlas_dirty = true;

  font->base_size = base;
  font->base_ascender = var->ascender;
  font->base_descender = var->descender;
  font->base_line_height = var->line_height;

  da_new(font->arena, var->glyphs, 64);
  da_new(font->arena, var->free_rects, 16);

  arena_clear(font->scratch);

  u32 nd = FT_Get_Char_Index(font->ft_face, 0);
  cheese_variant_load_and_cache_glyph(var, font->ft_face, font->arena,
                                      font->scratch, nd, true);

  if (!cheese_build_atlas(font->renderer, var, font->scratch)) {
    cheese_log_error("cheese_font_build_sdf_variant: Failed to build atlas");
    return;
  }
  var->atlas_dirty = false;

  da_append(font->arena, font->variants, var);
  hb_font_set_scale(var->hb_font, base * 64, base * 64);
  font->active_variant = var;
  cheese_log_debug("cheese_font_build_sdf_variant: Base atlas at %upx (%llu "
                   "glyphs)",
                   base, (u64)da_len(var->glyphs));
}

//
//
//

/**
 * @brief Shapes @p text into a fresh HarfBuzz buffer.
 * @details Configures the buffer as left-to-right, common-script, English,
 * runs HarfBuzz once and returns the buffer through @p out. The caller owns it
 * and must destroy it.
 *
 * @param font The font to shape with.
 * @param text The UTF-8 text to shape.
 * @param out Receives the shaped buffer.
 * @pre @p font has an active variant with a HarfBuzz font.
 */
static void cheese_font_shape_buffer(cheese_font_t *font, const string *text,
                                     hb_buffer_t **out) {
  hb_buffer_t *buf = hb_buffer_create();
  hb_buffer_add_utf8(buf, (const cstr *)text->base, (int)text->len, 0, -1);
  hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
  hb_buffer_set_script(buf, HB_SCRIPT_COMMON);
  hb_buffer_set_language(buf, hb_language_from_string("en", -1));
  hb_shape(font->active_variant->hb_font, buf, NULL, 0);
  *out = buf;
}

/**
 * @brief Accumulates a shaped run's advances into cumulative byte widths.
 * @details Walks the glyphs in cluster order, adding each advance as its byte
 * boundary is passed, so @c out[i] holds the width of the @c [0, i) prefix
 * scaled by @c font->scale. A substring's width is therefore
 * @c out[end] - out[start]. When @p out is null nothing is written.
 *
 * @param font The font the run was shaped with.
 * @param info Shaped glyph info.
 * @param pos Shaped glyph positions.
 * @param count Number of glyphs in @p info / @p pos.
 * @param len Length of the shaped text, in bytes.
 * @param out Receives @c len + 1 cumulative widths, or null to skip.
 * @return The total scaled width of the run.
 */
static f32 cheese_font_fill_widths(cheese_font_t *font,
                                   const hb_glyph_info_t *info,
                                   const hb_glyph_position_t *pos, u32 count,
                                   u32 len, f32 *out) {
  f32 width = 0.0f;
  u32 g = 0;
  for (u32 i = 0; i <= len; i++) {
    while (g < count && info[g].cluster < i)
      width += (f32)pos[g++].x_advance / 64.0f;
    if (out)
      out[i] = width * font->scale;
  }
  return width * font->scale;
}

/**
 * @brief Emits one quad per glyph of a shaped run.
 * @details Advances a pen by each glyph's scaled advance and, for glyphs that
 * have coverage, calls @p emit with the glyph's screen-space quad.
 *
 * @param font The font the run was shaped with.
 * @param info Shaped glyph info.
 * @param pos Shaped glyph positions.
 * @param count Number of glyphs in @p info / @p pos.
 * @param x Pen origin x, in pixels.
 * @param y Pen origin y (baseline), in pixels.
 * @param scale Extra scale multiplied with @c font->scale.
 * @param color Colour passed to every emitted quad.
 * @param emit Receiver for each quad.
 * @param userdata Opaque pointer forwarded to @p emit.
 */
static void cheese_font_emit_run(cheese_font_t *font,
                                 const hb_glyph_info_t *info,
                                 const hb_glyph_position_t *pos, u32 count,
                                 f32 x, f32 y, f32 scale, cheese_color_t color,
                                 cheese_glyph_emit_fn emit, void *userdata) {
  cheese_font_size_variant_t *variant = font->active_variant;

  f32 cursor_x = x;
  f32 cursor_y = y;
  f32 s = scale * (font->scale > 0.0f ? font->scale : 1.0f);

  for (u32 i = 0; i < count; i++) {
    f32 x_advance = (f32)pos[i].x_advance / 64.0f;
    f32 y_advance = (f32)pos[i].y_advance / 64.0f;
    f32 x_offset = (f32)pos[i].x_offset / 64.0f;
    f32 y_offset = (f32)pos[i].y_offset / 64.0f;

    cheese_glyph_t *glyph = cheese_font_get_glyph(font, info[i].codepoint);
    if (!glyph || glyph->width == 0 || glyph->height == 0) {
      cursor_x += x_advance * s;
      cursor_y += y_advance * s;
      continue;
    }

    cheese_glyph_quad_t quad = {
        .x = cursor_x + (glyph->bearing_x + x_offset) * s,
        .y = cursor_y - (glyph->bearing_y + y_offset) * s,
        .w = glyph->width * s,
        .h = glyph->height * s,
        .u0 = glyph->u0,
        .v0 = glyph->v0,
        .u1 = glyph->u1,
        .v1 = glyph->v1,
        .texture_id = variant->atlas_texture_id,
        .color = color,
    };
    emit(userdata, &quad);

    cursor_x += x_advance * s;
    cursor_y += y_advance * s;
  }
}

/**
 * @brief Shapes @p text directly, without the cache.
 * @details Runs one HarfBuzz shape and fills @p out (when non-null) with
 * cumulative byte widths. Used for text too long to cache.
 *
 * @param font The font to shape with.
 * @param text The UTF-8 text to measure.
 * @param out Receives @c text->len + 1 cumulative widths, or null to skip.
 * @return The total scaled width of @p text.
 * @pre @p font has an active variant.
 */
static f32 cheese_font_shape_widths(cheese_font_t *font, const string *text,
                                    f32 *out) {
  u32 len = (u32)text->len;
  if (out)
    out[0] = 0.0f;
  if (len == 0)
    return 0.0f;

  hb_buffer_t *buf;
  cheese_font_shape_buffer(font, text, &buf);

  u32 count;
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buf, &count);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, &count);

  f32 width = cheese_font_fill_widths(font, info, pos, count, len, out);

  hb_buffer_destroy(buf);
  return width;
}

/**
 * @brief Returns the cached shape of @p text, shaping on a miss.
 * @details Memoizes both the cumulative widths and the HarfBuzz glyph run,
 * keyed by the exact text bytes plus the active variant and @c font->scale, so
 * repeated frames reuse one shape for measuring and for drawing. The returned
 * entry is owned by the font and stays valid until the cache's byte budget is
 * exceeded, at which point the whole cache is dropped and rebuilt on demand.
 *
 * @param font The font to shape with; the cache is updated in place.
 * @param text The UTF-8 text to shape.
 * @return The cache entry, or null when @p text is too long or the cache is
 *         unavailable (the caller then shapes directly).
 * @pre @p font has an active variant.
 */
static cheese_font_width_entry_t *cheese_font_run_cached(cheese_font_t *font,
                                                         const string *text) {
  if (!font->shape_arena || !font->width_cache)
    return null;

  u32 len = (u32)text->len;
  if (len > CHEESE_FONT_WIDTH_CACHE_MAX_LEN)
    return null;

  cheese_font_size_variant_t *variant = font->active_variant;
  f32 scale = font->scale;

  u64 hash = 14695981039346656037ull;
  for (u32 i = 0; i < len; i++) {
    hash ^= (u8)text->base[i];
    hash *= 1099511628211ull;
  }
  hash ^= (u64)(uintptr_t)variant;
  hash *= 1099511628211ull;
  hash ^= (u64)(i64)(scale * 100000.0f);
  hash *= 1099511628211ull;

  for (u32 i = 0; i < font->width_cache_count; i++) {
    cheese_font_width_entry_t *e = &font->width_cache[i];
    if (e->hash != hash || e->len != len || e->variant != variant ||
        e->scale != scale)
      continue;
    if (len > 0 && memcmp(e->bytes, text->base, len) != 0)
      continue;
    return e;
  }

  hb_buffer_t *buf;
  cheese_font_shape_buffer(font, text, &buf);

  u32 count;
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buf, &count);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, &count);

  u64 need =
      (u64)(len + 1) * sizeof(f32) + (len > 0 ? len : 1) +
      (u64)count * (sizeof(hb_glyph_info_t) + sizeof(hb_glyph_position_t));
  if (font->width_cache_bytes + need > CHEESE_FONT_WIDTH_CACHE_BUDGET) {
    arena_clear(font->shape_arena);
    font->width_cache_count = 0;
    font->width_cache_next = 0;
    font->width_cache_bytes = 0;
  }

  cheese_font_width_entry_t *e;
  if (font->width_cache_count < CHEESE_FONT_WIDTH_CACHE_SLOTS) {
    e = &font->width_cache[font->width_cache_count++];
  } else {
    e = &font->width_cache[font->width_cache_next];
    font->width_cache_next =
        (font->width_cache_next + 1) % CHEESE_FONT_WIDTH_CACHE_SLOTS;
  }

  e->hash = hash;
  e->len = len;
  e->variant = variant;
  e->scale = scale;
  e->bytes = arena_alloc(font->shape_arena, u8, (len > 0 ? len : 1));
  if (len > 0)
    memcpy(e->bytes, text->base, len);
  e->widths = arena_alloc(font->shape_arena, f32, (len + 1));
  e->info =
      arena_alloc(font->shape_arena, hb_glyph_info_t, (count > 0 ? count : 1));
  e->pos = arena_alloc(font->shape_arena, hb_glyph_position_t,
                       (count > 0 ? count : 1));
  e->glyph_count = count;
  font->width_cache_bytes += need;

  if (count > 0) {
    memcpy(e->info, info, (u64)count * sizeof(hb_glyph_info_t));
    memcpy(e->pos, pos, (u64)count * sizeof(hb_glyph_position_t));
  }
  cheese_font_fill_widths(font, info, pos, count, len, e->widths);

  hb_buffer_destroy(buf);
  return e;
}

//
//
//

b32 cheese_font_system_init(void) {
  if (g_font_system_initialized)
    return true;

  ft_error_t error;
  ft_uint_t sdf_spread = 8;

  if ((error = FT_Init_FreeType(&g_ft_library)) != 0) {
    cheese_log_error("Failed to initialize FreeType: %s",
                     FT_Error_String(error));
    return false;
  }

  FT_Property_Set(g_ft_library, "sdf", "spread", &sdf_spread);

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

  if (!cheese_build_atlas(font->renderer, variant, font->scratch)) {
    cheese_log_error("cheese_font_rebuild_atlas: Failed to rebuild font atlas");
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
                                      font->scratch, font->notdef_glyph_id,
                                      true);

  for (u32 code_point = 32; code_point <= 255; code_point++) {
    u32 gid = FT_Get_Char_Index(font->ft_face, code_point);
    cheese_log_debug(
        "cheese_font_repopulate_default_glyphs: Loading glyph with id %d", gid);
    if (gid)
      cheese_variant_load_and_cache_glyph(variant, font->ft_face, font->arena,
                                          font->scratch, gid, false);
  }
}

f32 cheese_font_measure_text(const cheese_font_t *font, const string *label) {
  if (!font || !font->active_variant || !label || label->len == 0)
    return 0.0f;

  cheese_font_width_entry_t *e =
      cheese_font_run_cached((cheese_font_t *)font, label);
  if (e)
    return e->widths[label->len];

  return cheese_font_shape_widths((cheese_font_t *)font, label, null);
}

void cheese_font_prefix_widths(const cheese_font_t *font, const string *text,
                               f32 *out) {
  if (!font || !font->active_variant || !text || !out)
    return;

  u32 len = (u32)text->len;
  out[0] = 0.0f;
  if (len == 0)
    return;

  cheese_font_width_entry_t *e =
      cheese_font_run_cached((cheese_font_t *)font, text);
  if (e) {
    memcpy(out, e->widths, (len + 1) * sizeof(f32));
    return;
  }

  cheese_font_shape_widths((cheese_font_t *)font, text, out);
}

void cheese_font_shape_run(cheese_font_t *font, const string *text, f32 x,
                           f32 y, f32 scale, cheese_color_t color,
                           cheese_glyph_emit_fn emit, void *userdata) {
  if (!font || !font->active_variant || !text || text->len == 0 || !emit)
    return;

  cheese_font_size_variant_t *variant = font->active_variant;
  if (!variant->atlas_texture_id || variant->atlas_dirty)
    return;

  cheese_font_width_entry_t *e = cheese_font_run_cached(font, text);
  if (e) {
    cheese_font_emit_run(font, e->info, e->pos, e->glyph_count, x, y, scale,
                         color, emit, userdata);
    return;
  }

  hb_buffer_t *buf = null;
  cheese_font_shape_buffer(font, text, &buf);

  u32 glyph_count;
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t *glyph_pos =
      hb_buffer_get_glyph_positions(buf, &glyph_count);

  cheese_font_emit_run(font, glyph_info, glyph_pos, glyph_count, x, y, scale,
                       color, emit, userdata);

  hb_buffer_destroy(buf);
}
void cheese_font_set_size(cheese_font_t *font, u32 pixel_size) {
  if (!font)
    return;

  if (font->sdf) {
    if (!font->active_variant)
      cheese_font_build_sdf_variant(font);
    if (!font->active_variant)
      return;

    font->logical_size = pixel_size;
    font->scale = (f32)pixel_size / (f32)font->base_size;

    cheese_font_size_variant_t *var = font->active_variant;
    var->ascender = (i32)((f32)font->base_ascender * font->scale + 0.5f);
    var->descender = (i32)((f32)font->base_descender * font->scale + 0.5f);
    var->line_height = (i32)((f32)font->base_line_height * font->scale + 0.5f);
    return;
  }

  font->scale = 1.0f;
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
  var->arena = font->arena;

  FT_Set_Pixel_Sizes(font->ft_face, 0, pixel_size);
  var->hb_font = hb_ft_font_create(font->ft_face, NULL);
  var->ascender = font->ft_face->size->metrics.ascender >> 6;
  var->descender = font->ft_face->size->metrics.descender >> 6;
  var->line_height = font->ft_face->size->metrics.height >> 6;
  var->atlas_dirty = true;

  da_new(font->arena, var->glyphs, 64);
  da_new(font->arena, var->free_rects, 16);

  arena_clear(font->scratch);

  u32 nd = FT_Get_Char_Index(font->ft_face, 0);
  cheese_variant_load_and_cache_glyph(var, font->ft_face, font->arena,
                                      font->scratch, nd, true);
  for (u32 cp = 32; cp <= 255; cp++) {
    u32 gid = FT_Get_Char_Index(font->ft_face, cp);
    if (gid)
      cheese_variant_load_and_cache_glyph(var, font->ft_face, font->arena,
                                          font->scratch, gid, false);
  }

  if (!cheese_build_atlas(font->renderer, var, font->scratch)) {
    cheese_log_error("cheese_font_set_size: Failed to build atlas for size %u",
                     pixel_size);
    return;
  }
  var->atlas_dirty = false;

  da_append(font->arena, font->variants, var);
  hb_font_set_scale(var->hb_font, pixel_size * 64, pixel_size * 64);
  font->active_variant = var;
  cheese_log_debug(
      "cheese_font_set_size: Created variant size %u (%llu glyphs)", pixel_size,
      (u64)da_len(var->glyphs));
}

cheese_glyph_t *cheese_font_get_glyph(cheese_font_t *font, u32 glyph_id) {
  cheese_font_size_variant_t *var = font->active_variant;
  if (!var)
    return null;

  cheese_glyph_entry_t *entry = cheese_variant_find_entry(var, glyph_id);
  if (entry) {
    return &entry->glyph;
  }

  if (cheese_variant_load_and_cache_glyph(var, font->ft_face, font->arena,
                                          font->scratch, glyph_id, false)) {
    entry = cheese_variant_find_entry(var, glyph_id);
    if (entry) {
      if (!cheese_atlas_try_insert(var, font->renderer, entry))
        var->atlas_dirty = true;
      else
        var->atlas_dirty = false;
    }
  }

  u32 notdef_glyph_id = font->notdef_glyph_id;
  if (notdef_glyph_id && glyph_id != notdef_glyph_id) {
    entry = cheese_variant_find_entry(var, notdef_glyph_id);

    if (!entry && cheese_variant_load_and_cache_glyph(
                      var, font->ft_face, font->arena, font->scratch,
                      notdef_glyph_id, true)) {
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
    cheese_log_info("cheese_load_font: Font system not initialized before "
                    "loading font, starting it");
    cheese_font_system_init();
  }

  ft_face_t ft_face;
  cstr *path_cstr = string_to_cstr(path);
  ft_error_t error;

  cheese_log_debug("cheese_load_font: Loading font with path '%s'", path_cstr);

  cheese_log_debug("cheese_load_font: Creating font face");
  if ((error = FT_New_Face(g_ft_library, path_cstr, 0, &ft_face)) != 0) {
    cheese_log_error("cheese_load_font: Failed to load font: %s",
                     FT_Error_String(error));
    return null;
  }

  cheese_log_debug("cheese_load_font: Setting font size");
  if ((error = FT_Set_Pixel_Sizes(ft_face, 0, font_size)) != 0) {
    cheese_log_error("cheese_load_font: Failed to set font size: %s",
                     FT_Error_String(error));
    return null;
  }

  cheese_log_debug("cheese_load_font: Creating font");
  cheese_font_t *font = arena_alloc_zeroed(arena, cheese_font_t, 1);
  font->renderer = renderer;
  font->ft_face = ft_face;
  font->arena = arena;
  font->scratch = arena_new(GiB(1), MiB(1));
  font->shape_arena = arena_new(MiB(8), MiB(1));
  font->width_cache = arena_alloc_zeroed(arena, cheese_font_width_entry_t,
                                         CHEESE_FONT_WIDTH_CACHE_SLOTS);
  font->default_size = font_size;
  font->scale = 1.0f;
  font->sdf = renderer ? renderer->sdf_text : false;

  da_new(arena, font->variants, 4);
  FT_Set_Pixel_Sizes(ft_face, 0, font_size);
  font->notdef_glyph_id = FT_Get_Char_Index(ft_face, 0);

  cheese_font_set_size(font, font_size);

  cheese_log_debug("cheese_load_font: Loaded font '%s' with size %u", path_cstr,
                   font_size);
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

  if (font->scratch)
    arena_free(font->scratch);

  if (font->shape_arena)
    arena_free(font->shape_arena);
}
