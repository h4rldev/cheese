/***********************************/

#include <math.h>

#include <htils/arena.h>
#include <htils/assert.h>
#include <htils/basictypes.h>
#include <htils/file.h>
#include <htils/string.h>

#include <butter/graphics.h>
#include <butter/render.h>
#include <butter/shader.h>
#include <butter/texture.h>
#include <butter/types.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/render/font.h>

#include <renderers/butter.h>

/***********************************/

typedef struct {
  f32 x, y;
  f32 u, v;
  f32 r, g, b, a;
} vertex_t;

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define SRGB_TOE 0.04045f
#define SRGB_SLOPE 12.92f
#define SRGB_OFFSET 0.055f
#define SRGB_SCALE 1.055f
#define SRGB_GAMMA 2.4f

static inline f32 srgb_to_linear(f32 c) {
  return c <= SRGB_TOE ? c / SRGB_SLOPE
                       : powf((c + SRGB_OFFSET) / SRGB_SCALE, SRGB_GAMMA);
}

static inline void unpack_color(u32 color, f32 *r, f32 *g, f32 *b, f32 *a) {
  *r = srgb_to_linear(((color >> 16) & 0xFF) / 255.0f);
  *g = srgb_to_linear(((color >> 8) & 0xFF) / 255.0f);
  *b = srgb_to_linear(((color >> 0) & 0xFF) / 255.0f);
  *a = ((color >> 24) & 0xFF) / 255.0f;
}

static void butter_renderer_submit(butter_renderer_t *renderer) {
  if (renderer->draw_cmd_count == 0)
    return;

  butter_submit_draws(renderer->butter, renderer->draw_cmds,
                      renderer->draw_cmd_count);
  renderer->draw_cmd_count = 0;
}

static void butter_renderer_queue(butter_renderer_t *renderer,
                                  butter_draw_cmd_t *cmd) {
  if (!renderer || !cmd)
    return;

  if (renderer->draw_cmd_count >= renderer->draw_cmd_cap) {
    u64 new_cap = renderer->draw_cmd_cap * 2;
    butter_draw_cmd_t *grown =
        arena_alloc(renderer->arena, butter_draw_cmd_t, new_cap);
    memcpy(grown, renderer->draw_cmds,
           renderer->draw_cmd_count * sizeof(*grown));
    renderer->draw_cmds = grown;
    renderer->draw_cmd_cap = new_cap;
  }
  renderer->draw_cmds[renderer->draw_cmd_count++] = *cmd;
}

static void screen_to_ndc(butter_t *butter, f32 x, f32 y, f32 w, f32 h, f32 *x0,
                          f32 *y0, f32 *x1, f32 *y1) {
  f32 width = (f32)butter->extent.width;
  f32 height = (f32)butter->extent.height;

  if (width == 0 || height == 0) {
    cheese_log_error("Invalid width or height");
    width = 800;
    height = 600;
  }

  *x0 = (x / width) * 2.0f - 1.0f;
  *x1 = ((x + w) / width) * 2.0f - 1.0f;
  *y0 = -((y / height) * 2.0f - 1.0f); // flip Y
  *y1 = -(((y + h) / height) * 2.0f - 1.0f);
}

static void point_to_ndc(butter_t *butter, f32 x, f32 y, f32 *ndc_x,
                         f32 *ndc_y) {
  f32 width = (f32)butter->extent.width;
  f32 height = (f32)butter->extent.height;
  if (width == 0)
    width = 800;
  if (height == 0)
    height = 600;
  *ndc_x = (x / width) * 2.0f - 1.0f;
  *ndc_y = -((y / height) * 2.0f - 1.0f);
}

static void butter_emit_arc_ndc(butter_t *butter, vertex_t *v, u32 *n, f32 cx,
                                f32 cy, f32 cr, f32 a0, f32 a1, u32 count,
                                f32 r, f32 g, f32 b, f32 a) {
  for (u32 i = 0; i < count; i++) {
    f32 t = a0 + (f32)i / (f32)(count - 1) * (a1 - a0);
    f32 px = cx + cr * cosf(t);
    f32 py = cy + cr * sinf(t);
    f32 nx, ny;
    point_to_ndc(butter, px, py, &nx, &ny);

    v[(*n)++] = (vertex_t){nx, ny, 0, 0, r, g, b, a};
  }
}

