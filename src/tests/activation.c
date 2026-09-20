/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>

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

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
    .draw_line = stub_line,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = {0};

  cheese_input_t tab = {0};
  tab.key_event_count = 1;
  tab.key_events[0].key = CHEESE_KEY_TAB;

  cheese_begin(&cheese, frame, null, &g_renderer, tab, 0.016f);
  cheese_button(&cheese, null, (cheese_semantics_t){.key = "a"}, 100, 100, 10,
                10, cheese_val_str("A"), null, 0, 0, 0);
  cheese_end(&cheese);

  cheese_input_t enter = {0};
  enter.key_event_count = 1;
  enter.key_events[0].key = CHEESE_KEY_ENTER;

  cheese_begin(&cheese, frame, null, &g_renderer, enter, 0.016f);
  b32 clicked =
      cheese_button(&cheese, null, (cheese_semantics_t){.key = "a"}, 100, 100,
                    10, 10, cheese_val_str("A"), null, 0, 0, 0);
  assert(clicked && "Enter activates the focused button");
  cheese_end(&cheese);

  cheese_begin(&cheese, frame, null, &g_renderer, enter, 0.016f);
  clicked = cheese_button(&cheese, null, (cheese_semantics_t){.key = "b"}, 100,
                          100, 10, 10, cheese_val_str("B"), null, 0, 0, 0);
  assert(!clicked && "an unfocused button is not activated by Enter");
  cheese_end(&cheese);

  arena_free(frame);
  return 0;
}
