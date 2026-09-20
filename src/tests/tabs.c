/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/state.h>

#include <cheese/widgets/tabs.h>

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

static const cstr *g_labels[] = {"One", "Two", "Three"};

static i32 frame(cheese_t *cheese, arena_t *arena, cheese_state_t *selected,
                 cheese_input_t input) {
  cheese_begin(cheese, arena, null, &g_renderer, input, 0.016f);
  i32 value = cheese_tab_bar(cheese, null, (cheese_semantics_t){.key = "tabs"},
                             0.0f, 0.0f, 200.0f, 30.0f,
                             cheese_val_state(selected), g_labels, 3, null);
  cheese_end(cheese);
  return value;
}

//
//
//

int main(void) {
  arena_t *arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(arena);
  cheese_state_t *selected = cheese_state_i32(cheese.store, "tab", 0);

  cheese_input_t click = {
      .mouse_x = 100.0f,
      .mouse_y = 15.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };
  i32 value = frame(&cheese, arena, selected, click);
  assert(value == 1 && "clicking a tab selects it");
  assert(cheese_state_get_i32(selected) == 1 && "the selection is written");

  frame(&cheese, arena, selected, (cheese_input_t){0});

  cheese_input_t click2 = {
      .mouse_x = 166.0f,
      .mouse_y = 15.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };
  value = frame(&cheese, arena, selected, click2);
  assert(value == 2 && "the third tab selects");

  arena_free(arena);
  return 0;
}
