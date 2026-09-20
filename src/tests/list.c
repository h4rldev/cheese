/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/state.h>

#include <cheese/widgets/list.h>

/***********************************/

static i32 g_first;
static u32 g_calls;

static void stub(void *userdata) { (void)userdata; }

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

static void item(cheese_t *cheese, i32 index, cheese_rect_t row, b32 hovered,
                 void *userdata) {
  (void)cheese;
  (void)row;
  (void)hovered;
  (void)userdata;
  if (g_calls == 0)
    g_first = index;
  g_calls++;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .push_clip = stub_clip,
    .pop_clip = stub,
};

static i32 frame(cheese_t *cheese, arena_t *arena, cheese_state_t *scroll,
                 cheese_input_t input) {
  cheese_begin(cheese, arena, null, &g_renderer, input, 0.016f);
  i32 clicked = cheese_list(cheese, null, (cheese_semantics_t){.key = "list"},
                            0.0f, 0.0f, 100.0f, 60.0f, cheese_val_state(scroll),
                            100, 20.0f, item, null);
  cheese_end(cheese);
  return clicked;
}

//
//
//

int main(void) {
  arena_t *arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(arena);
  cheese_state_t *scroll = cheese_state_f32(cheese.store, "scroll", 0.0f);

  g_calls = 0;
  g_first = -1;
  frame(&cheese, arena, scroll, (cheese_input_t){0});
  assert(g_first == 0 && g_calls == 3 && "only the visible rows draw");
  assert(cheese_state_get_f32(scroll) == 0.0f);

  g_calls = 0;
  g_first = -1;
  cheese_input_t wheel = {
      .mouse_x = 50.0f, .mouse_y = 30.0f, .scroll_y = -1.0f};
  frame(&cheese, arena, scroll, wheel);
  assert(cheese_state_get_f32(scroll) == 60.0f && "the wheel scrolls");
  assert(g_first == 3 && "virtualization follows the offset");

  cheese_input_t press = {
      .mouse_x = 50.0f,
      .mouse_y = 30.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };
  i32 clicked = frame(&cheese, arena, scroll, press);
  assert(clicked == 4 && "clicking a row returns its index");

  cheese_state_set_f32(scroll, 1900.0f);
  cheese_input_t fling = {
      .mouse_x = 50.0f, .mouse_y = 30.0f, .scroll_y = -1.0f};
  frame(&cheese, arena, scroll, fling);
  assert(cheese_state_get_f32(scroll) == 1940.0f && "scroll clamps to content");

  arena_free(arena);
  return 0;
}
