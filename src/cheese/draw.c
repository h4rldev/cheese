#include <cheese/draw.h>
#include <cheese/log.h>
#include <cheese/types.h>

void cheese_draw_rect(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                      cheese_color_t color) {
  if (!cheese || !cheese->renderer || !cheese->renderer->draw_rect) {
    cheese_log_error(
        "cheese_draw_rect: Invalid renderer or missing draw callback");
    return;
  }

  cheese->renderer->draw_rect(cheese->renderer->userdata, x, y, w, h, color);
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

  /*cheese_log_debug(
      "Drawing text with %fx%f dimensions, text %.*s, color %u, and scale %f",
      x, y, text->len, text->base, color, scale);*/
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
