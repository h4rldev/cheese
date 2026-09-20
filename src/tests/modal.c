/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/overlay.h>

#include <cheese/render/draw.h>

#include <cheese/widgets/button.h>

/***********************************/

static void stub(void *userdata) { (void)userdata; }

//
//
//

static void stub_rect(void *userdata, cheese_corners_t radius, f32 x, f32 y,
                      f32 w, f32 h, cheese_color_t color) {
  (void)userdata;
  (void)radius;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)color;
}

//
//
//

static void stub_line(void *userdata, f32 x1, f32 y1, f32 x2, f32 y2,
                      f32 thickness, cheese_color_t color) {
  (void)userdata;
  (void)x1;
  (void)y1;
  (void)x2;
  (void)y2;
  (void)thickness;
  (void)color;
}

//
//
//

static void stub_clip(void *userdata, f32 x, f32 y, f32 w, f32 h) {
  (void)userdata;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
    .draw_line = stub_line,
    .push_clip = stub_clip,
    .pop_clip = stub,
};

static cheese_popup_t g_popup;
static b32 g_behind;
static b32 g_ok;

static void modal_draw(cheese_t *cheese, void *userdata) {
  (void)userdata;

  f32 x = 100.0f, y = 100.0f, w = 200.0f, h = 100.0f;
  cheese_draw_rect(cheese, (cheese_corners_t){0, 0, 0, 0}, x, y, w, h,
                   0xFFFFFFFF);

  if (cheese_button(cheese, null, (cheese_semantics_t){.key = "ok"}, x + 10.0f,
                    y + 10.0f, 60.0f, 30.0f, cheese_val_str("OK"), null, 0, 0,
                    0))
    g_ok = true;

  cheese_popup_dismissed(cheese, &g_popup,
                         (cheese_rect_t){(i32)x, (i32)y, (u32)w, (u32)h});
}

//
//
//

static void frame(cheese_t *cheese, arena_t *arena, cheese_input_t input,
                  b32 open_now) {
  cheese_begin(cheese, arena, null, &g_renderer, input, 0.016f);

  if (cheese_button(cheese, null, (cheese_semantics_t){.key = "behind"}, 10.0f,
                    10.0f, 50.0f, 50.0f, cheese_val_str("B"), null, 0, 0, 0))
    g_behind = true;

  if (open_now)
    cheese_popup_open(cheese, &g_popup);
  if (g_popup.open)
    cheese_overlay_modal(cheese, 100.0f, 100.0f, 200.0f, 100.0f, modal_draw,
                         null);

  cheese_end(cheese);
}

//
//
//

int main(void) {
  arena_t *arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(arena);

  cheese_input_t base = {.window_w = 800.0f, .window_h = 600.0f};

  cheese_input_t open_click = base;
  open_click.mouse_x = 700.0f;
  open_click.mouse_y = 500.0f;
  open_click.mouse_buttons = CHEESE_MOUSE_LEFT;
  frame(&cheese, arena, open_click, true);
  assert(g_popup.open && "stays open on its opening click");
  assert(!g_behind && "the opening click misses the tree beneath");

  frame(&cheese, arena, base, false);

  cheese_input_t behind_click = base;
  behind_click.mouse_x = 10.0f;
  behind_click.mouse_y = 10.0f;
  behind_click.mouse_buttons = CHEESE_MOUSE_LEFT;
  frame(&cheese, arena, behind_click, false);
  assert(!g_behind && "the modal blocks the tree beneath");
  assert(!g_popup.open && "a click on the scrim dismisses");

  frame(&cheese, arena, base, false);

  frame(&cheese, arena, base, true);
  frame(&cheese, arena, base, false);

  cheese_input_t ok = base;
  ok.mouse_x = 140.0f;
  ok.mouse_y = 125.0f;
  ok.mouse_buttons = CHEESE_MOUSE_LEFT;
  frame(&cheese, arena, ok, false);
  assert(g_ok && "modal content receives input");
  assert(!g_behind && "behind stays blocked");

  arena_free(arena);
  return 0;
}
