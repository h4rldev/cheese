/***********************************/

#include <math.h>
#include <string.h>

#include <SDL2/SDL.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/render/font.h>

#include <renderers/sdl2.h>

/***********************************/

#define sdl2_MAX_VERTS 4096
#define sdl2_RECT_SEG 10
#define sdl2_ARC_SEG 36

typedef struct {
  i32 id;
  SDL_Texture *texture;
  u32 width, height;
} sdl2_texture_t;

typedef struct {
  SDL_Renderer *renderer;
  arena_t *arena;

  sdl2_texture_t *textures; // Dynamic array.
  u32 texture_count, texture_cap;
  i32 next_texture_id;

  SDL_Rect clip_stack[32];
  u32 clip_depth;

  SDL_Vertex *verts;
  u32 vert_count, vert_cap;
  SDL_Texture *pending_texture;
} sdl2_renderer_t;

static f32 sdl2_min(f32 a, f32 b) { return a < b ? a : b; }

static SDL_Color sdl2_color(cheese_color_t color) {
  return (SDL_Color){
      .r = (u8)((color >> 16) & 0xFF),
      .g = (u8)((color >> 8) & 0xFF),
      .b = (u8)((color >> 0) & 0xFF),
      .a = (u8)((color >> 24) & 0xFF),
  };
}

// Bilinearly samples the four-corner gradient at a screen-space point.
static SDL_Color sdl2_gradient_at(f32 px, f32 py, f32 x, f32 y, f32 w, f32 h,
                                  cheese_gradient_t g) {
  f32 u = w > 0.0f ? (px - x) / w : 0.0f;
  f32 v = h > 0.0f ? (py - y) / h : 0.0f;

  f32 w00 = (1.0f - u) * (1.0f - v);
  f32 w10 = u * (1.0f - v);
  f32 w01 = (1.0f - u) * v;
  f32 w11 = u * v;

  f32 r = w00 * (f32)((g.top_left >> 16) & 0xFF) +
          w10 * (f32)((g.top_right >> 16) & 0xFF) +
          w01 * (f32)((g.bottom_left >> 16) & 0xFF) +
          w11 * (f32)((g.bottom_right >> 16) & 0xFF);
  f32 gg = w00 * (f32)((g.top_left >> 8) & 0xFF) +
           w10 * (f32)((g.top_right >> 8) & 0xFF) +
           w01 * (f32)((g.bottom_left >> 8) & 0xFF) +
           w11 * (f32)((g.bottom_right >> 8) & 0xFF);
  f32 b = w00 * (f32)(g.top_left & 0xFF) + w10 * (f32)(g.top_right & 0xFF) +
          w01 * (f32)(g.bottom_left & 0xFF) +
          w11 * (f32)(g.bottom_right & 0xFF);
  f32 a = w00 * (f32)((g.top_left >> 24) & 0xFF) +
          w10 * (f32)((g.top_right >> 24) & 0xFF) +
          w01 * (f32)((g.bottom_left >> 24) & 0xFF) +
          w11 * (f32)((g.bottom_right >> 24) & 0xFF);

  return (SDL_Color){
      .r = (u8)(r + 0.5f),
      .g = (u8)(gg + 0.5f),
      .b = (u8)(b + 0.5f),
      .a = (u8)(a + 0.5f),
  };
}

static sdl2_texture_t *sdl2_find_texture(sdl2_renderer_t *renderer,
                                         u32 texture_id) {
  for (u32 i = 0; i < renderer->texture_count; i++)
    if ((u32)renderer->textures[i].id == texture_id)
      return &renderer->textures[i];
  return null;
}

//
//
//

static void vert_push(sdl2_renderer_t *renderer, f32 x, f32 y, f32 u, f32 v,
                      SDL_Color color) {
  if (renderer->vert_count >= renderer->vert_cap)
    return;

  renderer->verts[renderer->vert_count++] = (SDL_Vertex){
      .position = {x, y},
      .color = color,
      .tex_coord = {u, v},
  };
}

