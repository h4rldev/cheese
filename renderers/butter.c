#include <math.h>

#include <htils/file.h>

#include <butter/graphics.h>
#include <butter/texture.h>
#include <butter/types.h>

#include <cheese/font.h>
#include <cheese/log.h>

#include <renderers/butter.h>

typedef struct {
  f32 x, y;
  f32 u, v;
  f32 r, g, b, a;
} vertex_t;

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

static inline void unpack_color(u32 color, f32 *r, f32 *g, f32 *b, f32 *a) {
  *r = ((color >> 16) & 0xFF) / 255.0f;
  *g = ((color >> 8) & 0xFF) / 255.0f;
  *b = ((color >> 0) & 0xFF) / 255.0f;
  *a = ((color >> 24) & 0xFF) / 255.0f;
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

static void butter_draw_rect(void *userdata, f32 x, f32 y, f32 w, f32 h,
                             cheese_color_t color) {
  /*cheese_log_debug("butter_draw_rect: x=%.2f, y=%.2f, w=%.2f, h=%.2f", x, y,
     w, h);*/

  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  butter_t *butter = renderer->butter;

  f32 r, g, b, a;
  unpack_color(color, &r, &g, &b, &a);

  butter_allocation_t alloc =
      butter_alloc_vertices(butter, 4, sizeof(vertex_t));
  if (!alloc.mapped) {
    cheese_log_error("Failed to allocate vertex buffer for draw_rect");
    return;
  }

  f32 x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  screen_to_ndc(butter, x, y, w, h, &x0, &y0, &x1, &y1);
  /*cheese_log_debug("butter_draw_rect: NDC: (x0: %.2f, y0: %.2f) (x1: "
                   "%.2f, y0: %.2f) (x0: %.2f, y1: %.2f) (x1: %.2f, y1: %.2f)",
                   x0, y0, x1, y0, x0, y1, x1, y1);*/

  vertex_t *vertices = alloc.mapped;
  vertices[0] = (vertex_t){x0, y0, 0, 0, r, g, b, a};
  vertices[1] = (vertex_t){x1, y0, 1, 0, r, g, b, a};
  vertices[2] = (vertex_t){x0, y1, 0, 1, r, g, b, a};
  vertices[3] = (vertex_t){x1, y1, 1, 1, r, g, b, a};

  butter_draw_cmd_t cmd = {0};
  cmd.pipeline = renderer->solid_pipeline;
  cmd.vertex_count = 4;
  cmd.vertex_buffer = alloc.buffer;
  cmd.vertex_offset = alloc.offset;

  butter_submit_draws(butter, &cmd, 1);
}

static void butter_draw_quad_textured(void *userdata, f32 x, f32 y, f32 w,
                                      f32 h, f32 u0, f32 v0, f32 u1, f32 v1,
                                      u32 texture_id, cheese_color_t color) {
  /*cheese_log_debug("butter_draw_quad_textured: x=%.2f, y=%.2f, w=%.2f,
     h=%.2f", x, y, w, h);*/

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
  /*cheese_log_debug("butter_draw_quad_textured: NDC: (x0: %.2f, y0: %.2f) (x1:
     "
                   "%.2f, y0: %.2f) (x0: %.2f, y1: %.2f) (x1: %.2f, y1: %.2f)",
                   x0, y0, x1, y0, x0, y1, x1, y1);*/

  vertex_t *vertices = alloc.mapped;
  vertices[0] = (vertex_t){x0, y0, u0, v0, r, g, b, a};
  vertices[1] = (vertex_t){x1, y0, u1, v0, r, g, b, a};
  vertices[2] = (vertex_t){x0, y1, u0, v1, r, g, b, a};
  vertices[3] = (vertex_t){x1, y1, u1, v1, r, g, b, a};

  butter_draw_cmd_t cmd = {0};
  cmd.pipeline = renderer->textured_pipeline;
  cmd.vertex_buffer = alloc.buffer;
  cmd.vertex_offset = alloc.offset;
  cmd.vertex_count = 4;
  cmd.texture_id = texture_id;

  if (renderer->clip_stack_depth > 0) {
    cmd.scissor = renderer->clip_stack[renderer->clip_stack_depth - 1];
    cmd.scissor_enabled = true;
  }

  butter_submit_draws(butter, &cmd, 1);
}

static void butter_draw_texture(void *userdata, f32 x, f32 y, f32 w, f32 h,
                                u32 texture_id, cheese_color_t color) {
  // cheese_log_debug("Drawing texture with id %d", texture_id);
  butter_draw_quad_textured(userdata, x, y, w, h, 0, 0, 1, 1, texture_id,
                            color);
}

static void butter_draw_line(void *userdata, f32 x1, f32 y1, f32 x2, f32 y2,
                             f32 thickness, cheese_color_t color) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  butter_t *butter = renderer->butter;

  f32 r, g, b, a;
  unpack_color(color, &r, &g, &b, &a);

  if (thickness > 0.0f && thickness <= 1.0f &&
      renderer->line_pipeline.pipeline != VK_NULL_HANDLE) {
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
    } else {
      cmd.scissor_enabled = false;
    }

    butter_submit_draws(butter, &cmd, 1);
    return;
  }

  f32 dx = x2 - x1;
  f32 dy = y2 - y1;
  f32 len = sqrtf(dx * dx + dy * dy);
  if (len < 0.0001f) {
    cheese_log_error("Invalid arc parameters");
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

  butter_allocation_t alloc =
      butter_alloc_vertices(butter, 4, sizeof(vertex_t));
  if (!alloc.mapped) {
    cheese_log_error(
        "Failed to allocate vertex buffer for >1.0f thickness draw_line");
    return;
  }

  vertex_t *vertices = alloc.mapped;
  vertices[0] = (vertex_t){px[0], py[0], 0, 0, r, g, b, a};
  vertices[1] = (vertex_t){px[1], py[1], 0, 0, r, g, b, a};
  vertices[2] = (vertex_t){px[2], py[2], 0, 0, r, g, b, a};
  vertices[3] = (vertex_t){px[3], py[3], 0, 0, r, g, b, a};

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

  butter_submit_draws(butter, &cmd, 1);
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

  if (thickness > 0.0f && thickness <= 1.0f &&
      renderer->line_pipeline.pipeline != VK_NULL_HANDLE) {
    u32 vcount = segments + 1;
    butter_allocation_t alloc =
        butter_alloc_vertices(butter, vcount, sizeof(vertex_t));
    if (!alloc.mapped) {
      cheese_log_error("Failed to allocate vertex buffer for >0.0f <1.0f "
                       "thickness draw_arc");
      return;
    }

    vertex_t *vertices = alloc.mapped;
    for (u32 i = 0; i <= vcount; i++) {
      f32 t = start_angle + (f32)i / (f32)segments * angle_range;
      vertices[i] = (vertex_t){
          cx + radius * cosf(t), cy + radius * sinf(t), 0, 0, r, g, b, a};
    }

    butter_draw_cmd_t cmd = {0};
    cmd.pipeline = renderer->line_pipeline;
    cmd.vertex_count = vcount;
    cmd.vertex_buffer = alloc.buffer;
    cmd.vertex_offset = alloc.offset;
    cmd.scissor_enabled = clip_active;
    if (clip_active)
      cmd.scissor = current_clip;

    butter_submit_draws(butter, &cmd, 1);
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
    vertices[0] = (vertex_t){cx, cy, 0, 0, r, g, b, a};
    for (u32 i = 0; i <= segments; i++) {
      f32 t = start_angle + (f32)i / (f32)segments * angle_range;
      vertices[i + 1] = (vertex_t){
          cx + radius * cosf(t), cy + radius * sinf(t), 0, 0, r, g, b, a};
    }

    butter_draw_cmd_t cmd = {0};
    cmd.pipeline = renderer->solid_pipeline;
    cmd.vertex_buffer = v_alloc.buffer;
    cmd.vertex_offset = v_alloc.offset;
    cmd.vertex_count = vcount;
    cmd.scissor_enabled = clip_active;
    if (clip_active)
      cmd.scissor = current_clip;

    butter_submit_draws(butter, &cmd, 1);
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

    vertices[i * 2] =
        (vertex_t){cx + radius * cos_t, cy + radius * sin_t, 0, 0, r, g, b, a};

    vertices[i * 2 + 1] = (vertex_t){
        cx + inner_radius * cos_t, cy + inner_radius * sin_t, 0, 0, r, g, b, a};
  }

  butter_draw_cmd_t cmd = {0};
  cmd.pipeline = renderer->solid_pipeline;
  cmd.vertex_buffer = v_alloc.buffer;
  cmd.vertex_offset = v_alloc.offset;
  cmd.vertex_count = vcount;
  cmd.scissor_enabled = clip_active;
  if (clip_active)
    cmd.scissor = current_clip;

  butter_submit_draws(butter, &cmd, 1);
}

