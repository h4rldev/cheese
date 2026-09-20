/***********************************/

#include <math.h>
#include <string.h>

#include <raylib.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/render/font.h>

#include <renderers/raylib.h>

/***********************************/

typedef struct {
  i32 id;
  Texture2D texture;
  u32 width, height;
} raylib_texture_t;

typedef struct {
  arena_t *arena;

  raylib_texture_t *textures; // Dynamic array.
  u32 texture_count, texture_cap;
  i32 next_texture_id;

  Rectangle clip_stack[32];
  u32 clip_depth;
} raylib_renderer_t;

static Color raylib_color(cheese_color_t color) {
  return (Color){
      .r = (u8)((color >> 16) & 0xFF),
      .g = (u8)((color >> 8) & 0xFF),
      .b = (u8)((color >> 0) & 0xFF),
      .a = (u8)((color >> 24) & 0xFF),
  };
}

static raylib_texture_t *raylib_find_texture(raylib_renderer_t *renderer,
                                             u32 texture_id) {
  for (u32 i = 0; i < renderer->texture_count; i++)
    if ((u32)renderer->textures[i].id == texture_id)
      return &renderer->textures[i];
  return null;
}

//
//
//

static void raylib_emit_glyph(void *userdata, const cheese_glyph_quad_t *quad) {
  raylib_renderer_t *renderer = (raylib_renderer_t *)userdata;
  raylib_texture_t *entry = raylib_find_texture(renderer, quad->texture_id);
  if (!entry)
    return;

  f32 tw = (f32)entry->width;
  f32 th = (f32)entry->height;
  Rectangle source = {
      quad->u0 * tw,
      quad->v0 * th,
      (quad->u1 - quad->u0) * tw,
      (quad->v1 - quad->v0) * th,
  };
  Rectangle dest = {quad->x, quad->y, quad->w, quad->h};

  DrawTexturePro(entry->texture, source, dest, (Vector2){0}, 0.0f,
                 raylib_color(quad->color));
}

//
//
//

static void raylib_draw_rect(void *userdata, cheese_corners_t radius, f32 x,
                             f32 y, f32 w, f32 h, cheese_color_t color) {
  (void)userdata;
  if (w <= 0.0f || h <= 0.0f)
    return;

  f32 tl = radius.top_left > 0 ? radius.top_left : 0.0f;
  f32 tr = radius.top_right > 0 ? radius.top_right : 0.0f;
  f32 br = radius.bottom_right > 0 ? radius.bottom_right : 0.0f;
  f32 bl = radius.bottom_left > 0 ? radius.bottom_left : 0.0f;
  f32 r = (tl + tr + br + bl) * 0.25f;

  if (r <= 0.0f) {
    DrawRectangleRec((Rectangle){x, y, w, h}, raylib_color(color));
    return;
  }

  f32 half = (w < h ? w : h) * 0.5f;
  f32 roundness = half > 0.0f ? r / half : 0.0f;
  if (roundness > 1.0f)
    roundness = 1.0f;
  DrawRectangleRounded((Rectangle){x, y, w, h}, roundness, 8,
                       raylib_color(color));
}

//
//
//

static void raylib_draw_border(void *userdata, cheese_corners_t radius, f32 x,
                               f32 y, f32 w, f32 h, f32 thickness, u32 sides,
                               cheese_color_t color) {
  (void)userdata;
  if (thickness <= 0.0f || w <= 0.0f || h <= 0.0f)
    return;

  if (sides == 0)
    sides = CHEESE_SIDE_ALL;
  Color c = raylib_color(color);

  f32 tl = radius.top_left > 0 ? radius.top_left : 0.0f;
  f32 tr = radius.top_right > 0 ? radius.top_right : 0.0f;
  f32 br = radius.bottom_right > 0 ? radius.bottom_right : 0.0f;
  f32 bl = radius.bottom_left > 0 ? radius.bottom_left : 0.0f;
  f32 r = (tl + tr + br + bl) * 0.25f;

  if (sides == CHEESE_SIDE_ALL && r > 0.0f) {
    f32 half = (w < h ? w : h) * 0.5f;
    f32 roundness = half > 0.0f ? r / half : 0.0f;
    if (roundness > 1.0f)
      roundness = 1.0f;
    DrawRectangleRoundedLinesEx((Rectangle){x, y, w, h}, roundness, 8,
                                thickness, c);
    return;
  }

  f32 t = thickness;
  if (t > w * 0.5f)
    t = w * 0.5f;
  if (t > h * 0.5f)
    t = h * 0.5f;

  if (sides & CHEESE_SIDE_TOP)
    DrawRectangleRec((Rectangle){x, y, w, t}, c);
  if (sides & CHEESE_SIDE_BOTTOM)
    DrawRectangleRec((Rectangle){x, y + h - t, w, t}, c);
  if (sides & CHEESE_SIDE_LEFT)
    DrawRectangleRec((Rectangle){x, y, t, h}, c);
  if (sides & CHEESE_SIDE_RIGHT)
    DrawRectangleRec((Rectangle){x + w - t, y, t, h}, c);
}