static void butter_draw_rect(void *userdata, cheese_corners_t radius, f32 x,
                             f32 y, f32 w, f32 h, cheese_color_t color) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  butter_t *butter = renderer->butter;

  f32 r, g, b, a;
  unpack_color(color, &r, &g, &b, &a);

  f32 tl = radius.top_left > 0 ? radius.top_left : 0.0f;
  f32 tr = radius.top_right > 0 ? radius.top_right : 0.0f;
  f32 br = radius.bottom_right > 0 ? radius.bottom_right : 0.0f;
  f32 bl = radius.bottom_left > 0 ? radius.bottom_left : 0.0f;

  f32 sc = 1.0f;
  if (tl + tr > w && tl + tr > 0)
    sc = min(sc, w / (tl + tr));
  if (bl + br > w && bl + br > 0)
    sc = min(sc, w / (bl + br));
  if (tl + bl > h && tl + bl > 0)
    sc = min(sc, h / (tl + bl));
  if (tr + br > h && tr + br > 0)
    sc = min(sc, h / (tr + br));

  tl *= sc;
  tr *= sc;
  bl *= sc;
  br *= sc;

  if (tl == 0 && tr == 0 && br == 0 && bl == 0) {
    butter_allocation_t alloc =
        butter_alloc_vertices(butter, 4, sizeof(vertex_t));
    if (!alloc.mapped) {
      cheese_log_error("Failed to allocate vertex buffer for draw_rect");
      return;
    }

    f32 x0, y0, x1, y1;
    screen_to_ndc(butter, x, y, w, h, &x0, &y0, &x1, &y1);
    vertex_t *v = alloc.mapped;
    v[0] = (vertex_t){x0, y0, 0, 0, r, g, b, a};
    v[1] = (vertex_t){x1, y0, 1, 0, r, g, b, a};
    v[2] = (vertex_t){x0, y1, 0, 1, r, g, b, a};
    v[3] = (vertex_t){x1, y1, 1, 1, r, g, b, a};

    butter_draw_cmd_t cmd = {0};
    cmd.pipeline = renderer->solid_pipeline;
    cmd.vertex_count = 4;
    cmd.vertex_buffer = alloc.buffer;
    cmd.vertex_offset = alloc.offset;
    butter_renderer_queue(renderer, &cmd);
    return;
  }

  const u32 SEG = 10;
  butter_allocation_t alloc =
      butter_alloc_vertices(butter, 1 + 4 * SEG + 1, sizeof(vertex_t));
  if (!alloc.mapped) {
    cheese_log_error("Failed to allocate vertex buffer for draw_rect");
    return;
  }

  vertex_t *v = alloc.mapped;

  f32 cxn, cyn;
  point_to_ndc(butter, x + w * 0.5f, y + h * 0.5f, &cxn, &cyn);
  v[0] = (vertex_t){cxn, cyn, 0, 0, r, g, b, a};

  struct {
    f32 cx, cy, cr, a0, a1;
  } corners[4] = {
      {x + tl, y + tl, tl, M_PI, 1.5f * M_PI},
      {x + w - tr, y + tr, tr, 1.5f * M_PI, 2.0f * M_PI},
      {x + w - br, y + h - br, br, 0.0f, 0.5f * M_PI},
      {x + bl, y + h - bl, bl, 0.5f * M_PI, M_PI},
  };

  u32 n = 1;
  for (u32 c = 0; c < 4; c++) {
    if (corners[c].cr <= 0.0f) {
      f32 nx, ny;
      point_to_ndc(butter, corners[c].cx, corners[c].cy, &nx, &ny);
      v[n++] = (vertex_t){nx, ny, 0, 0, r, g, b, a};
      continue;
    }

    butter_emit_arc_ndc(butter, v, &n, corners[c].cx, corners[c].cy,
                        corners[c].cr, corners[c].a0, corners[c].a1, SEG, r, g,
                        b, a);
  }

  v[n++] = v[1];

  butter_draw_cmd_t cmd = {0};
  cmd.pipeline = renderer->fan_pipeline;
  cmd.vertex_count = n;
  cmd.vertex_buffer = alloc.buffer;
  cmd.vertex_offset = alloc.offset;
  butter_renderer_queue(renderer, &cmd);
}

typedef struct {
  f32 ox, oy;
  f32 ix, iy;
} butter_border_pt_t;

static void butter_emit_border_seg(butter_renderer_t *renderer, f32 r, f32 g,
                                   f32 b, f32 a, const butter_border_pt_t *pts,
                                   u32 count) {
  if (count < 2)
    return;

  butter_allocation_t alloc =
      butter_alloc_vertices(renderer->butter, count * 2, sizeof(vertex_t));
  if (!alloc.mapped) {
    cheese_log_error("Failed to allocate vertices for border ring");
    return;
  }

  vertex_t *v = alloc.mapped;
  u32 n = 0;
  for (u32 i = 0; i < count; i++) {
    f32 nx, ny;
    point_to_ndc(renderer->butter, pts[i].ox, pts[i].oy, &nx, &ny);
    v[n++] = (vertex_t){nx, ny, 0, 0, r, g, b, a};
    point_to_ndc(renderer->butter, pts[i].ix, pts[i].iy, &nx, &ny);
    v[n++] = (vertex_t){nx, ny, 0, 0, r, g, b, a};
  }

  butter_draw_cmd_t cmd = {0};
  cmd.pipeline = renderer->solid_pipeline;
  cmd.vertex_count = n;
  cmd.vertex_buffer = alloc.buffer;
  cmd.vertex_offset = alloc.offset;
  butter_renderer_queue(renderer, &cmd);
}

static void butter_border_edge(butter_renderer_t *renderer, f32 r, f32 g, f32 b,
                               f32 a, f32 ox0, f32 oy0, f32 ox1, f32 oy1,
                               f32 ix0, f32 iy0, f32 ix1, f32 iy1) {
  butter_border_pt_t pts[2] = {
      {ox0, oy0, ix0, iy0},
      {ox1, oy1, ix1, iy1},
  };
  butter_emit_border_seg(renderer, r, g, b, a, pts, 2);
}

static void butter_border_arc(butter_renderer_t *renderer, f32 r, f32 g, f32 b,
                              f32 a, f32 ocx, f32 ocy, f32 ro, f32 icx, f32 icy,
                              f32 ri, f32 a0, f32 a1) {
  if (ro <= 0.0f)
    return;

  const u32 SEG = 8;
  butter_border_pt_t pts[SEG + 1];
  for (u32 i = 0; i <= SEG; i++) {
    f32 t = a0 + (f32)i / (f32)SEG * (a1 - a0);
    pts[i] = (butter_border_pt_t){ocx + ro * cosf(t), ocy + ro * sinf(t),
                                  icx + ri * cosf(t), icy + ri * sinf(t)};
  }
  butter_emit_border_seg(renderer, r, g, b, a, pts, SEG + 1);
}

