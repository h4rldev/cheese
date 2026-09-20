/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/input.h>
#include <cheese/core/semantics.h>

#include <cheese/widgets/widget.h>

/***********************************/

void cheese_widget_begin(cheese_t *cheese, cheese_semantics_t semantics,
                         cheese_role_t role, f32 x, f32 y, f32 w, f32 h,
                         cheese_widget_t *out) {
  if (!cheese || !out)
    return;

  out->x = x;
  out->y = y;
  out->w = w;
  out->h = h;
  out->id = cheese_semantics_peek_id(cheese, semantics, role);
  out->hovered = cheese->mouse_x >= x && cheese->mouse_x <= x + w &&
                 cheese->mouse_y >= y && cheese->mouse_y <= y + h;
  out->focused = cheese->focus_id && cheese->focus_id == out->id;
  out->pressed = out->hovered && (cheese->mouse_buttons & CHEESE_MOUSE_LEFT);
  out->active = cheese_captured(cheese, out->id);

  out->state = 0;
  if (out->hovered)
    out->state |= CHEESE_STATE_HOVERED;
  if (out->focused)
    out->state |= CHEESE_STATE_FOCUSED;
}

b32 cheese_widget_activated(const cheese_t *cheese, const cheese_widget_t *w) {
  if (!cheese || !w)
    return false;

  u32 clicked = cheese->mouse_buttons & ~cheese->mouse_prev_buttons;
  if (w->hovered && (clicked & CHEESE_MOUSE_LEFT))
    return true;

  return w->focused && (cheese_key_pressed(cheese, CHEESE_KEY_ENTER) ||
                        cheese_key_pressed(cheese, CHEESE_KEY_SPACE));
}

i32 cheese_widget_emit(cheese_t *cheese, cheese_semantics_t semantics,
                       cheese_role_t role, const cstr *name, const cstr *value,
                       u32 extra_state, const cheese_widget_t *w) {
  if (!cheese || !w)
    return -1;

  return cheese_semantics_emit(
      cheese, semantics, role, name, value, w->state | extra_state,
      (cheese_rect_t){(i32)w->x, (i32)w->y, (u32)w->w, (u32)w->h});
}
