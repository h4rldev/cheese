/***********************************/

#include <string.h>

#include <htils/arena.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/overlay.h>
#include <cheese/core/selection.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>
#include <cheese/core/theme.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

/***********************************/

static void cheese_edit_prune(cheese_t *cheese) {
  if (!cheese || !cheese->edit_id)
    return;

  cheese_semantics_node_t *node = null;
  for (u64 i = 0; i < cheese_semantics_count(cheese); i++) {
    cheese_semantics_node_t *n = cheese_semantics_at(cheese, i);
    if (n->id == cheese->edit_id) {
      node = n;
      break;
    }
  }

  if (!node || cheese->edit_id != cheese->focus_id) {
    cheese->edit_id = 0;
    return;
  }

  u32 pressed =
      (cheese->mouse_buttons & ~cheese->mouse_prev_buttons) & CHEESE_MOUSE_LEFT;
  if (pressed) {
    cheese_rect_t b = node->bounds;
    b32 inside =
        cheese->mouse_x >= (f32)b.x && cheese->mouse_x <= (f32)b.x + (f32)b.w &&
        cheese->mouse_y >= (f32)b.y && cheese->mouse_y <= (f32)b.y + (f32)b.h;
    if (!inside)
      cheese->edit_id = 0;
  }
}

//
//
//

cheese_t cheese_default(arena_t *persistent) {
  cheese_t cheese = {0};

  cheese.scope_stack[0] = (cheese_style_scope_t){cheese_style_new(), null};
  cheese.scope_depth = 1;
  cheese.arena = persistent;
  cheese.prop_next = 1;
  cheese_style_register_core_props(&cheese);
  cheese.store = cheese_state_store_new(persistent);
  cheese.arrow_nav = true;
  cheese.cursor_last = -1;

  return cheese;
}

void cheese_input(cheese_t *cheese, cheese_input_t input) {
  if (!cheese)
    return;

  b32 changed = !cheese->input_seen ||
                memcmp(&cheese->last_input, &input, sizeof(input)) != 0;

  cheese->last_input = input;
  cheese->input_seen = true;

  if (changed)
    cheese->input_frame_needed = true;
}

void cheese_set_cursor_callback(cheese_t *cheese, cheese_cursor_callback_t cb,
                                void *userdata) {
  if (!cheese)
    return;

  cheese->cursor_callback = cb;
  cheese->cursor_callback_userdata = userdata;
}

void cheese_set_clipboard(cheese_t *cheese, cheese_clipboard_get_fn get,
                          cheese_clipboard_set_fn set, void *userdata) {
  if (!cheese)
    return;

  cheese->clipboard_get = get;
  cheese->clipboard_set = set;
  cheese->clipboard_userdata = userdata;
}

void cheese_cursor_request(cheese_t *cheese, cheese_cursor_t cursor) {
  if (!cheese || cursor < CHEESE_CURSOR_DEFAULT || cursor >= CHEESE_CURSOR_MAX)
    return;

  cheese->cursor_request = cursor;
  cheese->cursor_requested = true;
}

cheese_cursor_t cheese_cursor_current(const cheese_t *cheese) {
  if (!cheese || cheese->cursor_last < 0)
    return CHEESE_CURSOR_DEFAULT;

  return (cheese_cursor_t)cheese->cursor_last;
}

void cheese_begin(cheese_t *cheese, arena_t *frame_arena, cheese_font_t *font,
                  cheese_renderer_t *renderer, cheese_input_t input,
                  f32 delta_time) {
  cheese->renderer = renderer;
  cheese->frame_arena = frame_arena;

  cheese->raw_mouse_x = input.mouse_x;
  cheese->raw_mouse_y = input.mouse_y;

  cheese_rect_t prev = cheese->prev_overlay_rect;
  b32 over_overlay = prev.w > 0 && input.mouse_x >= prev.x &&
                     input.mouse_x <= prev.x + prev.w &&
                     input.mouse_y >= prev.y &&
                     input.mouse_y <= prev.y + prev.h;

  cheese->overlay_blocked = over_overlay;
  cheese->mouse_x = over_overlay ? -1.0e9f : input.mouse_x;
  cheese->mouse_y = over_overlay ? -1.0e9f : input.mouse_y;

  cheese->overlays = null;
  cheese->overlay_rect = (cheese_rect_t){0};

  cheese->text_runs = null;
  cheese->text_run_count = 0;
  cheese->text_run_cap = 0;

  cheese->scroll_x = max(-1.0f, min(1.0f, input.scroll_x));
  cheese->scroll_y = max(-1.0f, min(1.0f, input.scroll_y));
  cheese->mouse_prev_buttons = cheese->mouse_buttons;
  cheese->mouse_buttons = input.mouse_buttons;
  cheese->key_mods = input.key_mods;

  if (!(cheese->mouse_buttons & CHEESE_MOUSE_LEFT))
    cheese->active_id = 0;

  cheese->key_event_count =
      min(input.key_event_count, (u32)CHEESE_MAX_KEY_EVENTS);
  for (u32 i = 0; i < cheese->key_event_count; i++)
    cheese->key_events[i] = input.key_events[i];

  cheese->frame_count++;
  cheese->delta_time = delta_time;

  cheese_semantics_begin(cheese);

  cheese->scope_stack[0] = (cheese_style_scope_t){cheese_style_new(), null};
  cheese->scope_depth = 1;
  cheese->layout_stack[0] = cheese_layout_default();
  cheese->layout_stack[0].width = input.window_w;
  cheese->layout_stack[0].height = input.window_h;
  cheese->layout_stack[0].origin_x = 0.0f;
  cheese->layout_stack[0].origin_y = 0.0f;
  cheese->layout_stack_depth = 1;
  cheese->scroll_stack_depth = 0;
  cheese->class_styles = null;
  cheese->class_styles = null;
  cheese->theme = null;
  cheese->style_dirty = true;

  for (u32 i = 0; i < CHEESE_ROLE_MAX; i++)
    cheese->role_set[i] = false;

  cheese_theme_frame_begin(cheese);

  cheese->cursor_requested = false;

  renderer->flush_deferred(renderer->userdata);

  if (font && (!font->active_variant->atlas_texture_id ||
               font->active_variant->atlas_dirty)) {
    cheese_font_rebuild_atlas(font);
  }
}

void cheese_end(cheese_t *cheese) {
  cheese_overlay_flush(cheese);
  cheese_flush_draws(cheese);
  cheese_selection_update(cheese);

  cheese->prev_overlay_rect = cheese->overlay_rect;
  if (cheese_key_pressed(cheese, CHEESE_KEY_TAB))
    cheese_focus_move(cheese, (cheese->key_mods & CHEESE_MOD_SHIFT) ? -1 : 1);

  cheese_focus_arrows(cheese);
  cheese_edit_prune(cheese);
  cheese_semantics_snapshot_capture(cheese);

  cheese_cursor_t desired =
      cheese->cursor_requested ? cheese->cursor_request : CHEESE_CURSOR_DEFAULT;
  if ((i32)desired != cheese->cursor_last && cheese->cursor_callback) {
    cheese->cursor_last = (i32)desired;
    cheese->cursor_callback(cheese->cursor_callback_userdata, desired);
  }

  cheese->scroll_x = 0;
  cheese->scroll_y = 0;
}