static void butter_draw_border(void *userdata, cheese_corners_t radius, f32 x,
                               f32 y, f32 w, f32 h, f32 thickness, u32 sides,
                               cheese_color_t color) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  if (!renderer || thickness <= 0.0f || w <= 0.0f || h <= 0.0f)
    return;

  if (sides == 0)
    sides = CHEESE_SIDE_ALL;

  f32 r, g, b, a;
  unpack_color(color, &r, &g, &b, &a);

  f32 tl = radius.top_left > 0 ? radius.top_left : 0.0f;
  f32 tr = radius.top_right > 0 ? radius.top_right : 0.0f;
  f32 br = radius.bottom_right > 0 ? radius.bottom_right : 0.0f;
  f32 bl = radius.bottom_left > 0 ? radius.bottom_left : 0.0f;

  f32 sc = 1.0f;
  if (tl + tr > w && tl + tr > 0)
    sc = min(sc, w / (tl + tr));
  if (bl + br > w && bl + br > 0)
    sc = min(sc, w / (bl + br));
  if (tl + bl > h && tl + bl > 0)
    sc = min(sc, h / (tl + bl));
  if (tr + br > h && tr + br > 0)
    sc = min(sc, h / (tr + br));
  tl *= sc;
  tr *= sc;
  bl *= sc;
  br *= sc;

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
    butter_border_edge(renderer, r, g, b, a, x + tl, y, x + w - tr, y, ix + itl,
                       iy, ix + iw - itr, iy);
  if (sides & (CHEESE_SIDE_TOP | CHEESE_SIDE_RIGHT))
    butter_border_arc(renderer, r, g, b, a, x + w - tr, y + tr, tr,
                      ix + iw - itr, iy + itr, itr, -0.5f * M_PI, 0.0f);
  if (sides & CHEESE_SIDE_RIGHT)
    butter_border_edge(renderer, r, g, b, a, x + w, y + tr, x + w, y + h - br,
                       ix + iw, iy + itr, ix + iw, iy + ih - ibr);
  if (sides & (CHEESE_SIDE_BOTTOM | CHEESE_SIDE_RIGHT))
    butter_border_arc(renderer, r, g, b, a, x + w - br, y + h - br, br,
                      ix + iw - ibr, iy + ih - ibr, ibr, 0.0f, 0.5f * M_PI);
  if (sides & CHEESE_SIDE_BOTTOM)
    butter_border_edge(renderer, r, g, b, a, x + w - br, y + h, x + bl, y + h,
                       ix + iw - ibr, iy + ih, ix + ibl, iy + ih);
  if (sides & (CHEESE_SIDE_BOTTOM | CHEESE_SIDE_LEFT))
    butter_border_arc(renderer, r, g, b, a, x + bl, y + h - bl, bl, ix + ibl,
                      iy + ih - ibl, ibl, 0.5f * M_PI, M_PI);
  if (sides & CHEESE_SIDE_LEFT)
    butter_border_edge(renderer, r, g, b, a, x, y + h - bl, x, y + tl, ix,
                       iy + ih - ibl, ix, iy + itl);
  if (sides & (CHEESE_SIDE_LEFT | CHEESE_SIDE_TOP))
    butter_border_arc(renderer, r, g, b, a, x + tl, y + tl, tl, ix + itl,
                      iy + itl, itl, M_PI, 1.5f * M_PI);
}

static void butter_draw_quad_textured(void *userdata, f32 x, f32 y, f32 w,
                                      f32 h, f32 u0, f32 v0, f32 u1, f32 v1,
                                      u32 texture_id, cheese_color_t color,
                                      butter_pipeline_t *pipeline) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  butter_t *butter = renderer->butter;

  f32 r, g, b, a;
  unpack_color(color, &r, &g, &b, &a);

  butter_allocation_t alloc =
      butter_alloc_vertices(butter, 4, sizeof(vertex_t));
  if (!alloc.mapped) {
    cheese_log_error("Failed to allocate vertex buffer for draw_quad_textured");
    return;
  }

  f32 x0, y0, x1, y1;
  screen_to_ndc(butter, x, y, w, h, &x0, &y0, &x1, &y1);

  vertex_t *vertices = alloc.mapped;
  vertices[0] = (vertex_t){x0, y0, u0, v0, r, g, b, a};
  vertices[1] = (vertex_t){x1, y0, u1, v0, r, g, b, a};
  vertices[2] = (vertex_t){x0, y1, u0, v1, r, g, b, a};
  vertices[3] = (vertex_t){x1, y1, u1, v1, r, g, b, a};

  butter_draw_cmd_t cmd = {0};
  cmd.pipeline = pipeline ? pipeline : renderer->textured_pipeline;
  cmd.vertex_buffer = alloc.buffer;
  cmd.vertex_offset = alloc.offset;
  cmd.vertex_count = 4;
  cmd.texture_id = texture_id;

  if (renderer->clip_stack_depth > 0) {
    cmd.scissor = renderer->clip_stack[renderer->clip_stack_depth - 1];
    cmd.scissor_enabled = true;
  }

  butter_renderer_queue(renderer, &cmd);
}

static void butter_draw_texture(void *userdata, f32 x, f32 y, f32 w, f32 h,
                                u32 texture_id, cheese_color_t color) {
  // cheese_log_debug("Drawing texture with id %d", texture_id);
  butter_draw_quad_textured(userdata, x, y, w, h, 0, 0, 1, 1, texture_id, color,
                            null);
}

