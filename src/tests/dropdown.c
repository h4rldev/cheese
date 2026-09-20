/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/state.h>

#include <cheese/widgets/dropdown.h>

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

static const cstr *g_items[] = {"One", "Two", "Three"};

static i32 frame(cheese_t *cheese, arena_t *frame_arena,
                 cheese_state_t *selected, cheese_popup_t *state,
                 cheese_input_t input) {
  cheese_begin(cheese, frame_arena, null, &g_renderer, input, 0.016f);
  i32 value = cheese_dropdown(
      cheese, null, (cheese_semantics_t){.key = "dd"}, 0.0f, 0.0f, 100.0f,
      20.0f, cheese_val_state(selected), g_items, 3, state, null);
  cheese_end(cheese);
  return value;
}

int main(void) {
  arena_t *frame_arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(frame_arena);
  cheese_state_t *selected = cheese_state_i32(cheese.store, "sel", 0);
  cheese_popup_t state = {0};

  cheese_input_t press_trigger = {
      .mouse_x = 5.0f,
      .mouse_y = 10.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };
  frame(&cheese, frame_arena, selected, &state, press_trigger);
  assert(state.open && "a click on the trigger opens the dropdown");

  cheese_input_t release = {.mouse_x = 5.0f, .mouse_y = 10.0f};
  frame(&cheese, frame_arena, selected, &state, release);
  assert(state.open && "releasing keeps it open");

  cheese_input_t press_item = {
      .mouse_x = 50.0f,
      .mouse_y = 84.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };
  frame(&cheese, frame_arena, selected, &state, press_item);
  assert(!state.open && "clicking an item closes the dropdown");
  assert(cheese_state_get_i32(selected) == 2 && "the item writes the state");

  frame(&cheese, frame_arena, selected, &state, release);
  frame(&cheese, frame_arena, selected, &state, press_trigger);
  assert(state.open && "reopens");
  frame(&cheese, frame_arena, selected, &state, release);

  cheese_input_t escape = {.mouse_x = 5.0f, .mouse_y = 10.0f};
  escape.key_event_count = 1;
  escape.key_events[0] = (cheese_key_event_t){.key = CHEESE_KEY_ESCAPE};
  frame(&cheese, frame_arena, selected, &state, escape);
  assert(!state.open && "escape closes the dropdown");

  arena_free(frame_arena);
  return 0;
}