static void vert_flush(sdl2_renderer_t *renderer, SDL_Texture *texture) {
  if (renderer->vert_count == 0)
    return;

  SDL_RenderGeometry(renderer->renderer, texture, renderer->verts,
                     (i32)renderer->vert_count, null, 0);
  renderer->vert_count = 0;
}

static void tri(sdl2_renderer_t *renderer, SDL_Color c0, SDL_Color c1,
                SDL_Color c2, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2) {
  vert_push(renderer, x0, y0, 0, 0, c0);
  vert_push(renderer, x1, y1, 0, 0, c1);
  vert_push(renderer, x2, y2, 0, 0, c2);
}

static void quad(sdl2_renderer_t *renderer, SDL_Color color, f32 x0, f32 y0,
                 f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3) {
  tri(renderer, color, color, color, x0, y0, x1, y1, x2, y2);
  tri(renderer, color, color, color, x0, y0, x2, y2, x3, y3);
}

// Scales per-corner radii so no two adjacent radii overlap a side.
static cheese_corners_t sdl2_scale_radii(cheese_corners_t radius, f32 w,
                                         f32 h) {
  f32 tl = radius.top_left > 0 ? radius.top_left : 0.0f;
  f32 tr = radius.top_right > 0 ? radius.top_right : 0.0f;
  f32 br = radius.bottom_right > 0 ? radius.bottom_right : 0.0f;
  f32 bl = radius.bottom_left > 0 ? radius.bottom_left : 0.0f;

  f32 sc = 1.0f;
  if (tl + tr > w && tl + tr > 0)
    sc = sdl2_min(sc, w / (tl + tr));
  if (bl + br > w && bl + br > 0)
    sc = sdl2_min(sc, w / (bl + br));
  if (tl + bl > h && tl + bl > 0)
    sc = sdl2_min(sc, h / (tl + bl));
  if (tr + br > h && tr + br > 0)
    sc = sdl2_min(sc, h / (tr + br));

  return (cheese_corners_t){tl * sc, tr * sc, bl * sc, br * sc};
}

//
//
//

static void sdl2_draw_rect_gradient(void *userdata, cheese_corners_t radius,
                                    f32 x, f32 y, f32 w, f32 h,
                                    cheese_gradient_t colors) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  if (w <= 0.0f || h <= 0.0f)
    return;

  cheese_corners_t r = sdl2_scale_radii(radius, w, h);

  f32 tl = r.top_left, tr = r.top_right, br = r.bottom_right,
      bl = r.bottom_left;
  if (tl <= 0 && tr <= 0 && br <= 0 && bl <= 0) {
    SDL_Color ctl = sdl2_color(colors.top_left);
    SDL_Color ctr = sdl2_color(colors.top_right);
    SDL_Color cbl = sdl2_color(colors.bottom_left);
    SDL_Color cbr = sdl2_color(colors.bottom_right);
    tri(renderer, ctl, ctr, cbr, x, y, x + w, y, x + w, y + h);
    tri(renderer, ctl, cbr, cbl, x, y, x + w, y + h, x, y + h);
    vert_flush(renderer, null);
    return;
  }

  SDL_FPoint pts[4 * (sdl2_RECT_SEG + 1) + 1];
  u32 n = 0;

  struct {
    f32 cx, cy, cr, a0, a1;
  } corners[4] = {
      {x + tl, y + tl, tl, (f32)M_PI, 1.5f * (f32)M_PI},
      {x + w - tr, y + tr, tr, 1.5f * (f32)M_PI, 2.0f * (f32)M_PI},
      {x + w - br, y + h - br, br, 0.0f, 0.5f * (f32)M_PI},
      {x + bl, y + h - bl, bl, 0.5f * (f32)M_PI, (f32)M_PI},
  };

  for (u32 k = 0; k < 4; k++) {
    if (corners[k].cr <= 0.0f) {
      pts[n++] = (SDL_FPoint){corners[k].cx, corners[k].cy};
      continue;
    }
    for (u32 i = 0; i <= sdl2_RECT_SEG; i++) {
      f32 t = corners[k].a0 +
              (f32)i / (f32)sdl2_RECT_SEG * (corners[k].a1 - corners[k].a0);
      pts[n++] = (SDL_FPoint){corners[k].cx + corners[k].cr * cosf(t),
                              corners[k].cy + corners[k].cr * sinf(t)};
    }
  }

  f32 cx = x + w * 0.5f;
  f32 cy = y + h * 0.5f;
  SDL_Color cc = sdl2_gradient_at(cx, cy, x, y, w, h, colors);
  for (u32 i = 0; i < n; i++) {
    SDL_FPoint a = pts[i];
    SDL_FPoint b = pts[(i + 1) % n];
    SDL_Color ca = sdl2_gradient_at(a.x, a.y, x, y, w, h, colors);
    SDL_Color cb = sdl2_gradient_at(b.x, b.y, x, y, w, h, colors);
    tri(renderer, cc, ca, cb, cx, cy, a.x, a.y, b.x, b.y);
  }
  vert_flush(renderer, null);
}