static void butter_draw_line(void *userdata, f32 x1, f32 y1, f32 x2, f32 y2,
                             f32 thickness, cheese_color_t color) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  butter_t *butter = renderer->butter;

  f32 r, g, b, a;
  unpack_color(color, &r, &g, &b, &a);

  if (thickness > 0.0f && thickness <= 1.0f) {
    butter_allocation_t alloc =
        butter_alloc_vertices(butter, 2, sizeof(vertex_t));

    if (!alloc.mapped) {
      cheese_log_error("Failed to allocate vertex buffer for >0.0f <1.0f "
                       "thickness draw_line");
      return;
    }

    f32 x1n = 0, y1n = 0, x2n = 0, y2n = 0;
    point_to_ndc(butter, x1, y1, &x1n, &y1n);
    point_to_ndc(butter, x2, y2, &x2n, &y2n);

    vertex_t *vertices = alloc.mapped;
    vertices[0] = (vertex_t){x1n, y1n, 0, 0, r, g, b, a};
    vertices[1] = (vertex_t){x2n, y2n, 0, 0, r, g, b, a};

    butter_draw_cmd_t cmd = {0};
    cmd.pipeline = renderer->line_pipeline;
    cmd.vertex_count = 2;
    cmd.vertex_buffer = alloc.buffer;
    cmd.vertex_offset = alloc.offset;

    if (renderer->clip_stack_depth > 0) {
      cmd.scissor = renderer->clip_stack[renderer->clip_stack_depth - 1];
      cmd.scissor_enabled = true;
    } else
      cmd.scissor_enabled = false;

    butter_renderer_queue(renderer, &cmd);
    return;
  }

  f32 dx = x2 - x1;
  f32 dy = y2 - y1;
  f32 len = sqrtf(dx * dx + dy * dy);
  if (len < 0.0001f) {
    cheese_log_error("Invalid line parameters");
    return;
  }

  f32 half_thickness = thickness / 2;
  if (half_thickness < 0.5f)
    half_thickness = 0.5f;

  f32 nx = -dy / len;
  f32 ny = dx / len;

  f32 px[4], py[4];
  px[0] = x1 + nx * half_thickness;
  py[0] = y1 + ny * half_thickness;
  px[1] = x1 - nx * half_thickness;
  py[1] = y1 - ny * half_thickness;
  px[2] = x2 - nx * half_thickness;
  py[2] = y2 - ny * half_thickness;
  px[3] = x2 + nx * half_thickness;
  py[3] = y2 + ny * half_thickness;

  f32 nx0, ny0, nx1, ny1, nx2, ny2, nx3, ny3;
  point_to_ndc(butter, px[0], py[0], &nx0, &ny0);
  point_to_ndc(butter, px[1], py[1], &nx1, &ny1);
  point_to_ndc(butter, px[2], py[2], &nx2, &ny2);
  point_to_ndc(butter, px[3], py[3], &nx3, &ny3);

  butter_allocation_t alloc =
      butter_alloc_vertices(butter, 4, sizeof(vertex_t));
  if (!alloc.mapped) {
    cheese_log_error(
        "Failed to allocate vertex buffer for >1.0f thickness draw_line");
    return;
  }

  vertex_t *vertices = alloc.mapped;
  vertices[0] = (vertex_t){nx0, ny0, 0, 0, r, g, b, a};
  vertices[1] = (vertex_t){nx1, ny1, 0, 0, r, g, b, a};
  vertices[2] = (vertex_t){nx2, ny2, 0, 0, r, g, b, a};
  vertices[3] = (vertex_t){nx3, ny3, 0, 0, r, g, b, a};

  butter_draw_cmd_t cmd = {0};
  cmd.pipeline = renderer->solid_pipeline;
  cmd.vertex_buffer = alloc.buffer;
  cmd.vertex_offset = alloc.offset;
  cmd.vertex_count = 4;

  if (renderer->clip_stack_depth > 0) {
    cmd.scissor = renderer->clip_stack[renderer->clip_stack_depth - 1];
    cmd.scissor_enabled = true;
  } else
    cmd.scissor_enabled = false;

  butter_renderer_queue(renderer, &cmd);
}

