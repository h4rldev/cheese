/***********************************/

#include <math.h>

#include <htils/basictypes.h>
#include <htils/darray.h>

#include <cheese/types.h>

#include <cheese/core/input.h>

/***********************************/

static b32 cheese_role_is_focusable(cheese_role_t role) {
  switch (role) {
  case CHEESE_ROLE_BUTTON:
  case CHEESE_ROLE_CHECKBOX:
  case CHEESE_ROLE_RADIO:
  case CHEESE_ROLE_SLIDER:
  case CHEESE_ROLE_SCROLLBAR:
  case CHEESE_ROLE_TAB:
  case CHEESE_ROLE_TEXT_INPUT:
    return true;
  default:
    return false;
  }
}

//
//
//

b32 cheese_key_pressed(const cheese_t *cheese, cheese_key_t key) {
  if (!cheese)
    return false;

  for (u32 i = 0; i < cheese->key_event_count; i++)
    if (cheese->key_events[i].key == key && !cheese->key_events[i].repeat)
      return true;

  return false;
}

u32 cheese_key_event_count(const cheese_t *cheese) {
  return cheese ? cheese->key_event_count : 0;
}

const cheese_key_event_t *cheese_key_event_at(const cheese_t *cheese,
                                              u32 index) {
  if (!cheese || index >= cheese->key_event_count)
    return null;

  return &cheese->key_events[index];
}

void cheese_focus_move(cheese_t *cheese, i32 dir) {
  if (!cheese || !cheese->semantics_nodes)
    return;

  i64 count = (i64)da_len(cheese->semantics_nodes);
  i64 current = -1;
  i64 first = -1;
  i64 last = -1;
  i64 next = -1;
  i64 prev = -1;
  i64 prev_seen = -1;

  for (i64 i = 0; i < count; i++) {
    if (!cheese_role_is_focusable(cheese->semantics_nodes[i].role))
      continue;

    if (first < 0)
      first = i;

    if (cheese->semantics_nodes[i].id == cheese->focus_id) {
      current = i;
      prev = prev_seen;
    }

    if (current >= 0 && i > current && next < 0)
      next = i;

    prev_seen = i;
    last = i;
  }

  i64 chosen;
  if (current < 0)
    chosen = (dir >= 0) ? first : last;
  else if (dir >= 0)
    chosen = (next >= 0) ? next : first;
  else
    chosen = (prev >= 0) ? prev : last;

  cheese->focus_id = (chosen >= 0) ? cheese->semantics_nodes[chosen].id : 0;
}

void cheese_focus_move_dir(cheese_t *cheese, cheese_focus_dir_t dir) {
  if (!cheese || !cheese->semantics_nodes)
    return;

  u64 count = da_len(cheese->semantics_nodes);
  i64 current = -1;

  for (u64 i = 0; i < count; i++) {
    cheese_semantics_node_t *n = &cheese->semantics_nodes[i];
    if (cheese_role_is_focusable(n->role) && n->id == cheese->focus_id) {
      current = (i64)i;
      break;
    }
  }

  if (current < 0) {
    for (u64 i = 0; i < count; i++) {
      if (cheese_role_is_focusable(cheese->semantics_nodes[i].role)) {
        cheese->focus_id = cheese->semantics_nodes[i].id;
        return;
      }
    }
    return;
  }

  cheese_rect_t cur = cheese->semantics_nodes[current].bounds;
  f32 cx = (f32)cur.x + (f32)cur.w * 0.5f;
  f32 cy = (f32)cur.y + (f32)cur.h * 0.5f;

  i64 best = -1;
  f32 best_score = 0.0f;

  for (u64 i = 0; i < count; i++) {
    if ((i64)i == current)
      continue;

    cheese_semantics_node_t *n = &cheese->semantics_nodes[i];
    if (!cheese_role_is_focusable(n->role))
      continue;

    f32 nx = (f32)n->bounds.x + (f32)n->bounds.w * 0.5f;
    f32 ny = (f32)n->bounds.y + (f32)n->bounds.h * 0.5f;
    f32 dx = nx - cx;
    f32 dy = ny - cy;
    f32 primary, cross;

    switch (dir) {
    case CHEESE_FOCUS_LEFT:
      if (dx >= -0.5f)
        continue;
      primary = -dx;
      cross = fabsf(dy);
      break;
    case CHEESE_FOCUS_RIGHT:
      if (dx <= 0.5f)
        continue;
      primary = dx;
      cross = fabsf(dy);
      break;
    case CHEESE_FOCUS_UP:
      if (dy >= -0.5f)
        continue;
      primary = -dy;
      cross = fabsf(dx);
      break;
    case CHEESE_FOCUS_DOWN:
    default:
      if (dy <= 0.5f)
        continue;
      primary = dy;
      cross = fabsf(dx);
      break;
    }

    f32 score = primary + cross * 2.0f;
    if (best < 0 || score < best_score) {
      best = (i64)i;
      best_score = score;
    }
  }

  if (best >= 0)
    cheese->focus_id = cheese->semantics_nodes[best].id;
}

void cheese_set_arrow_nav(cheese_t *cheese, b32 enabled) {
  if (cheese)
    cheese->arrow_nav = enabled;
}

void cheese_focus_arrows(cheese_t *cheese) {
  if (!cheese || !cheese->arrow_nav || cheese->edit_id)
    return;

  if (cheese_key_pressed(cheese, CHEESE_KEY_LEFT))
    cheese_focus_move_dir(cheese, CHEESE_FOCUS_LEFT);
  else if (cheese_key_pressed(cheese, CHEESE_KEY_RIGHT))
    cheese_focus_move_dir(cheese, CHEESE_FOCUS_RIGHT);
  else if (cheese_key_pressed(cheese, CHEESE_KEY_UP))
    cheese_focus_move_dir(cheese, CHEESE_FOCUS_UP);
  else if (cheese_key_pressed(cheese, CHEESE_KEY_DOWN))
    cheese_focus_move_dir(cheese, CHEESE_FOCUS_DOWN);
}

void cheese_capture(cheese_t *cheese, u64 id) {
  if (!cheese)
    return;

  cheese->active_id = id;
  cheese->press_x = cheese->mouse_x;
  cheese->press_y = cheese->mouse_y;
}

b32 cheese_captured(const cheese_t *cheese, u64 id) {
  return cheese && id != 0 && cheese->active_id == id;
}