// Flat-colour rect: delegates with four equal corner colours.
static void sdl2_draw_rect(void *userdata, cheese_corners_t radius, f32 x,
                           f32 y, f32 w, f32 h, cheese_color_t color) {
  sdl2_draw_rect_gradient(userdata, radius, x, y, w, h,
                          (cheese_gradient_t){color, color, color, color});
}

//
//
//

static void sdl2_border_edge(sdl2_renderer_t *renderer, SDL_Color c, f32 ox0,
                             f32 oy0, f32 ox1, f32 oy1, f32 ix0, f32 iy0,
                             f32 ix1, f32 iy1) {
  quad(renderer, c, ox0, oy0, ox1, oy1, ix1, iy1, ix0, iy0);
}

static void sdl2_border_arc(sdl2_renderer_t *renderer, SDL_Color c, f32 ocx,
                            f32 ocy, f32 ro, f32 icx, f32 icy, f32 ri, f32 a0,
                            f32 a1) {
  if (ro <= 0.0f)
    return;

  f32 pox = 0, poy = 0, pix = 0, piy = 0;
  for (u32 i = 0; i <= sdl2_RECT_SEG; i++) {
    f32 t = a0 + (f32)i / (f32)sdl2_RECT_SEG * (a1 - a0);
    f32 ox = ocx + ro * cosf(t);
    f32 oy = ocy + ro * sinf(t);
    f32 ix = icx + ri * cosf(t);
    f32 iy = icy + ri * sinf(t);
    if (i > 0)
      quad(renderer, c, pox, poy, ox, oy, ix, iy, pix, piy);
    pox = ox;
    poy = oy;
    pix = ix;
    piy = iy;
  }
}

