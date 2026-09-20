/***********************************/

#include <assert.h>
#include <math.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>

#include <cheese/widgets/scrollbar.h>

/***********************************/

static cheese_renderer_t g_renderer;

//
//
//

static b32 approx(f32 a, f32 b) { return fabsf(a - b) < 0.1f; }

//
//
//

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

static u64 find_scrollbar_id(cheese_t *cheese) {
  for (u64 i = 0; i < cheese_semantics_count(cheese); i++) {
    cheese_semantics_node_t *n = cheese_semantics_at(cheese, i);
    if (n->role == CHEESE_ROLE_SCROLLBAR)
      return n->id;
  }

  return 0;
}

int main(void) {
  g_renderer = (cheese_renderer_t){
      .flush_deferred = stub,
      .flush_draws = stub,
      .draw_rect = stub_rect,
  };

  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = cheese_default(frame);
  cheese_state_t *scroll = cheese_state_f32(cheese.store, "scroll", 0.0f);
  cheese_semantics_t sem = {.key = "sb"};

  cheese_input_t press = {
      .mouse_x = 5.0f,
      .mouse_y = 50.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };

  cheese_begin(&cheese, frame, null, &g_renderer, press, 0.016f);
  f32 v = cheese_scrollbar(&cheese, null, sem, 0.0f, 0.0f, 10.0f, 100.0f,
                           cheese_val_state(scroll), 100.0f, 400.0f,
                           CHEESE_SCROLLBAR_VERTICAL);
  cheese_end(&cheese);
  assert(approx(v, 150.0f) && "a press on the track jumps the thumb");
  assert(approx(cheese_state_get_f32(scroll), 150.0f) &&
         "and writes the state");

  u64 id = find_scrollbar_id(&cheese);
  assert(id != 0 && "the scrollbar emits a semantics node");

  cheese_input_t drag = {
      .mouse_x = 5.0f,
      .mouse_y = 75.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };

  cheese_begin(&cheese, frame, null, &g_renderer, drag, 0.016f);
  v = cheese_scrollbar(&cheese, null, sem, 0.0f, 0.0f, 10.0f, 100.0f,
                       cheese_val_state(scroll), 100.0f, 400.0f,
                       CHEESE_SCROLLBAR_VERTICAL);
  cheese_end(&cheese);
  assert(approx(v, 250.0f) && "drag tracks the pointer");

  cheese_input_t released = {.mouse_x = 500.0f, .mouse_y = 500.0f};

  cheese_begin(&cheese, frame, null, &g_renderer, released, 0.016f);
  v = cheese_scrollbar(&cheese, null, sem, 0.0f, 0.0f, 10.0f, 100.0f,
                       cheese_val_state(scroll), 100.0f, 400.0f,
                       CHEESE_SCROLLBAR_VERTICAL);
  cheese_end(&cheese);
  assert(approx(v, 250.0f) && "no drag without the button");

  cheese.focus_id = id;

  cheese_input_t enter = {.mouse_x = 500.0f, .mouse_y = 500.0f};
  enter.key_event_count = 1;
  enter.key_events[0] = (cheese_key_event_t){.key = CHEESE_KEY_ENTER};

  cheese_begin(&cheese, frame, null, &g_renderer, enter, 0.016f);
  cheese_scrollbar(&cheese, null, sem, 0.0f, 0.0f, 10.0f, 100.0f,
                   cheese_val_state(scroll), 100.0f, 400.0f,
                   CHEESE_SCROLLBAR_VERTICAL);
  cheese_end(&cheese);
  assert(cheese.edit_id == id && "enter starts editing the scrollbar");

  cheese_input_t up = {.mouse_x = 500.0f, .mouse_y = 500.0f};
  up.key_event_count = 1;
  up.key_events[0] = (cheese_key_event_t){.key = CHEESE_KEY_UP};

  cheese_begin(&cheese, frame, null, &g_renderer, up, 0.016f);
  v = cheese_scrollbar(&cheese, null, sem, 0.0f, 0.0f, 10.0f, 100.0f,
                       cheese_val_state(scroll), 100.0f, 400.0f,
                       CHEESE_SCROLLBAR_VERTICAL);
  cheese_end(&cheese);
  assert(approx(v, 240.0f) && "Up steps by ~10% of the viewport");

  cheese_input_t page_down = {.mouse_x = 500.0f, .mouse_y = 500.0f};
  page_down.key_event_count = 1;
  page_down.key_events[0] = (cheese_key_event_t){.key = CHEESE_KEY_PAGE_DOWN};

  cheese_begin(&cheese, frame, null, &g_renderer, page_down, 0.016f);
  v = cheese_scrollbar(&cheese, null, sem, 0.0f, 0.0f, 10.0f, 100.0f,
                       cheese_val_state(scroll), 100.0f, 400.0f,
                       CHEESE_SCROLLBAR_VERTICAL);
  cheese_end(&cheese);
  assert(approx(v, 300.0f) && "PageDown clamps at the end");

  cheese_input_t over_end = {
      .mouse_x = 5.0f,
      .mouse_y = 90.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };

  cheese_begin(&cheese, frame, null, &g_renderer, over_end, 0.016f);
  v = cheese_scrollbar(&cheese, null, sem, 0.0f, 0.0f, 10.0f, 100.0f,
                       cheese_val_state(scroll), 100.0f, 400.0f,
                       CHEESE_SCROLLBAR_VERTICAL);
  cheese_end(&cheese);
  assert(approx(v, 300.0f) && "a press near the end clamps to the range");

  cheese_begin(&cheese, frame, null, &g_renderer, released, 0.016f);
  cheese_scrollbar(&cheese, null, sem, 0.0f, 0.0f, 10.0f, 100.0f,
                   cheese_val_state(scroll), 100.0f, 400.0f,
                   CHEESE_SCROLLBAR_VERTICAL);
  cheese_end(&cheese);

  cheese_input_t h_press = {
      .mouse_x = 50.0f,
      .mouse_y = 5.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };

  cheese_begin(&cheese, frame, null, &g_renderer, h_press, 0.016f);
  v = cheese_scrollbar(&cheese, null, sem, 0.0f, 0.0f, 100.0f, 10.0f,
                       cheese_val_state(scroll), 100.0f, 400.0f,
                       CHEESE_SCROLLBAR_HORIZONTAL);
  cheese_end(&cheese);
  assert(approx(v, 150.0f) && "the horizontal axis maps x");

  arena_free(frame);
  return 0;
}
