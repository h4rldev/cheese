/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/widgets/list.h>

/***********************************/

i32 cheese_list(cheese_t *cheese, const cstr *classes,
                cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                cheese_value_t scroll_y, u32 count, f32 row_height,
                cheese_list_item_fn draw_item, void *userdata) {
  if (!cheese || !cheese->renderer || row_height <= 0.0f)
    return -1;

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_LIST, classes, null);

  f32 content_h = (f32)count * row_height;
  f32 max_scroll = max(0.0f, content_h - h);
  if (max_scroll > 0.0f && !scroll_y.state)
    cheese_log_warn_once("cheese_list: %.0fpx of content in a %.0fpx viewport "
                         "with no bound scroll value; the list can not scroll",
                         content_h, h);

  f32 scroll = cheese_value_f32(scroll_y);
  if (scroll < 0.0f)
    scroll = 0.0f;
  if (scroll > max_scroll)
    scroll = max_scroll;

  b32 over = cheese->mouse_x >= x && cheese->mouse_x <= x + w &&
             cheese->mouse_y >= y && cheese->mouse_y <= y + h;

  if (over && scroll_y.state && cheese->scroll_y != 0.0f) {
    f32 step = max(row_height * 3.0f, h * 0.25f);
    scroll -= cheese->scroll_y * step;
    if (scroll < 0.0f)
      scroll = 0.0f;
    if (scroll > max_scroll)
      scroll = max_scroll;
    cheese_value_set_f32(scroll_y, scroll);
  }

  cheese_semantics_begin_node(cheese, semantics, CHEESE_ROLE_CONTAINER, null,
                              (cheese_rect_t){(i32)x, (i32)y, (u32)w, (u32)h});

  b32 clip = cheese->renderer->push_clip != null;
  if (clip)
    cheese_push_clip(cheese, x, y, w, h);

  i32 first = (i32)(scroll / row_height);
  f32 range = scroll + h;
  i32 last = (i32)(range / row_height);
  if (range - (f32)last * row_height > 0.0f)
    last++;
  if (first < 0)
    first = 0;
  if (last > (i32)count)
    last = (i32)count;

  b32 clicked =
      (cheese->mouse_buttons & ~cheese->mouse_prev_buttons) & CHEESE_MOUSE_LEFT;
  i32 result = -1;

  for (i32 i = first; i < last; i++) {
    f32 ry = y - scroll + (f32)i * row_height;
    b32 row_hovered =
        over && cheese->mouse_y >= ry && cheese->mouse_y <= ry + row_height;
    cheese_rect_t row = {(i32)x, (i32)ry, (u32)w, (u32)row_height};

    if (draw_item)
      draw_item(cheese, i, row, row_hovered, userdata);

    if (row_hovered && clicked)
      result = i;
  }

  if (clip)
    cheese_pop_clip(cheese);

  cheese_semantics_end_node(cheese);

  return result;
}

i32 cheese_list_auto(cheese_t *cheese, const cstr *classes,
                     cheese_semantics_t semantics, cheese_value_t scroll_y,
                     u32 count, f32 row_height, cheese_list_item_fn draw_item,
                     void *userdata) {
  cheese_layout_t *layout = cheese_current_layout(cheese);

  f32 w = layout->width > 0.0f ? layout->width : 200.0f;
  f32 h = layout->height > 0.0f ? layout->height : 200.0f;

  f32 x, y;
  cheese_layout_place(cheese, &w, &h, &x, &y);

  return cheese_list(cheese, classes, semantics, x, y, w, h, scroll_y, count,
                     row_height, draw_item, userdata);
}