static void sdl2_draw_border(void *userdata, cheese_corners_t radius, f32 x,
                             f32 y, f32 w, f32 h, f32 thickness, u32 sides,
                             cheese_color_t color) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  if (thickness <= 0.0f || w <= 0.0f || h <= 0.0f)
    return;

  if (sides == 0)
    sides = CHEESE_SIDE_ALL;
  SDL_Color c = sdl2_color(color);

  cheese_corners_t r = sdl2_scale_radii(radius, w, h);
  f32 tl = r.top_left, tr = r.top_right, br = r.bottom_right,
      bl = r.bottom_left;

  f32 t = thickness;
  if (t > w * 0.5f)
    t = w * 0.5f;
  if (t > h * 0.5f)
    t = h * 0.5f;

  f32 itl = tl > t ? tl - t : 0.0f;
  f32 itr = tr > t ? tr - t : 0.0f;
  f32 ibr = br > t ? br - t : 0.0f;
  f32 ibl = bl > t ? bl - t : 0.0f;

  f32 ix = x + t, iy = y + t, iw = w - 2.0f * t, ih = h - 2.0f * t;

  if (sides & CHEESE_SIDE_TOP)
    sdl2_border_edge(renderer, c, x + tl, y, x + w - tr, y, ix + itl, iy,
                     ix + iw - itr, iy);
  if (sides & (CHEESE_SIDE_TOP | CHEESE_SIDE_RIGHT))
    sdl2_border_arc(renderer, c, x + w - tr, y + tr, tr, ix + iw - itr,
                    iy + itr, itr, -0.5f * (f32)M_PI, 0.0f);
  if (sides & CHEESE_SIDE_RIGHT)
    sdl2_border_edge(renderer, c, x + w, y + tr, x + w, y + h - br, ix + iw,
                     iy + itr, ix + iw, iy + ih - ibr);
  if (sides & (CHEESE_SIDE_BOTTOM | CHEESE_SIDE_RIGHT))
    sdl2_border_arc(renderer, c, x + w - br, y + h - br, br, ix + iw - ibr,
                    iy + ih - ibr, ibr, 0.0f, 0.5f * (f32)M_PI);
  if (sides & CHEESE_SIDE_BOTTOM)
    sdl2_border_edge(renderer, c, x + w - br, y + h, x + bl, y + h,
                     ix + iw - ibr, iy + ih, ix + ibl, iy + ih);
  if (sides & (CHEESE_SIDE_BOTTOM | CHEESE_SIDE_LEFT))
    sdl2_border_arc(renderer, c, x + bl, y + h - bl, bl, ix + ibl,
                    iy + ih - ibl, ibl, 0.5f * (f32)M_PI, (f32)M_PI);
  if (sides & CHEESE_SIDE_LEFT)
    sdl2_border_edge(renderer, c, x, y + h - bl, x, y + tl, ix, iy + ih - ibl,
                     ix, iy + itl);
  if (sides & (CHEESE_SIDE_LEFT | CHEESE_SIDE_TOP))
    sdl2_border_arc(renderer, c, x + tl, y + tl, tl, ix + itl, iy + itl, itl,
                    (f32)M_PI, 1.5f * (f32)M_PI);

  vert_flush(renderer, null);
}

//
//
//

static void sdl2_draw_texture(void *userdata, f32 x, f32 y, f32 w, f32 h,
                              u32 texture_id, cheese_color_t color) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  sdl2_texture_t *entry = sdl2_find_texture(renderer, texture_id);
  if (!entry)
    return;

  SDL_Color c = sdl2_color(color);
  SDL_SetTextureColorMod(entry->texture, c.r, c.g, c.b);
  SDL_SetTextureAlphaMod(entry->texture, c.a);

  SDL_Rect src = {0, 0, (i32)entry->width, (i32)entry->height};
  SDL_FRect dst = {x, y, w, h};
  SDL_RenderCopyF(renderer->renderer, entry->texture, &src, &dst);
}

//
//
//

static void sdl2_draw_line(void *userdata, f32 x1, f32 y1, f32 x2, f32 y2,
                           f32 thickness, cheese_color_t color) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  SDL_Color c = sdl2_color(color);

  f32 dx = x2 - x1;
  f32 dy = y2 - y1;
  f32 len = sqrtf(dx * dx + dy * dy);
  if (len < 0.0001f)
    return;

  f32 half = thickness * 0.5f;
  if (half < 0.5f)
    half = 0.5f;

  f32 nx = -dy / len;
  f32 ny = dx / len;

  quad(renderer, c, x1 + nx * half, y1 + ny * half, x2 + nx * half,
       y2 + ny * half, x2 - nx * half, y2 - ny * half, x1 - nx * half,
       y1 - ny * half);
  vert_flush(renderer, null);
}

//
//
//