static void butter_draw_arc(void *userdata, f32 cx, f32 cy, f32 radius,
                            f32 start_angle, f32 end_angle, f32 thickness,
                            cheese_color_t color) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  butter_t *butter = renderer->butter;

  vk_rect2d_t current_clip = {0};

  f32 r, g, b, a;
  unpack_color(color, &r, &g, &b, &a);

  start_angle = fmodf(start_angle, 2.0f * M_PI);
  end_angle = fmodf(end_angle, 2.0f * M_PI);
  while (start_angle >= end_angle)
    end_angle += 2.0f * M_PI;

  f32 angle_range = end_angle - start_angle;
  if (angle_range <= 0.0f || radius <= 0.0f) {
    cheese_log_error("Invalid arc parameters");
    return;
  }

  u32 segments = (u32)(angle_range / (2.0f * M_PI) * 36.0f);
  if (segments < 3)
    segments = 3;

  b32 clip_active = (renderer->clip_stack_depth > 0);
  if (clip_active)
    current_clip = renderer->clip_stack[renderer->clip_stack_depth - 1];

  if (thickness > 0.0f && thickness <= 1.0f) {
    u32 vcount = segments + 1;
    butter_allocation_t alloc =
        butter_alloc_vertices(butter, vcount, sizeof(vertex_t));
    if (!alloc.mapped) {
      cheese_log_error("Failed to allocate vertex buffer for >0.0f <1.0f "
                       "thickness draw_arc");
      return;
    }

    vertex_t *vertices = alloc.mapped;
    for (u32 i = 0; i < vcount; i++) {
      f32 t = start_angle + (f32)i / (f32)segments * angle_range;
      f32 px = cx + radius * cosf(t);
      f32 py = cy + radius * sinf(t);

      f32 nx, ny;
      point_to_ndc(butter, px, py, &nx, &ny);

      vertices[i] = (vertex_t){nx, ny, 0, 0, r, g, b, a};
    }

    butter_draw_cmd_t cmd = {0};
    cmd.pipeline = renderer->line_pipeline;
    cmd.vertex_count = vcount;
    cmd.vertex_buffer = alloc.buffer;
    cmd.vertex_offset = alloc.offset;
    cmd.scissor_enabled = clip_active;
    if (clip_active)
      cmd.scissor = current_clip;

    butter_renderer_queue(renderer, &cmd);
    return;
  }

  if (thickness <= 0.0f) {
    u32 vcount = segments + 2;
    butter_allocation_t v_alloc =
        butter_alloc_vertices(butter, vcount, sizeof(vertex_t));
    if (!v_alloc.mapped) {
      cheese_log_error("Failed to allocate vertex buffer for filled draw_arc");
      return;
    }

    vertex_t *vertices = v_alloc.mapped;

    f32 nx0, ny0;
    point_to_ndc(butter, cx, cy, &nx0, &ny0);
    vertices[0] = (vertex_t){nx0, ny0, 0, 0, r, g, b, a};
    for (u32 i = 0; i <= segments; i++) {
      f32 t = start_angle + (f32)i / (f32)segments * angle_range;
      f32 px = cx + radius * cosf(t);
      f32 py = cy + radius * sinf(t);

      f32 nx, ny;
      point_to_ndc(butter, px, py, &nx, &ny);

      vertices[i + 1] = (vertex_t){nx, ny, 0, 0, r, g, b, a};
    }

    butter_draw_cmd_t cmd = {0};
    cmd.pipeline = renderer->fan_pipeline;
    cmd.vertex_buffer = v_alloc.buffer;
    cmd.vertex_offset = v_alloc.offset;
    cmd.vertex_count = vcount;
    cmd.scissor_enabled = clip_active;
    if (clip_active)
      cmd.scissor = current_clip;

    butter_renderer_queue(renderer, &cmd);
    return;
  }

  f32 inner_radius = radius - thickness;
  if (inner_radius < 0.0f)
    inner_radius = 0.0f;

  u32 vcount = (segments + 1) * 2;
  butter_allocation_t v_alloc =
      butter_alloc_vertices(butter, vcount, sizeof(vertex_t));
  if (!v_alloc.mapped) {
    cheese_log_error(
        "Failed to allocate vertex buffer for >1.0f thickness draw_arc");
    return;
  }

  vertex_t *vertices = v_alloc.mapped;
  for (u32 i = 0; i <= segments; i++) {
    f32 t = start_angle + (f32)i / (f32)segments * angle_range;
    f32 cos_t = cosf(t);
    f32 sin_t = sinf(t);

    f32 nx0, ny0, nx1, ny1;
    f32 px0 = cx + radius * cos_t;
    f32 py0 = cy + radius * sin_t;
    f32 px1 = cx + inner_radius * cos_t;
    f32 py1 = cy + inner_radius * sin_t;

    point_to_ndc(butter, px0, py0, &nx0, &ny0);
    point_to_ndc(butter, px1, py1, &nx1, &ny1);

    vertices[i * 2] = (vertex_t){nx0, ny0, 0, 0, r, g, b, a};
    vertices[i * 2 + 1] = (vertex_t){nx1, ny1, 0, 0, r, g, b, a};
  }

  butter_draw_cmd_t cmd = {0};
  cmd.pipeline = renderer->solid_pipeline;
  cmd.vertex_buffer = v_alloc.buffer;
  cmd.vertex_offset = v_alloc.offset;
  cmd.vertex_count = vcount;
  cmd.scissor_enabled = clip_active;
  if (clip_active)
    cmd.scissor = current_clip;

  butter_renderer_queue(renderer, &cmd);
}

