/***********************************/

#include <htils/basictypes.h>
#include <htils/darray.h>

#include <cheese/types.h>

#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/overlay.h>

#include <cheese/render/draw.h>

/***********************************/

typedef struct {
  cheese_overlay_draw_fn draw;
  void *userdata;
} cheese_modal_ctx_t;

//
//
//

static void cheese_modal_scrim(cheese_t *cheese, void *userdata) {
  cheese_modal_ctx_t *ctx = userdata;

  f32 win_w = cheese->layout_stack[0].width;
  f32 win_h = cheese->layout_stack[0].height;

  cheese_draw_rect(cheese, (cheese_corners_t){0, 0, 0, 0}, 0.0f, 0.0f, win_w,
                   win_h, 0x00000080);

  ctx->draw(cheese, ctx->userdata);
}

//
//
//

void cheese_overlay(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                    cheese_overlay_draw_fn draw, void *userdata) {
  if (!cheese || !draw)
    return;

  if (!cheese->overlays)
    da_new(cheese->frame_arena, cheese->overlays, 4);

  cheese_overlay_entry_t entry = {x, y, w, h, draw, userdata};
  da_append(cheese->frame_arena, cheese->overlays, entry);

  cheese->overlay_rect = (cheese_rect_t){(i32)x, (i32)y, (u32)w, (u32)h};
}

void cheese_overlay_modal(cheese_t *cheese, f32 x, f32 y, f32 w, f32 h,
                          cheese_overlay_draw_fn draw, void *userdata) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;

  if (!cheese || !draw)
    return;

  cheese_modal_ctx_t *ctx =
      arena_alloc_zeroed(cheese->frame_arena, cheese_modal_ctx_t, 1);
  ctx->draw = draw;
  ctx->userdata = userdata;

  cheese_overlay(cheese, 0.0f, 0.0f, cheese->layout_stack[0].width,
                 cheese->layout_stack[0].height, cheese_modal_scrim, ctx);
}

void cheese_popup_open(cheese_t *cheese, cheese_popup_t *popup) {
  if (!cheese || !popup)
    return;

  if (!popup->open)
    popup->opened_frame = cheese->frame_count;
  popup->open = true;
}

void cheese_popup_close(cheese_popup_t *popup) {
  if (popup)
    popup->open = false;
}

b32 cheese_popup_dismissed(cheese_t *cheese, cheese_popup_t *popup,
                           cheese_rect_t bounds) {
  if (!cheese || !popup || !popup->open)
    return false;

  f32 mx = cheese->mouse_x;
  f32 my = cheese->mouse_y;
  b32 inside = mx >= (f32)bounds.x && mx <= (f32)(bounds.x + bounds.w) &&
               my >= (f32)bounds.y && my <= (f32)(bounds.y + bounds.h);
  b32 clicked =
      (cheese->mouse_buttons & ~cheese->mouse_prev_buttons) & CHEESE_MOUSE_LEFT;
  b32 opened_now = popup->opened_frame == cheese->frame_count;

  b32 dismiss = cheese_key_pressed(cheese, CHEESE_KEY_ESCAPE) ||
                (clicked && !inside && !opened_now);

  if (dismiss)
    popup->open = false;

  return dismiss;
}

void cheese_overlay_flush(cheese_t *cheese) {
  if (!cheese || !cheese->overlays)
    return;

  f32 muted_x = cheese->mouse_x;
  f32 muted_y = cheese->mouse_y;
  cheese->mouse_x = cheese->raw_mouse_x;
  cheese->mouse_y = cheese->raw_mouse_y;

  for (u64 i = 0; i < da_len(cheese->overlays); i++) {
    cheese_overlay_entry_t *entry = &cheese->overlays[i];

    b32 clip = cheese->renderer && cheese->renderer->push_clip != null;
    if (clip)
      cheese_push_clip(cheese, entry->x, entry->y, entry->w, entry->h);

    cheese_layout_t child = *cheese_current_layout(cheese);
    child.x = entry->x;
    child.y = entry->y;
    child.width = entry->w;
    child.height = entry->h;
    child.origin_x = entry->x;
    child.origin_y = entry->y;
    child.line_cross = 0.0f;
    child.container_w = entry->w;
    child.container_h = entry->h;
    cheese_push_layout(cheese, child);

    entry->draw(cheese, entry->userdata);

    cheese_pop_layout(cheese);
    if (clip)
      cheese_pop_clip(cheese);
  }

  cheese->mouse_x = muted_x;
  cheese->mouse_y = muted_y;
}