//
//
//

static void raylib_draw_texture(void *userdata, f32 x, f32 y, f32 w, f32 h,
                                u32 texture_id, cheese_color_t color) {
  raylib_renderer_t *renderer = (raylib_renderer_t *)userdata;
  raylib_texture_t *entry = raylib_find_texture(renderer, texture_id);
  if (!entry)
    return;

  Rectangle source = {0, 0, (f32)entry->width, (f32)entry->height};
  Rectangle dest = {x, y, w, h};
  DrawTexturePro(entry->texture, source, dest, (Vector2){0}, 0.0f,
                 raylib_color(color));
}

//
//
//

static void raylib_draw_line(void *userdata, f32 x1, f32 y1, f32 x2, f32 y2,
                             f32 thickness, cheese_color_t color) {
  (void)userdata;
  if (thickness < 1.0f)
    thickness = 1.0f;
  DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, thickness,
             raylib_color(color));
}

//
//
//

static void raylib_draw_arc(void *userdata, f32 cx, f32 cy, f32 radius,
                            f32 start_angle, f32 end_angle, f32 thickness,
                            cheese_color_t color) {
  (void)userdata;
  if (radius <= 0.0f)
    return;

  start_angle = fmodf(start_angle, 2.0f * PI);
  end_angle = fmodf(end_angle, 2.0f * PI);
  while (start_angle >= end_angle)
    end_angle += 2.0f * PI;

  f32 range = end_angle - start_angle;
  if (range <= 0.0f)
    return;

  i32 segments = (i32)(range / (2.0f * PI) * 36.0f);
  if (segments < 3)
    segments = 3;

  f32 a0 = start_angle * (180.0f / PI);
  f32 a1 = end_angle * (180.0f / PI);

  if (thickness <= 0.0f) {
    DrawCircleSector((Vector2){cx, cy}, radius, a0, a1, segments,
                     raylib_color(color));
    return;
  }

  f32 inner = radius - thickness;
  if (inner < 0.0f)
    inner = 0.0f;
  DrawRing((Vector2){cx, cy}, inner, radius, a0, a1, segments,
           raylib_color(color));
}

//
//
//

static void raylib_draw_text(void *userdata, f32 x, f32 y, const string *text,
                             cheese_font_t *font, cheese_color_t color,
                             f32 scale) {
  cheese_font_shape_run(font, text, x, y, scale, color, raylib_emit_glyph,
                        userdata);
}

//
//
//

static void raylib_push_clip(void *userdata, f32 x, f32 y, f32 w, f32 h) {
  raylib_renderer_t *renderer = (raylib_renderer_t *)userdata;
  if (renderer->clip_depth >= 32)
    return;

  Rectangle rect = {x, y, w, h};
  if (renderer->clip_depth > 0) {
    Rectangle parent = renderer->clip_stack[renderer->clip_depth - 1];
    f32 x1 = rect.x > parent.x ? rect.x : parent.x;
    f32 y1 = rect.y > parent.y ? rect.y : parent.y;
    f32 x2 = rect.x + rect.width;
    f32 px2 = parent.x + parent.width;
    f32 y2 = rect.y + rect.height;
    f32 py2 = parent.y + parent.height;
    if (px2 < x2)
      x2 = px2;
    if (py2 < y2)
      y2 = py2;
    if (x1 >= x2 || y1 >= y2)
      rect = (Rectangle){0, 0, 0, 0};
    else
      rect = (Rectangle){x1, y1, x2 - x1, y2 - y1};
  }

  renderer->clip_stack[renderer->clip_depth++] = rect;
  BeginScissorMode((i32)rect.x, (i32)rect.y, (i32)rect.width, (i32)rect.height);
}