static void sdl2_draw_arc(void *userdata, f32 cx, f32 cy, f32 radius,
                          f32 start_angle, f32 end_angle, f32 thickness,
                          cheese_color_t color) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  if (radius <= 0.0f)
    return;

  SDL_Color c = sdl2_color(color);

  start_angle = fmodf(start_angle, 2.0f * (f32)M_PI);
  end_angle = fmodf(end_angle, 2.0f * (f32)M_PI);
  while (start_angle >= end_angle)
    end_angle += 2.0f * (f32)M_PI;

  f32 range = end_angle - start_angle;
  if (range <= 0.0f)
    return;

  u32 segments = (u32)(range / (2.0f * (f32)M_PI) * (f32)sdl2_ARC_SEG);
  if (segments < 3)
    segments = 3;

  if (thickness <= 0.0f) {
    for (u32 i = 0; i < segments; i++) {
      f32 t0 = start_angle + (f32)i / (f32)segments * range;
      f32 t1 = start_angle + (f32)(i + 1) / (f32)segments * range;
      tri(renderer, c, c, c, cx, cy, cx + radius * cosf(t0),
          cy + radius * sinf(t0), cx + radius * cosf(t1),
          cy + radius * sinf(t1));
    }
    vert_flush(renderer, null);
    return;
  }

  f32 inner = radius - thickness;
  if (inner < 0.0f)
    inner = 0.0f;

  for (u32 i = 0; i < segments; i++) {
    f32 t0 = start_angle + (f32)i / (f32)segments * range;
    f32 t1 = start_angle + (f32)(i + 1) / (f32)segments * range;
    f32 cos0 = cosf(t0), sin0 = sinf(t0);
    f32 cos1 = cosf(t1), sin1 = sinf(t1);
    quad(renderer, c, cx + radius * cos0, cy + radius * sin0,
         cx + radius * cos1, cy + radius * sin1, cx + inner * cos1,
         cy + inner * sin1, cx + inner * cos0, cy + inner * sin0);
  }
  vert_flush(renderer, null);
}

//
//
//

static void sdl2_emit_glyph(void *userdata, const cheese_glyph_quad_t *q) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  sdl2_texture_t *entry = sdl2_find_texture(renderer, q->texture_id);
  if (!entry)
    return;

  renderer->pending_texture = entry->texture;
  SDL_Color c = sdl2_color(q->color);

  vert_push(renderer, q->x, q->y, q->u0, q->v0, c);
  vert_push(renderer, q->x + q->w, q->y, q->u1, q->v0, c);
  vert_push(renderer, q->x, q->y + q->h, q->u0, q->v1, c);
  vert_push(renderer, q->x + q->w, q->y, q->u1, q->v0, c);
  vert_push(renderer, q->x + q->w, q->y + q->h, q->u1, q->v1, c);
  vert_push(renderer, q->x, q->y + q->h, q->u0, q->v1, c);
}

static void sdl2_draw_text(void *userdata, f32 x, f32 y, const string *text,
                           cheese_font_t *font, cheese_color_t color,
                           f32 scale) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  renderer->pending_texture = null;

  cheese_font_shape_run(font, text, x, y, scale, color, sdl2_emit_glyph,
                        userdata);

  if (renderer->pending_texture)
    vert_flush(renderer, renderer->pending_texture);
}

//
//
//

static void sdl2_push_clip(void *userdata, f32 x, f32 y, f32 w, f32 h) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  if (renderer->clip_depth >= 32)
    return;

  SDL_Rect rect = {(i32)x, (i32)y, (i32)w, (i32)h};
  if (renderer->clip_depth > 0) {
    SDL_Rect parent = renderer->clip_stack[renderer->clip_depth - 1];
    i32 x1 = rect.x > parent.x ? rect.x : parent.x;
    i32 y1 = rect.y > parent.y ? rect.y : parent.y;
    i32 x2 = rect.x + rect.w;
    i32 px2 = parent.x + parent.w;
    i32 y2 = rect.y + rect.h;
    i32 py2 = parent.y + parent.h;
    if (px2 < x2)
      x2 = px2;
    if (py2 < y2)
      y2 = py2;
    if (x1 >= x2 || y1 >= y2)
      rect = (SDL_Rect){0, 0, 0, 0};
    else
      rect = (SDL_Rect){x1, y1, x2 - x1, y2 - y1};
  }

  renderer->clip_stack[renderer->clip_depth++] = rect;
  SDL_RenderSetClipRect(renderer->renderer, &rect);
}

static void sdl2_pop_clip(void *userdata) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  if (renderer->clip_depth == 0)
    return;

  renderer->clip_depth--;
  if (renderer->clip_depth > 0) {
    SDL_Rect parent = renderer->clip_stack[renderer->clip_depth - 1];
    SDL_RenderSetClipRect(renderer->renderer, &parent);
  } else {
    SDL_RenderSetClipRect(renderer->renderer, null);
  }
}

