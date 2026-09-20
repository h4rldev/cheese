/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/semantics.h>

/***********************************/

static void stub_flush(void *userdata) { (void)userdata; }

//
//
//

static void draw_two_buttons(cheese_t *cheese) {
  cheese_semantics_emit(cheese, (cheese_semantics_t){.key = "a"},
                        CHEESE_ROLE_BUTTON, "A", null, 0,
                        (cheese_rect_t){0, 0, 10, 10});
  cheese_semantics_emit(cheese, (cheese_semantics_t){.key = "b"},
                        CHEESE_ROLE_BUTTON, "B", null, 0,
                        (cheese_rect_t){0, 0, 10, 10});
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub_flush,
    .flush_draws = stub_flush,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = {0};

  cheese_input_t tab = {0};
  tab.key_event_count = 1;
  tab.key_events[0].key = CHEESE_KEY_TAB;

  cheese_begin(&cheese, frame, null, &g_renderer, tab, 0.016f);
  draw_two_buttons(&cheese);
  u64 a_id = cheese_semantics_find(&cheese, "a")->id;
  u64 b_id = cheese_semantics_find(&cheese, "b")->id;
  cheese_end(&cheese);
  assert(cheese.focus_id == a_id && "first Tab focuses the first node");

  cheese_begin(&cheese, frame, null, &g_renderer, tab, 0.016f);
  draw_two_buttons(&cheese);
  assert((cheese_semantics_find(&cheese, "a")->state & CHEESE_STATE_FOCUSED) &&
         "the focused node reports CHEESE_STATE_FOCUSED");
  cheese_end(&cheese);
  assert(cheese.focus_id == b_id);

  cheese_begin(&cheese, frame, null, &g_renderer, tab, 0.016f);
  draw_two_buttons(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == a_id && "focus wraps forward");

  cheese_input_t back = tab;
  back.key_mods = CHEESE_MOD_SHIFT;
  cheese_begin(&cheese, frame, null, &g_renderer, back, 0.016f);
  draw_two_buttons(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == b_id && "Shift+Tab wraps backward");

  arena_free(frame);
  return 0;
}