static void butter_draw_text(void *userdata, f32 x, f32 y, const string *text,
                             cheese_font_t *font, cheese_color_t color,
                             f32 scale) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  if (!font || !font->active_variant || !text || text->len == 0) {
    cheese_log_error("Invalid text or font");
    return;
  }

  if (!font->active_variant->atlas_texture_id ||
      font->active_variant->atlas_dirty) {
    cheese_log_info("Font atlas not ready or dirty, rebuilding next frame");
    return;
  }

  hb_buffer_t *hb_buffer = hb_buffer_create();
  hb_buffer_add_utf8(hb_buffer, (cstr *)text->base, text->len, 0, -1);
  hb_buffer_set_direction(hb_buffer, HB_DIRECTION_LTR);
  hb_buffer_set_script(hb_buffer, HB_SCRIPT_COMMON);
  hb_buffer_set_language(hb_buffer, hb_language_from_string("en", -1));
  hb_shape(font->active_variant->hb_font, hb_buffer, NULL, 0);

  u32 glyph_count;
  hb_glyph_info_t *glyph_info =
      hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
  hb_glyph_position_t *glyph_pos =
      hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

  f32 cursor_x = x;
  f32 cursor_y = y;

  f32 s = scale * (font->scale > 0.0f ? font->scale : 1.0f);
  butter_pipeline_t *pipe =
      font->active_variant->sdf ? renderer->sdf_pipeline : null;

  for (u32 i = 0; i < glyph_count; i++) {
    hb_codepoint_t glyph_id = glyph_info[i].codepoint;

    f32 x_advance = (f32)glyph_pos[i].x_advance / 64.0f;
    f32 y_advance = (f32)glyph_pos[i].y_advance / 64.0f;
    f32 x_offset = (f32)glyph_pos[i].x_offset / 64.0f;
    f32 y_offset = (f32)glyph_pos[i].y_offset / 64.0f;

    cheese_glyph_t *glyph = cheese_font_get_glyph(font, glyph_id);
    if (!glyph || glyph->width == 0 || glyph->height == 0) {
      cursor_x += x_advance * s;
      cursor_y += y_advance * s;
      continue;
    }

    f32 w = glyph->width * s;
    f32 h = glyph->height * s;
    f32 off_x = (glyph->bearing_x + x_offset) * s;
    f32 off_y = (glyph->bearing_y + y_offset) * s;

    butter_draw_quad_textured(renderer, cursor_x + off_x, cursor_y - off_y, w,
                              h, glyph->u0, glyph->v0, glyph->u1, glyph->v1,
                              font->active_variant->atlas_texture_id, color,
                              pipe);

    cursor_x += x_advance * s;
    cursor_y += y_advance * s;
  }

  hb_buffer_destroy(hb_buffer);
}

static void butter_push_clip(void *userdata, f32 x, f32 y, f32 w, f32 h) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  if (renderer->clip_stack_depth >= 32) {
    cheese_log_error("Maximum clip stack depth reached");
    return;
  }

  vk_rect2d_t rect = {0};
  rect.offset.x = (i32)x;
  rect.offset.y = (i32)y;
  rect.extent.width = (u32)w;
  rect.extent.height = (u32)h;

  if (renderer->clip_stack_depth > 0) {
    vk_rect2d_t parent = renderer->clip_stack[renderer->clip_stack_depth - 1];
    i32 x1 = max(rect.offset.x, parent.offset.x);
    i32 y1 = max(rect.offset.y, parent.offset.y);
    i32 x2 = min(rect.offset.x + rect.extent.width,
                 parent.offset.x + parent.extent.width);
    i32 y2 = min(rect.offset.y + rect.extent.height,
                 parent.offset.y + parent.extent.height);

    if (x1 >= x2 || y1 >= y2) {
      rect.offset.x = 0;
      rect.offset.y = 0;
      rect.extent.width = 0;
      rect.extent.height = 0;
    } else {
      rect.offset.x = x1;
      rect.offset.y = y1;
      rect.extent.width = (u32)(x2 - x1);
      rect.extent.height = (u32)(y2 - y1);
    }
  }

  renderer->clip_stack[renderer->clip_stack_depth++] = rect;
}

static void butter_pop_clip(void *userdata) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  if (renderer->clip_stack_depth > 0)
    renderer->clip_stack_depth--;
}

static i32 butter__create_texture(void *userdata, u32 width, u32 height,
                                  cheese_texture_format_t format,
                                  const u8 *data) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  butter_t *butter = renderer->butter;

  vk_format_t vk_format = VK_FORMAT_R8G8B8A8_SRGB;
  u64 bytes_per_pixel = 4;
  if (format == CHEESE_TEXTURE_R8) {
    vk_format = VK_FORMAT_R8_UNORM;
    bytes_per_pixel = 1;
  }

  butter_texture_t *tex = butter_submit_texture_upload(
      butter, width, height, vk_format, data,
      (u64)width * height * bytes_per_pixel, renderer->default_sampler);

  while (!butter_texture_is_ready(tex))
    ;

  if (tex->image == VK_NULL_HANDLE) {
    cheese_log_error("Failed to create texture");
    return 0;
  }

  i32 id = butter_texture_register(butter, tex);
  cheese_log_debug("Registered texture: %d", id);
  return id;
}

static void butter__destroy_texture(void *userdata, u32 texture_id) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  if (renderer->deferred_count < 64)
    renderer->deferred_ids[renderer->deferred_count++] = texture_id;
}

static void butter__update_texture_region(void *userdata, u32 texture_id, i32 x,
                                          i32 y, u32 w, u32 h, const void *data,
                                          u64 data_size) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  butter_texture_t *tex = butter_texture_get(renderer->butter, texture_id);

  if (!tex) {
    cheese_log_error("butter__update_texture_region: Unknown texture id %u",
                     texture_id);
    return;
  }

  butter_update_texture_region(renderer->butter, tex, x, y, w, h, data,
                               data_size);
}

static void butter_renderer_destroy_pipelines(butter_renderer_t *renderer) {
  butter_t *butter = renderer->butter;

  if (butter_pipeline_valid(renderer->solid_pipeline))
    butter_destroy_pipeline(butter, renderer->solid_pipeline);
  if (butter_pipeline_valid(renderer->textured_pipeline))
    butter_destroy_pipeline(butter, renderer->textured_pipeline);
  if (butter_pipeline_valid(renderer->sdf_pipeline))
    butter_destroy_pipeline(butter, renderer->sdf_pipeline);
  if (butter_pipeline_valid(renderer->line_pipeline))
    butter_destroy_pipeline(butter, renderer->line_pipeline);
  if (butter_pipeline_valid(renderer->fan_pipeline))
    butter_destroy_pipeline(butter, renderer->fan_pipeline);

  renderer->solid_pipeline = null;
  renderer->textured_pipeline = null;
  renderer->sdf_pipeline = null;
  renderer->line_pipeline = null;
  renderer->fan_pipeline = null;
}

