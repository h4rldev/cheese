/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>

#include <cheese/widgets/container.h>

/***********************************/

void cheese_begin_container(cheese_t *cheese, const cstr *classes,
                            cheese_semantics_t semantics, f32 x, f32 y,
                            cheese_size_t w, cheese_size_t h,
                            const cheese_layout_t *layout) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_CONTAINER, classes,
                              null);

  cheese_layout_t *parent = cheese_current_layout(cheese);
  f32 rw = cheese_size_resolve(w, parent ? parent->width : 0.0f, 0.0f);
  f32 rh = cheese_size_resolve(h, parent ? parent->height : 0.0f, 0.0f);
  if (rw < 0.0f)
    rw = 0.0f;
  if (rh < 0.0f)
    rh = 0.0f;

  cheese_semantics_begin_node(
      cheese, semantics, CHEESE_ROLE_CONTAINER, null,
      (cheese_rect_t){(i32)x, (i32)y, (u32)rw, (u32)rh});

  cheese_push_scope(cheese, cheese_style_new(), classes);

  f32 pad_left = cheese_style_get_pad_left(&style);
  f32 pad_bottom = cheese_style_get_pad_bottom(&style);
  f32 pad_top = cheese_style_get_pad_top(&style);
  f32 pad_right = cheese_style_get_pad_right(&style);

  cheese_draw_rect(cheese, style.corner_radius, x, y, rw, rh, style.bg_color);
  cheese_draw_border(cheese, &style, x, y, rw, rh);

  f32 clip_x = x + pad_left - 1.0f;
  f32 clip_y = y + pad_top - 1.0f;
  f32 clip_w = rw - pad_left - pad_right + 2.0f;
  f32 clip_h = rh - pad_top - pad_bottom + 2.0f;

  cheese_push_clip(cheese, clip_x, clip_y, clip_w, clip_h);

  cheese_layout_t child = *layout;
  child.x = x + pad_left;
  child.y = y + pad_top;
  child.width = rw - pad_left - pad_right;
  child.height = rh - pad_top - pad_bottom;
  child.origin_x = child.x;
  child.origin_y = child.y;
  child.line_cross = 0.0f;
  child.container_w = rw;
  child.container_h = rh;
  child.anchor = CHEESE_ANCHOR_FLOW;

  cheese_push_layout(cheese, child);
}

void cheese_begin_container_auto(cheese_t *cheese, const cstr *classes,
                                 cheese_semantics_t semantics, cheese_size_t w,
                                 cheese_size_t h) {
  cheese_layout_t *layout = cheese_current_layout(cheese);

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_CONTAINER, classes,
                              null);

  f32 margin_left = cheese_style_get_margin_left(&style);
  f32 margin_bottom = cheese_style_get_margin_bottom(&style);
  f32 margin_top = cheese_style_get_margin_top(&style);
  f32 margin_right = cheese_style_get_margin_right(&style);

  f32 rw = cheese_size_resolve(w, layout->width, 0.0f);
  f32 rh = cheese_size_resolve(h, layout->height, 0.0f);

  f32 x, y;
  cheese_layout_place(cheese, &rw, &rh, &x, &y);

  f32 inner_w = rw - margin_left - margin_right;
  f32 inner_h = rh - margin_bottom - margin_top;
  if (inner_w < 0.0f)
    inner_w = 0.0f;
  if (inner_h < 0.0f)
    inner_h = 0.0f;

  cheese_begin_container(cheese, classes, semantics, x + margin_left,
                         y + margin_top, cheese_px(inner_w), cheese_px(inner_h),
                         layout);
}

void cheese_begin_scroll(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, f32 x, f32 y,
                         cheese_size_t w, cheese_size_t h,
                         cheese_value_t scroll_x, cheese_value_t scroll_y) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_SCROLL, classes,
                              null);

  cheese_layout_t *parent = cheese_current_layout(cheese);
  f32 rw = cheese_size_resolve(w, parent ? parent->width : 0.0f, 0.0f);
  f32 rh = cheese_size_resolve(h, parent ? parent->height : 0.0f, 0.0f);

  f32 sx = cheese_value_f32(scroll_x);
  f32 sy = cheese_value_f32(scroll_y);

  if (cheese->scroll_stack_depth < CHEESE_STACK_MAX_DEPTH) {
    cheese->scroll_stack_x[cheese->scroll_stack_depth] = scroll_x;
    cheese->scroll_stack_y[cheese->scroll_stack_depth] = scroll_y;
    cheese->scroll_stack_depth++;
  }

  cheese_semantics_begin_node(
      cheese, semantics, CHEESE_ROLE_CONTAINER, null,
      (cheese_rect_t){(i32)x, (i32)y, (u32)rw, (u32)rh});

  cheese_push_scope(cheese, cheese_style_new(), classes);
  cheese_draw_rect(cheese, style.corner_radius, x, y, rw, rh, style.bg_color);
  cheese_draw_border(cheese, &style, x, y, rw, rh);

  cheese_push_clip(cheese, x, y, rw, rh);

  cheese_layout_t child = *parent;
  child.x = x - sx;
  child.y = y - sy;
  child.width = rw;
  child.height = rh;
  child.origin_x = child.x;
  child.origin_y = child.y;
  child.line_cross = 0.0f;
  child.container_w = rw;
  child.container_h = rh;
  child.anchor = CHEESE_ANCHOR_FLOW;

  cheese_push_layout(cheese, child);
}

void cheese_end_scroll(cheese_t *cheese) {
  if (cheese->scroll_stack_depth > 0) {
    u32 i = --cheese->scroll_stack_depth;
    cheese_layout_t *child = cheese_current_layout(cheese);
    f32 used_w = child->x - child->origin_x;
    f32 used_h = child->y - child->origin_y;
    if (!cheese->scroll_stack_x[i].state && used_w > child->width)
      cheese_log_warn_once("cheese_scroll: %.0fpx of content in a %.0fpx "
                           "viewport with no bound scroll_x; it can not scroll",
                           used_w, child->width);
    if (!cheese->scroll_stack_y[i].state && used_h > child->height)
      cheese_log_warn_once("cheese_scroll: %.0fpx of content in a %.0fpx "
                           "viewport with no bound scroll_y; it can not scroll",
                           used_h, child->height);
  }
  cheese_pop_layout(cheese);
  cheese_pop_clip(cheese);
  cheese_pop_scope(cheese);
}

void cheese_end_container(cheese_t *cheese) {
  cheese_pop_layout(cheese);
  cheese_pop_clip(cheese);
  cheese_pop_scope(cheese);
}