//
//
//

static void raylib_pop_clip(void *userdata) {
  raylib_renderer_t *renderer = (raylib_renderer_t *)userdata;
  if (renderer->clip_depth == 0)
    return;

  renderer->clip_depth--;
  EndScissorMode();

  if (renderer->clip_depth > 0) {
    Rectangle parent = renderer->clip_stack[renderer->clip_depth - 1];
    BeginScissorMode((i32)parent.x, (i32)parent.y, (i32)parent.width,
                     (i32)parent.height);
  }
}

//
//
//

static i32 raylib_create_texture(void *userdata, u32 width, u32 height,
                                 cheese_texture_format_t format,
                                 const u8 *data) {
  raylib_renderer_t *renderer = (raylib_renderer_t *)userdata;

  Image image = {
      .data = (void *)data,
      .width = (i32)width,
      .height = (i32)height,
      .mipmaps = 1,
      .format = format == CHEESE_TEXTURE_R8 ? PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
                                            : PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
  };

  Texture2D texture = LoadTextureFromImage(image);
  if (texture.id == 0) {
    cheese_log_error("raylib: failed to upload texture %ux%u", width, height);
    return 0;
  }
  SetTextureFilter(texture, TEXTURE_FILTER_POINT);

  if (renderer->texture_count >= renderer->texture_cap) {
    u32 cap = renderer->texture_cap ? renderer->texture_cap * 2 : 16;
    raylib_texture_t *grown =
        arena_alloc(renderer->arena, raylib_texture_t, cap);
    if (renderer->texture_count > 0)
      memcpy(grown, renderer->textures,
             renderer->texture_count * sizeof(*grown));
    renderer->textures = grown;
    renderer->texture_cap = cap;
  }

  i32 id = renderer->next_texture_id++;
  renderer->textures[renderer->texture_count++] =
      (raylib_texture_t){id, texture, width, height};
  return id;
}

//
//
//

static void raylib_delete_texture(void *userdata, u32 texture_id) {
  raylib_renderer_t *renderer = (raylib_renderer_t *)userdata;
  for (u32 i = 0; i < renderer->texture_count; i++) {
    if ((u32)renderer->textures[i].id != texture_id)
      continue;
    UnloadTexture(renderer->textures[i].texture);
    renderer->textures[i] = renderer->textures[--renderer->texture_count];
    return;
  }
}

//
//
//

static void raylib_update_texture_region(void *userdata, u32 texture_id, i32 x,
                                         i32 y, u32 w, u32 h, const void *data,
                                         u64 data_size) {
  (void)data_size;
  raylib_renderer_t *renderer = (raylib_renderer_t *)userdata;
  raylib_texture_t *entry = raylib_find_texture(renderer, texture_id);
  if (!entry)
    return;

  UpdateTextureRec(entry->texture, (Rectangle){(f32)x, (f32)y, (f32)w, (f32)h},
                   data);
}

//
//
//

static void raylib_flush_deferred(void *userdata) { (void)userdata; }

//
//
//

static void raylib_flush_draws(void *userdata) { (void)userdata; }

//
//
//

cheese_renderer_t cheese_create_raylib_renderer(arena_t *arena) {
  raylib_renderer_t *renderer = arena_alloc_zeroed(arena, raylib_renderer_t, 1);
  renderer->arena = arena;
  renderer->next_texture_id = 1;

  cheese_renderer_t result = {0};
  result.userdata = renderer;
  result.sdf_text = false;

  result.draw_rect = raylib_draw_rect;
  result.draw_border = raylib_draw_border;
  result.draw_texture = raylib_draw_texture;
  result.draw_line = raylib_draw_line;
  result.draw_arc = raylib_draw_arc;
  result.draw_text = raylib_draw_text;
  result.push_clip = raylib_push_clip;
  result.pop_clip = raylib_pop_clip;
  result.create_texture = raylib_create_texture;
  result.delete_texture = raylib_delete_texture;
  result.update_texture_region = raylib_update_texture_region;
  result.flush_deferred = raylib_flush_deferred;
  result.flush_draws = raylib_flush_draws;

  return result;
}
