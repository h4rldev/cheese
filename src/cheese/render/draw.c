/***********************************/

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/selection.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

/***********************************/

void cheese_flush_draws(cheese_t *cheese) {
  if (!cheese || !cheese->renderer || !cheese->renderer->flush_draws) {
    cheese_log_error(
        "cheese_flush_draws: Invalid renderer or missing flush callback");
    return;
  }

  cheese->renderer->flush_draws(cheese->renderer->userdata);
}

void cheese_draw_rect(cheese_t *cheese, cheese_corners_t radius, f32 x, f32 y,
                      f32 w, f32 h, cheese_color_t color) {
  if (!cheese || !cheese->renderer || !cheese->renderer->draw_rect) {
    cheese_log_error(
        "cheese_draw_rect: Invalid renderer or missing draw callback");
    return;
  }

  cheese->renderer->draw_rect(cheese->renderer->userdata, radius, x, y, w, h,
                              color);
}

void cheese_draw_border(cheese_t *cheese, const cheese_style_t *style, f32 x,
                        f32 y, f32 w, f32 h) {
  if (!cheese || !cheese->renderer || !cheese->renderer->draw_border || !style)
    return;

  f32 width =
      cheese_style_get_prop_f32(style, cheese->core_props.border_width, -1.0f);
  if (width <= 0.0f)
    return;

  u32 sides =
      cheese_style_get_prop_u32(style, cheese->core_props.border_sides, 0);
  if (sides == 0)
    sides = CHEESE_SIDE_ALL;

  cheese_color_t color =
      cheese_style_get_prop_color(style, cheese->core_props.border_color, 0);

  cheese->renderer->draw_border(cheese->renderer->userdata,
                                style->corner_radius, x, y, w, h, width, sides,
                                color);
}

void cheese_draw_focus_ring(cheese_t *cheese, const cheese_style_t *style,
                            f32 x, f32 y, f32 w, f32 h) {
  if (!cheese || !style || !cheese->renderer || !cheese->renderer->draw_border)
    return;

  f32 width = cheese_style_get_prop_f32(
      style, cheese->core_props.focus_ring_width, -1.0f);
  cheese_color_t color = cheese_style_get_prop_color(
      style, cheese->core_props.focus_ring_color, 0);
  if (width <= 0.0f || color == 0)
    return;

  f32 off = cheese_style_get_prop_f32(
      style, cheese->core_props.focus_ring_offset, 0.0f);
  if (off < 0.0f)
    off = 0.0f;

  cheese_corners_t radius = {
      max(0.0f, style->corner_radius.top_left + off),
      max(0.0f, style->corner_radius.top_right + off),
      max(0.0f, style->corner_radius.bottom_left + off),
      max(0.0f, style->corner_radius.bottom_right + off),
  };

  cheese->renderer->draw_border(cheese->renderer->userdata, radius, x - off,
                                y - off, w + off * 2.0f, h + off * 2.0f, width,
                                CHEESE_SIDE_ALL, color);
}

void cheese_draw_texture(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                         u32 texture_id, cheese_color_t color) {
  if (!cheese || !cheese->renderer || !cheese->renderer->draw_texture) {
    cheese_log_error(
        "cheese_draw_texture: Invalid renderer or missing draw callback");
    return;
  }

  cheese->renderer->draw_texture(cheese->renderer->userdata, x, y, w, h,
                                 texture_id, color);
}

cheese_texture_t cheese_texture_upload(cheese_t *cheese, u32 width, u32 height,
                                       const u8 *rgba8) {
  cheese_texture_t texture = {0};

  if (!cheese || !cheese->renderer || !cheese->renderer->create_texture)
    return texture;

  i32 id = cheese->renderer->create_texture(
      cheese->renderer->userdata, width, height, CHEESE_TEXTURE_RGBA8, rgba8);
  if (id <= 0)
    return texture;

  texture.id = (u32)id;
  texture.width = width;
  texture.height = height;
  return texture;
}

void cheese_texture_destroy(cheese_t *cheese, cheese_texture_t texture) {
  if (!cheese || !texture.id || !cheese->renderer ||
      !cheese->renderer->delete_texture)
    return;

  cheese->renderer->delete_texture(cheese->renderer->userdata, texture.id);
}

void cheese_update_texture_region(cheese_t *cheese, u32 texture_id, i32 x,
                                  i32 y, u32 w, u32 h, const void *data,
                                  u64 data_size) {
  if (!cheese || !cheese->renderer ||
      !cheese->renderer->update_texture_region) {
    cheese_log_error("cheese_update_texture_region: Invalid renderer or "
                     "missing update callback");
    return;
  }

  cheese->renderer->update_texture_region(
      cheese->renderer->userdata, texture_id, x, y, w, h, data, data_size);
}

void cheese_draw_line(cheese_t *cheese, f32 x1, f32 y1, f32 x2, f32 y2,
                      f32 thickness, cheese_color_t color) {
  if (!cheese || !cheese->renderer || !cheese->renderer->draw_line) {
    cheese_log_error(
        "cheese_draw_line: Invalid renderer or missing draw callback");
    return;
  }

  cheese->renderer->draw_line(cheese->renderer->userdata, x1, y1, x2, y2,
                              thickness, color);
}

void cheese_draw_arc(cheese_t *cheese, f32 cx, f32 cy, f32 radius,
                     f32 start_angle, f32 end_angle, f32 thickness,
                     cheese_color_t color) {
  if (!cheese || !cheese->renderer || !cheese->renderer->draw_arc) {
    cheese_log_error(
        "cheese_draw_arc: Invalid renderer or missing draw callback");
    return;
  }

  cheese->renderer->draw_arc(cheese->renderer->userdata, cx, cy, radius,
                             start_angle, end_angle, thickness, color);
}

void cheese_draw_text(cheese_t *cheese, f32 x, f32 y, const string *text,
                      cheese_font_t *font, cheese_color_t color, f32 scale) {
  if (!cheese || !cheese->renderer || !cheese->renderer->draw_text) {
    cheese_log_error(
        "cheese_draw_text: Invalid renderer or missing draw callback");
    return;
  }

  cheese_style_t *style = cheese_current_style(cheese);
  if (style && font) {
    u32 target_size = style->font_size ? style->font_size : font->default_size;
    cheese_font_set_size(font, target_size);
  }

  if (style && cheese_style_get_selectable(style))
    cheese_selection_record(cheese, text, font, x, y);

  cheese->renderer->draw_text(cheese->renderer->userdata, x, y, text, font,
                              color, scale);
}

void cheese_push_clip(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h) {
  if (!cheese || !cheese->renderer || !cheese->renderer->push_clip) {
    cheese_log_error(
        "cheese_push_clip: Invalid renderer or missing draw callback");
    return;
  }

  cheese->renderer->push_clip(cheese->renderer->userdata, x, y, w, h);
}

void cheese_pop_clip(cheese_t *cheese) {
  if (!cheese || !cheese->renderer || !cheese->renderer->pop_clip) {
    cheese_log_error(
        "cheese_pop_clip: Invalid renderer or missing draw callback");
    return;
  }

  cheese->renderer->pop_clip(cheese->renderer->userdata);
}