static b32 butter_renderer_build_pipelines(butter_renderer_t *renderer) {
  butter_t *butter = renderer->butter;

  butter_attribute_t attrs[3] = {
      {.location = 0,
       .type = BUTTER_ATTRIB_POSITION_2D,
       .offset = offsetof(vertex_t, x)},
      {.location = 1,
       .type = BUTTER_ATTRIB_UV,
       .offset = offsetof(vertex_t, u)},
      {.location = 2,
       .type = BUTTER_ATTRIB_COLOR,
       .offset = offsetof(vertex_t, r)},
  };

  butter_shader_t solid_shaders[2] = {
      *butter_shader_get(butter, "solid_vert"),
      *butter_shader_get(butter, "solid_frag"),
  };
  butter_shader_t textured_shaders[2] = {
      *butter_shader_get(butter, "solid_vert"),
      *butter_shader_get(butter, "textured_frag"),
  };
  butter_shader_t fan_shaders[2] = {
      *butter_shader_get(butter, "solid_vert"),
      *butter_shader_get(butter, "solid_frag"),
  };
  butter_shader_t sdf_shaders[2] = {
      *butter_shader_get(butter, "solid_vert"),
      *butter_shader_get(butter, "sdf_frag"),
  };

  cheese_log_debug("Creating solid pipeline");
  butter_pipeline_desc_t solid_desc = butter_pipeline_desc_default();
  butter_pipeline_desc_add_attributes(&solid_desc, attrs, 3);
  butter_pipeline_desc_add_shaders(&solid_desc, solid_shaders, 2);
  butter_pipeline_desc_set_vertex_stride(&solid_desc, sizeof(vertex_t));
  solid_desc.cull_mode = BUTTER_CULL_NONE;
  solid_desc.topology = BUTTER_TOPOLOGY_TRIANGLE_STRIP;
  solid_desc.blend_mode = BUTTER_BLEND_ALPHA;
  renderer->solid_pipeline = butter_create_pipeline(butter, &solid_desc);
  if (!butter_pipeline_valid(renderer->solid_pipeline)) {
    cheese_log_fatal("Failed to create solid pipeline");
    butter_renderer_destroy_pipelines(renderer);
    return false;
  }

  cheese_log_debug("Creating textured pipeline");
  butter_pipeline_desc_t textured_desc = butter_pipeline_desc_default();
  butter_pipeline_desc_add_attributes(&textured_desc, attrs, 3);
  butter_pipeline_desc_add_shaders(&textured_desc, textured_shaders, 2);
  butter_pipeline_desc_set_vertex_stride(&textured_desc, sizeof(vertex_t));
  textured_desc.cull_mode = BUTTER_CULL_NONE;
  textured_desc.topology = BUTTER_TOPOLOGY_TRIANGLE_STRIP;
  textured_desc.blend_mode = BUTTER_BLEND_ALPHA;
  textured_desc.descriptor_set_layouts = &butter->texture_descriptor_set_layout;
  textured_desc.descriptor_set_layout_count = 1;
  renderer->textured_pipeline = butter_create_pipeline(butter, &textured_desc);
  if (!butter_pipeline_valid(renderer->textured_pipeline)) {
    cheese_log_fatal("Failed to create textured pipeline");
    butter_renderer_destroy_pipelines(renderer);
    return false;
  }

  cheese_log_debug("Creating sdf pipeline");
  butter_pipeline_desc_t sdf_desc = butter_pipeline_desc_default();
  butter_pipeline_desc_add_attributes(&sdf_desc, attrs, 3);
  butter_pipeline_desc_add_shaders(&sdf_desc, sdf_shaders, 2);
  butter_pipeline_desc_set_vertex_stride(&sdf_desc, sizeof(vertex_t));
  sdf_desc.cull_mode = BUTTER_CULL_NONE;
  sdf_desc.topology = BUTTER_TOPOLOGY_TRIANGLE_STRIP;
  sdf_desc.blend_mode = BUTTER_BLEND_ALPHA;
  sdf_desc.descriptor_set_layouts = &butter->texture_descriptor_set_layout;
  sdf_desc.descriptor_set_layout_count = 1;
  renderer->sdf_pipeline = butter_create_pipeline(butter, &sdf_desc);
  if (!butter_pipeline_valid(renderer->sdf_pipeline)) {
    cheese_log_fatal("Failed to create sdf pipeline");
    butter_renderer_destroy_pipelines(renderer);
    return false;
  }

  cheese_log_debug("Creating line pipeline");
  butter_pipeline_desc_t line_desc = butter_pipeline_desc_default();
  butter_pipeline_desc_add_attributes(&line_desc, attrs, 3);
  butter_pipeline_desc_add_shaders(&line_desc, solid_shaders, 2);
  butter_pipeline_desc_set_vertex_stride(&line_desc, sizeof(vertex_t));
  line_desc.cull_mode = BUTTER_CULL_NONE;
  line_desc.topology = BUTTER_TOPOLOGY_LINE_STRIP;
  renderer->line_pipeline = butter_create_pipeline(butter, &line_desc);
  if (!butter_pipeline_valid(renderer->line_pipeline)) {
    cheese_log_fatal("Failed to create line pipeline");
    butter_renderer_destroy_pipelines(renderer);
    return false;
  }

  cheese_log_debug("Creating fan pipeline");
  butter_pipeline_desc_t fan_desc = butter_pipeline_desc_default();
  butter_pipeline_desc_add_attributes(&fan_desc, attrs, 3);
  butter_pipeline_desc_add_shaders(&fan_desc, fan_shaders, 2);
  butter_pipeline_desc_set_vertex_stride(&fan_desc, sizeof(vertex_t));
  fan_desc.cull_mode = BUTTER_CULL_NONE;
  fan_desc.topology = BUTTER_TOPOLOGY_TRIANGLE_FAN;
  fan_desc.blend_mode = BUTTER_BLEND_ALPHA;
  renderer->fan_pipeline = butter_create_pipeline(butter, &fan_desc);
  if (!butter_pipeline_valid(renderer->fan_pipeline)) {
    cheese_log_fatal("Failed to create fan pipeline");
    butter_renderer_destroy_pipelines(renderer);
    return false;
  }

  renderer->aa_samples = butter_get_aa_samples(butter);
  return true;
}