//
//
//

static i32 sdl2_create_texture(void *userdata, u32 width, u32 height,
                               cheese_texture_format_t format, const u8 *data) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  if (format != CHEESE_TEXTURE_RGBA8) {
    cheese_log_error("sdl2: only RGBA8 textures are supported");
    return 0;
  }

  SDL_Texture *texture =
      SDL_CreateTexture(renderer->renderer, SDL_PIXELFORMAT_RGBA32,
                        SDL_TEXTUREACCESS_STATIC, (i32)width, (i32)height);
  if (!texture) {
    cheese_log_error("sdl2: SDL_CreateTexture failed: %s", SDL_GetError());
    return 0;
  }

  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
  SDL_UpdateTexture(texture, null, data, (i32)(width * 4));

  if (renderer->texture_count >= renderer->texture_cap) {
    u32 cap = renderer->texture_cap ? renderer->texture_cap * 2 : 16;
    sdl2_texture_t *grown = arena_alloc(renderer->arena, sdl2_texture_t, cap);
    if (renderer->texture_count > 0)
      memcpy(grown, renderer->textures,
             renderer->texture_count * sizeof(*grown));
    renderer->textures = grown;
    renderer->texture_cap = cap;
  }

  i32 id = renderer->next_texture_id++;
  renderer->textures[renderer->texture_count++] =
      (sdl2_texture_t){id, texture, width, height};
  return id;
}

static void sdl2_delete_texture(void *userdata, u32 texture_id) {
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  for (u32 i = 0; i < renderer->texture_count; i++) {
    if ((u32)renderer->textures[i].id != texture_id)
      continue;
    SDL_DestroyTexture(renderer->textures[i].texture);
    renderer->textures[i] = renderer->textures[--renderer->texture_count];
    return;
  }
}

static void sdl2_update_texture_region(void *userdata, u32 texture_id, i32 x,
                                       i32 y, u32 w, u32 h, const void *data,
                                       u64 data_size) {
  (void)data_size;
  sdl2_renderer_t *renderer = (sdl2_renderer_t *)userdata;
  sdl2_texture_t *entry = sdl2_find_texture(renderer, texture_id);
  if (!entry)
    return;

  SDL_Rect rect = {x, y, (i32)w, (i32)h};
  SDL_UpdateTexture(entry->texture, &rect, data, (i32)(w * 4));
}

//
//
//

static void sdl2_flush_deferred(void *userdata) { (void)userdata; }

//
//
//

static void sdl2_flush_draws(void *userdata) { (void)userdata; }

//
//
//

cheese_renderer_t cheese_create_sdl2_renderer(SDL_Renderer *renderer,
                                              arena_t *arena) {
  sdl2_renderer_t *r = arena_alloc_zeroed(arena, sdl2_renderer_t, 1);
  r->renderer = renderer;
  r->arena = arena;
  r->next_texture_id = 1;
  r->vert_cap = sdl2_MAX_VERTS;
  r->verts = arena_alloc(arena, SDL_Vertex, r->vert_cap);

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  cheese_renderer_t result = {0};
  result.userdata = r;
  result.sdf_text = false;

  result.draw_rect = sdl2_draw_rect;
  result.draw_rect_gradient = sdl2_draw_rect_gradient;
  result.draw_border = sdl2_draw_border;
  result.draw_texture = sdl2_draw_texture;
  result.draw_line = sdl2_draw_line;
  result.draw_arc = sdl2_draw_arc;
  result.draw_text = sdl2_draw_text;
  result.push_clip = sdl2_push_clip;
  result.pop_clip = sdl2_pop_clip;
  result.create_texture = sdl2_create_texture;
  result.delete_texture = sdl2_delete_texture;
  result.update_texture_region = sdl2_update_texture_region;
  result.flush_deferred = sdl2_flush_deferred;
  result.flush_draws = sdl2_flush_draws;

  return result;
}