static void butter_draw_text(void *userdata, f32 x, f32 y, const string *text,
                             cheese_font_t *font, cheese_color_t color,
                             f32 scale) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  if (!font || !font->active_variant || !text || text->len == 0) {
    cheese_log_error("Invalid text or font");
    return;
  }

  if (!font->active_variant->atlas_texture_id) {
    cheese_font_rebuild_atlas(font);
  }

  hb_buffer_t *hb_buffer = hb_buffer_create();
  hb_buffer_add_utf8(hb_buffer, (cstr *)text->base, text->len, 0, -1);
  hb_buffer_guess_segment_properties(hb_buffer);
  hb_shape(font->active_variant->hb_font, hb_buffer, NULL, 0);

  u32 glyph_count;
  hb_glyph_info_t *glyph_info =
      hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
  hb_glyph_position_t *glyph_pos =
      hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

  f32 cursor_x = x;
  f32 cursor_y = y;

  for (u32 i = 0; i < glyph_count; i++) {
    hb_codepoint_t glyph_id = glyph_info[i].codepoint;

    f32 x_advance = (f32)glyph_pos[i].x_advance / 64.0f;
    f32 y_advance = (f32)glyph_pos[i].y_advance / 64.0f;
    f32 x_offset = (f32)glyph_pos[i].x_offset / 64.0f;
    f32 y_offset = (f32)glyph_pos[i].y_offset / 64.0f;

    cheese_glyph_t *glyph = cheese_font_get_glyph(font, glyph_id);
    if (!glyph || glyph->width == 0 || glyph->height == 0) {
      cursor_x += x_advance * scale;
      cursor_y += y_advance * scale;
      continue;
    }

    /*if (glyph_id == FT_Get_Char_Index(font->ft_face, 'M')) {
      cheese_log_debug(
          "M %d: bearing_x=%f, width=%f, u0=%f, v0=%f, u1=%f, v1=%f", glyph_id,
          glyph->bearing_x, glyph->width, glyph->u0, glyph->v0, glyph->u1,
          glyph->v1);
    }*/

    f32 w = glyph->width * scale;
    f32 h = glyph->height * scale;
    f32 off_x = (glyph->bearing_x + x_offset) * scale;
    f32 off_y = (glyph->bearing_y + y_offset) * scale;

    butter_draw_quad_textured(renderer, cursor_x + off_x, cursor_y - off_y, w,
                              h, glyph->u0, glyph->v0, glyph->u1, glyph->v1,
                              font->active_variant->atlas_texture_id, color);

    cursor_x += x_advance * scale;
    cursor_y += y_advance * scale;
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
                                  const u8 *data) {
  butter_renderer_t *renderer = (butter_renderer_t *)userdata;
  butter_t *butter = renderer->butter;

  butter_texture_t *tex = butter_create_texture(
      butter, width, height, VK_FORMAT_R8G8B8A8_SRGB, data,
      (u64)width * height * 4, renderer->default_sampler);

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
  butter_t *butter = renderer->butter;
  butter_texture_deregister(butter, texture_id);
}

