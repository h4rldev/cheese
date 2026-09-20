/***********************************/

#include <assert.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/state.h>

/***********************************/

static void stub_flush(void *userdata) { (void)userdata; }

//
//
//

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = {0};
  cheese.store = cheese_state_store_new(frame);
  cheese_renderer_t renderer = {
      .flush_deferred = stub_flush,
      .flush_draws = stub_flush,
  };

  cheese_input_t input = {0};
  input.key_event_count = 2;
  input.key_events[0].key = CHEESE_KEY_TAB;
  input.key_events[0].mods = CHEESE_MOD_SHIFT;
  input.key_events[1].key = CHEESE_KEY_ENTER;
  input.key_events[1].repeat = true;
  input.key_mods = CHEESE_MOD_CTRL;

  cheese_begin(&cheese, frame, null, &renderer, input, 0.016f);

  cheese_clear_frame_needed(&cheese);
  cheese_input_t a = {.mouse_x = 1.0f};
  cheese_input(&cheese, a);
  assert(cheese_needs_frame(&cheese) && "the first input wakes a frame");

  cheese_clear_frame_needed(&cheese);
  cheese_input(&cheese, a);
  assert(!cheese_needs_frame(&cheese) && "unchanged input is silent");

  cheese_input_t b = {.mouse_x = 2.0f};
  cheese_input(&cheese, b);
  assert(cheese_needs_frame(&cheese) && "motion wakes a frame");

  cheese_capture(&cheese, 42);
  assert(cheese_captured(&cheese, 42) && "capture is claimed");
  assert(!cheese_captured(&cheese, 7) && "capture is exclusive");

  cheese_input_t press = {.mouse_buttons = CHEESE_MOUSE_LEFT};
  cheese_begin(&cheese, frame, null, &renderer, press, 0.016f);
  cheese_capture(&cheese, 42);
  assert(cheese_captured(&cheese, 42) && "held while the button is down");

  cheese_begin(&cheese, frame, null, &renderer, input, 0.016f);
  assert(!cheese_captured(&cheese, 42) && "released when the button comes up");

  assert(cheese_key_pressed(&cheese, CHEESE_KEY_TAB));
  assert(!cheese_key_pressed(&cheese, CHEESE_KEY_ENTER) &&
         "a repeat must not count as a fresh press");
  assert(cheese_key_event_count(&cheese) == 2 && "the event count is reported");
  assert(cheese_key_event_at(&cheese, 1) != null);
  assert(cheese_key_event_at(&cheese, 2) == null);
  assert(cheese_key_event_at(&cheese, 0)->mods == CHEESE_MOD_SHIFT &&
         "per-event modifiers carry through");
  assert(cheese.key_mods == CHEESE_MOD_CTRL &&
         "the frame's modifier mask carries through");

  assert(cheese_value_b32(cheese_val_b32(true)) == true &&
         "a literal b32 value reads back");
  assert(cheese_value_i32(cheese_val_i32(-7)) == -7 &&
         "a literal i32 value reads back");
  assert(cheese_value_u32(cheese_val_u32(7u)) == 7u &&
         "a literal u32 value reads back");
  assert(cheese_value_f64(cheese_val_f64(1.5)) == 1.5 &&
         "a literal f64 value reads back");
  assert(strcmp(cheese_value_str(cheese_val_str("hi")), "hi") == 0 &&
         "a literal string value reads back");

  cheese_state_t *n = cheese_state_u32(cheese.store, "count", 0);
  cheese_value_set_u32(cheese_val_state(n), 9u);
  assert(cheese_state_get_u32(n) == 9u && "a u32 binding writes its state");

  cheese_state_t *d = cheese_state_f64(cheese.store, "ratio", 0.0);
  cheese_value_set_f64(cheese_val_state(d), 2.5);
  assert(cheese_state_get_f64(d) == 2.5 && "an f64 binding writes its state");

  cheese_end(&cheese);

  arena_free(frame);
  return 0;
}
