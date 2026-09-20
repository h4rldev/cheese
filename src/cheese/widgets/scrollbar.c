/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>

#include <cheese/widgets/scrollbar.h>
#include <cheese/widgets/widget.h>

/***********************************/

#define CHEESE_SCROLLBAR_MIN_THUMB 24.0f
#define CHEESE_SCROLLBAR_THICKNESS 12.0f

typedef struct {
  f32 track;
  f32 thumb;
  f32 travel;
  f32 offset;
  f32 max_scroll;
} cheese_scrollbar_layout_t;

//
//
//

static void cheese_scrollbar_layout(f32 w, f32 h, f32 viewport, f32 content,
                                    f32 scroll, cheese_scrollbar_axis_t axis,
                                    cheese_scrollbar_layout_t *out) {
  f32 track = axis == CHEESE_SCROLLBAR_VERTICAL ? h : w;
  f32 max_scroll = max(0.0f, content - viewport);
  f32 ratio = content > 0.0f ? viewport / content : 1.0f;

  f32 thumb = track * ratio;
  if (thumb < CHEESE_SCROLLBAR_MIN_THUMB)
    thumb = CHEESE_SCROLLBAR_MIN_THUMB;
  if (thumb > track)
    thumb = track;

  f32 travel = track - thumb;
  f32 clamped = min(max_scroll, max(0.0f, scroll));

  out->track = track;
  out->thumb = thumb;
  out->travel = travel;
  out->offset = max_scroll > 0.0f ? (clamped / max_scroll) * travel : 0.0f;
  out->max_scroll = max_scroll;
}

//
//
//

f32 cheese_scrollbar_begin(cheese_t *cheese, const cheese_widget_t *scope,
                           cheese_value_t scroll, f32 viewport, f32 content,
                           cheese_scrollbar_axis_t axis, b32 editing) {
  if (!cheese || !scope)
    return cheese_value_f32(scroll);

  f32 v = cheese_value_f32(scroll);
  f32 next = v;
  f32 max_scroll = max(0.0f, content - viewport);

  if (scope->hovered) {
    u32 clicked = cheese->mouse_buttons & ~cheese->mouse_prev_buttons;
    if (clicked & CHEESE_MOUSE_LEFT)
      cheese_capture(cheese, scope->id);
  }

  if (cheese_captured(cheese, scope->id)) {
    cheese_scrollbar_layout_t l;
    cheese_scrollbar_layout(scope->w, scope->h, viewport, content, v, axis, &l);

    f32 pointer = axis == CHEESE_SCROLLBAR_VERTICAL
                      ? cheese->mouse_y - scope->y
                      : cheese->mouse_x - scope->x;
    f32 target = pointer - l.thumb * 0.5f;
    next = l.travel > 0.0f ? (target / l.travel) * l.max_scroll : 0.0f;
  } else if (editing) {
    f32 line = max(1.0f, viewport * 0.1f);

    if (cheese_key_pressed(cheese, CHEESE_KEY_UP) ||
        cheese_key_pressed(cheese, CHEESE_KEY_LEFT))
      next -= line;
    if (cheese_key_pressed(cheese, CHEESE_KEY_DOWN) ||
        cheese_key_pressed(cheese, CHEESE_KEY_RIGHT))
      next += line;
    if (cheese_key_pressed(cheese, CHEESE_KEY_PAGE_UP))
      next -= viewport;
    if (cheese_key_pressed(cheese, CHEESE_KEY_PAGE_DOWN))
      next += viewport;
  }

  next = min(max_scroll, max(0.0f, next));
  if (next != v) {
    v = next;
    cheese_value_set_f32(scroll, v);
  }

  return v;
}