static butter_shader_t load_shader(arena_t *arena, const string *path,
                                   butter_shader_stage_t stage) {
  cheese_log_debug("Loading shader '%.*s'", path->len, path->base);
  string *code = read_file(arena, path);
  if (!code || code->len == 0) {
    cheese_log_error("Failed to load shader '%s'", path);
    return (butter_shader_t){0};
  }

  return (butter_shader_t){
      .stage = stage,
      .code = code->base,
      .code_size = code->len,
      .entry_point = "main",
      .spec = null,
  };
}

cheese_renderer_t cheese_create_butter_renderer(butter_t *butter,
                                                arena_t *arena) {
  butter_renderer_t *renderer = arena_alloc_zeroed(arena, butter_renderer_t, 1);
  renderer->butter = butter;
  renderer->clip_stack_depth = 0;

  cheese_log_debug("Creating atrributes");
  butter_attribute_t attrs[3] = {
      {
          .location = 0,
          .type = BUTTER_ATTRIB_POSITION_2D,
          .offset = offsetof(vertex_t, x),
      },
      {
          .location = 1,
          .type = BUTTER_ATTRIB_UV,
          .offset = offsetof(vertex_t, u),
      },
      {
          .location = 2,
          .type = BUTTER_ATTRIB_COLOR,
          .offset = offsetof(vertex_t, r),
      },
  };

  string *solid_vert_path = string_from_cstr(arena, "solid.vert.spv");
  string *solid_frag_path = string_from_cstr(arena, "solid.frag.spv");

  string *textured_frag_path = string_from_cstr(arena, "textured.frag.spv");

  butter_shader_t solid_vert_shader =
      load_shader(arena, solid_vert_path, BUTTER_STAGE_VERTEX);
  butter_shader_t solid_frag_shader =
      load_shader(arena, solid_frag_path, BUTTER_STAGE_FRAGMENT);
  butter_shader_t textured_frag_shader =
      load_shader(arena, textured_frag_path, BUTTER_STAGE_FRAGMENT);

  butter_shader_t solid_shaders[2] = {
      solid_vert_shader,
      solid_frag_shader,
  };

  butter_shader_t textured_shaders[2] = {
      solid_vert_shader,
      textured_frag_shader,
  };

  cheese_log_debug("Creating solid pipeline");
  butter_pipeline_desc_t solid_desc = butter_pipeline_desc_default();
  butter_pipeline_desc_add_attributes(&solid_desc, attrs, 3);
  butter_pipeline_desc_add_shaders(&solid_desc, solid_shaders, 2);
  butter_pipeline_desc_set_vertex_stride(&solid_desc, sizeof(vertex_t));

  solid_desc.cull_mode = BUTTER_CULL_NONE;
  solid_desc.topology = BUTTER_TOPOLOGY_TRIANGLE_STRIP;

  renderer->solid_pipeline =
      butter_create_pipeline(butter, &solid_desc, butter->render_pass);
  if (renderer->solid_pipeline.pipeline == VK_NULL_HANDLE) {
    cheese_log_fatal("Failed to create solid pipeline");
    return (cheese_renderer_t){0};
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

  renderer->textured_pipeline =
      butter_create_pipeline(butter, &textured_desc, butter->render_pass);
  if (renderer->textured_pipeline.pipeline == VK_NULL_HANDLE) {
    cheese_log_fatal("Failed to create textured pipeline");
    butter_destroy_pipeline(butter, &renderer->solid_pipeline);
    return (cheese_renderer_t){0};
  } else {
    cheese_log_debug("Textured pipeline created: pipeline=%p, layout=%p",
                     (void *)renderer->textured_pipeline.pipeline,
                     (void *)renderer->textured_pipeline.layout);
  }

  cheese_log_debug("Creating line pipeline");
  butter_pipeline_desc_t line_desc = butter_pipeline_desc_default();
  butter_pipeline_desc_add_attributes(&line_desc, attrs, 3);
  butter_pipeline_desc_add_shaders(&line_desc, solid_shaders, 2);
  butter_pipeline_desc_set_vertex_stride(&line_desc, sizeof(vertex_t));

  line_desc.cull_mode = BUTTER_CULL_NONE;
  line_desc.topology = BUTTER_TOPOLOGY_LINE_STRIP;

  renderer->line_pipeline =
      butter_create_pipeline(butter, &line_desc, butter->render_pass);
  if (renderer->line_pipeline.pipeline == VK_NULL_HANDLE)
    cheese_log_warning("Failed to create line pipeline, falling back to solid");

  cheese_log_debug("Creating default sampler");
  butter_sampler_desc_t sampler_desc = butter_sampler_desc_nearest_clamp();
  renderer->default_sampler = butter_create_sampler(butter, &sampler_desc);
  if (renderer->default_sampler == VK_NULL_HANDLE) {
    cheese_log_fatal("Failed to create default sampler");
    if (renderer->line_pipeline.pipeline != VK_NULL_HANDLE)
      butter_destroy_pipeline(butter, &renderer->line_pipeline);
    butter_destroy_pipeline(butter, &renderer->textured_pipeline);
    butter_destroy_pipeline(butter, &renderer->solid_pipeline);
  }

  cheese_log_debug("Creating renderer");
  cheese_renderer_t cheese_renderer = {0};
  cheese_renderer.userdata = renderer;
  cheese_renderer.create_texture = butter__create_texture;
  cheese_renderer.delete_texture = butter__destroy_texture;

  cheese_renderer.draw_rect = butter_draw_rect;
  cheese_renderer.draw_texture = butter_draw_texture;
  cheese_renderer.draw_line = butter_draw_line;
  cheese_renderer.draw_arc = butter_draw_arc;
  cheese_renderer.draw_text = butter_draw_text;
  cheese_renderer.push_clip = butter_push_clip;
  cheese_renderer.pop_clip = butter_pop_clip;

  return cheese_renderer;
}

void cheese_destroy_butter_renderer(cheese_renderer_t *renderer) {
  if (!renderer || !renderer->userdata)
    return;

  butter_renderer_t *butter_renderer = (butter_renderer_t *)renderer->userdata;

  if (butter_renderer->solid_pipeline.pipeline)
    butter_destroy_pipeline(butter_renderer->butter,
                            &butter_renderer->solid_pipeline);

  if (butter_renderer->textured_pipeline.pipeline)
    butter_destroy_pipeline(butter_renderer->butter,
                            &butter_renderer->textured_pipeline);

  if (butter_renderer->line_pipeline.pipeline)
    butter_destroy_pipeline(butter_renderer->butter,
                            &butter_renderer->line_pipeline);

  if (butter_renderer->default_sampler)
    butter_destroy_sampler(butter_renderer->butter,
                           butter_renderer->default_sampler);

  renderer->userdata = null;
}
