/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/overlay.h>

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
static b32 g_dismissed;

static void popup_draw(cheese_t *cheese, void *userdata) {
  (void)userdata;
  g_dismissed =
      cheese_popup_dismissed(cheese, &g_popup, (cheese_rect_t){0, 0, 100, 80});
}

//
//
//

static void frame(cheese_t *cheese, arena_t *arena, cheese_input_t input,
                  b32 open_now) {
  cheese_begin(cheese, arena, null, &g_renderer, input, 0.016f);
  if (open_now)
    cheese_popup_open(cheese, &g_popup);
  if (g_popup.open)
    cheese_overlay(cheese, 0.0f, 0.0f, 100.0f, 80.0f, popup_draw, null);
  cheese_end(cheese);
}

//
//
//

int main(void) {
  arena_t *arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(arena);

  cheese_input_t base = {0};
  cheese_input_t open_click = {
      .mouse_x = 500.0f,
      .mouse_y = 500.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };

  frame(&cheese, arena, open_click, true);
  assert(g_popup.open && "stays open on its opening click");

  frame(&cheese, arena, base, false);

  g_dismissed = false;
  cheese_input_t inside = {
      .mouse_x = 50.0f,
      .mouse_y = 40.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };
  frame(&cheese, arena, inside, false);
  assert(g_popup.open && "a click inside is kept");
  assert(!g_dismissed && "an inside click is not a dismissal");

  frame(&cheese, arena, base, false);

  cheese_input_t outside = {
      .mouse_x = 500.0f,
      .mouse_y = 500.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };
  frame(&cheese, arena, outside, false);
  assert(!g_popup.open && "a click outside dismisses");
  assert(g_dismissed && "and reports the dismissal");

  frame(&cheese, arena, base, false);

  frame(&cheese, arena, base, true);
  assert(g_popup.open && "reopens");

  cheese_input_t escape = {0};
  escape.key_event_count = 1;
  escape.key_events[0] = (cheese_key_event_t){.key = CHEESE_KEY_ESCAPE};
  frame(&cheese, arena, escape, false);
  assert(!g_popup.open && "escape dismisses");

  arena_free(arena);
  return 0;
}
