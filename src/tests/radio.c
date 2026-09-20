/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/state.h>

#include <cheese/widgets/toggle.h>

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

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = cheese_default(frame);
  cheese_state_t *selected = cheese_state_i32(cheese.store, "selected", 0);

  cheese_input_t press = {
      .mouse_x = 5.0f,
      .mouse_y = 30.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };

  cheese_begin(&cheese, frame, null, &g_renderer, press, 0.016f);
  b32 first =
      cheese_radio(&cheese, null, (cheese_semantics_t){.key = "r0"}, 0.0f, 0.0f,
                   cheese_val_state(selected), 0, cheese_val_str("One"), null);
  b32 second = cheese_radio(&cheese, null, (cheese_semantics_t){.key = "r1"},
                            0.0f, 30.0f, cheese_val_state(selected), 1,
                            cheese_val_str("Two"), null);
  cheese_end(&cheese);

  assert(first && "the first radio starts selected");
  assert(second && "clicking the second selects it");
  assert(cheese_state_get_i32(selected) == 1 && "the group state is written");

  arena_free(frame);
  return 0;
}