f32 cheese_scrollbar(cheese_t *cheese, const cstr *classes,
                     cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                     cheese_value_t scroll, f32 viewport, f32 content,
                     cheese_scrollbar_axis_t axis) {
  if (!cheese || !cheese->renderer)
    return cheese_value_f32(scroll);

  cheese_widget_t scope;
  cheese_widget_begin(cheese, semantics, CHEESE_ROLE_SCROLLBAR, x, y, w, h,
                      &scope);

  u32 state = scope.state;
  if (scope.pressed || scope.active)
    state |= CHEESE_STATE_PRESSED;

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_SCROLLBAR, classes,
                              null);
  cheese_style_apply_state(&style, state);

  if (scope.hovered)
    cheese_cursor_request(cheese, style.cursor >= 0
                                      ? (cheese_cursor_t)style.cursor
                                      : CHEESE_CURSOR_DEFAULT);

  b32 editing = cheese->edit_id && cheese->edit_id == scope.id;
  u32 clicked = cheese->mouse_buttons & ~cheese->mouse_prev_buttons;

  if (scope.hovered && (clicked & CHEESE_MOUSE_LEFT)) {
    cheese->focus_id = scope.id;
    cheese->edit_id = scope.id;
    editing = true;
  }

  if (scope.focused && !editing && cheese_widget_activated(cheese, &scope)) {
    cheese->edit_id = scope.id;
    editing = true;
  }

  if (editing && cheese_key_pressed(cheese, CHEESE_KEY_ESCAPE)) {
    cheese->edit_id = 0;
    editing = false;
  }

  f32 v = cheese_scrollbar_begin(cheese, &scope, scroll, viewport, content,
                                 axis, editing);

  cheese_scrollbar_layout_t l;
  cheese_scrollbar_layout(w, h, viewport, content, v, axis, &l);

  f32 thickness = axis == CHEESE_SCROLLBAR_VERTICAL ? w : h;
  cheese_corners_t radius = style.corner_radius;
  if (radius.top_left < 0.0f) {
    f32 r = thickness * 0.5f;
    radius = (cheese_corners_t){r, r, r, r};
  }

  f32 fraction = l.max_scroll > 0.0f ? v / l.max_scroll : 0.0f;
  const cstr *value_str =
      cheese_semantics_format(cheese, "%d%%", (i32)(fraction * 100.0f + 0.5f));

  cheese_widget_emit(cheese, semantics, CHEESE_ROLE_SCROLLBAR, null, value_str,
                     state, &scope);

  cheese_color_t track_color =
      style.bg_color ? style.bg_color : cheese_color_rgb(40, 40, 40);
  cheese_color_t thumb_color =
      style.text_color ? style.text_color : cheese_color_rgb(255, 255, 255);

  cheese_draw_rect(cheese, radius, x, y, w, h, track_color);

  f32 tx = x, ty = y, tw = w, th = h;
  if (axis == CHEESE_SCROLLBAR_VERTICAL) {
    ty = y + l.offset;
    th = l.thumb;
  } else {
    tx = x + l.offset;
    tw = l.thumb;
  }

  cheese_draw_rect(cheese, radius, tx, ty, tw, th, thumb_color);
  cheese_draw_border(cheese, &style, x, y, w, h);

  return v;
}

f32 cheese_scrollbar_auto(cheese_t *cheese, const cstr *classes,
                          cheese_semantics_t semantics, cheese_value_t scroll,
                          f32 viewport, f32 content,
                          cheese_scrollbar_axis_t axis) {
  cheese_layout_t *layout = cheese_current_layout(cheese);

  f32 w, h;
  if (axis == CHEESE_SCROLLBAR_VERTICAL) {
    w = CHEESE_SCROLLBAR_THICKNESS;
    h = layout->height > 0.0f ? layout->height : 100.0f;
  } else {
    w = layout->width > 0.0f ? layout->width : 100.0f;
    h = CHEESE_SCROLLBAR_THICKNESS;
  }

  f32 x, y;
  cheese_layout_place(cheese, &w, &h, &x, &y);

  return cheese_scrollbar(cheese, classes, semantics, x, y, w, h, scroll,
                          viewport, content, axis);
}