static void butter_flush_deferred(void *userdata) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  for (u32 i = 0; i < renderer->deferred_count; i++)
    butter_texture_deregister(renderer->butter, renderer->deferred_ids[i]);
  renderer->deferred_count = 0;

  u32 samples = butter_get_aa_samples(renderer->butter);
  if (samples != renderer->aa_samples) {
    cheese_log_debug("AA samples changed (%u -> %u), rebuilding pipelines",
                     renderer->aa_samples, samples);
    butter_renderer_destroy_pipelines(renderer);
    if (!butter_renderer_build_pipelines(renderer))
      cheese_log_fatal("Failed to rebuild pipelines after AA change");
  }
}

static void butter_flush_draws(void *userdata) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  if (!renderer)
    return;

  butter_renderer_submit(renderer);
}

cheese_renderer_t cheese_create_butter_renderer(butter_t *butter,
                                                arena_t *arena) {
  butter_renderer_t *renderer = arena_alloc_zeroed(arena, butter_renderer_t, 1);
  renderer->butter = butter;
  renderer->clip_stack_depth = 0;
  renderer->arena = arena;
  renderer->draw_cmd_cap = 64;
  renderer->draw_cmds = arena_alloc_zeroed(renderer->arena, butter_draw_cmd_t,
                                           renderer->draw_cmd_cap);

  butter_shader_t *solid_vert_shader = butter_shader_from_memory(
      butter, arena, "solid_vert", BUTTER_STAGE_VERTEX, solid_vert_spv,
      sizeof(solid_vert_spv), "main");

  butter_shader_t *solid_frag_shader = butter_shader_from_memory(
      butter, arena, "solid_frag", BUTTER_STAGE_FRAGMENT, solid_frag_spv,
      sizeof(solid_frag_spv), "main");

  butter_shader_t *textured_frag_shader = butter_shader_from_memory(
      butter, arena, "textured_frag", BUTTER_STAGE_FRAGMENT, textured_frag_spv,
      sizeof(textured_frag_spv), "main");

  butter_shader_t *sdf_frag_shader = butter_shader_from_memory(
      butter, arena, "sdf_frag", BUTTER_STAGE_FRAGMENT, sdf_frag_spv,
      sizeof(sdf_frag_spv), "main");

  if (!solid_vert_shader || !solid_frag_shader || !textured_frag_shader ||
      !sdf_frag_shader) {
    cheese_log_error("Failed to load shaders");
    return (cheese_renderer_t){0};
  }

  if (!butter_renderer_build_pipelines(renderer))
    return (cheese_renderer_t){0};

  cheese_log_debug("Creating default sampler");
  butter_sampler_desc_t sampler_desc = butter_sampler_desc_nearest_clamp();
  renderer->default_sampler = butter_create_sampler(butter, &sampler_desc);
  if (renderer->default_sampler == VK_NULL_HANDLE) {
    cheese_log_fatal("Failed to create default sampler");
    butter_renderer_destroy_pipelines(renderer);
  }

  cheese_log_debug("Creating renderer");
  cheese_renderer_t cheese_renderer = {0};
  cheese_renderer.userdata = renderer;
  cheese_renderer.sdf_text = true;
  cheese_renderer.create_texture = butter__create_texture;
  cheese_renderer.delete_texture = butter__destroy_texture;
  cheese_renderer.update_texture_region = butter__update_texture_region;

  cheese_renderer.draw_rect = butter_draw_rect;
  cheese_renderer.draw_border = butter_draw_border;
  cheese_renderer.draw_texture = butter_draw_texture;
  cheese_renderer.draw_line = butter_draw_line;
  cheese_renderer.draw_arc = butter_draw_arc;
  cheese_renderer.draw_text = butter_draw_text;
  cheese_renderer.push_clip = butter_push_clip;
  cheese_renderer.pop_clip = butter_pop_clip;

  cheese_renderer.flush_deferred = butter_flush_deferred;
  cheese_renderer.flush_draws = butter_flush_draws;

  return cheese_renderer;
}

void cheese_destroy_butter_renderer(cheese_renderer_t *renderer) {
  if (!renderer || !renderer->userdata)
    return;

  butter_renderer_t *butter_renderer = (butter_renderer_t *)renderer->userdata;
  renderer->flush_deferred(butter_renderer);

  butter_renderer_destroy_pipelines(butter_renderer);

  if (butter_renderer->default_sampler)
    butter_destroy_sampler(butter_renderer->butter,
                           butter_renderer->default_sampler);

  renderer->userdata = null;
}
