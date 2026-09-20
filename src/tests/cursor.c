/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>

#include <cheese/widgets/button.h>

/***********************************/

static void stub(void *userdata) { (void)userdata; }

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

static void stub_border(void *userdata, cheese_corners_t radius, f32 x, f32 y,
                        f32 w, f32 h, f32 thickness, u32 sides,
                        cheese_color_t color) {
  (void)userdata;
  (void)radius;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)thickness;
  (void)sides;
  (void)color;
}

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
    .draw_border = stub_border,
};

//
//
//

static cheese_cursor_t g_seq[16];
static u32 g_count;

static void cursor_cb(void *userdata, cheese_cursor_t cursor) {
  (void)userdata;
  if (g_count < 16)
    g_seq[g_count++] = cursor;
}

//
//
//

static void frame(cheese_t *cheese, arena_t *frame_arena, f32 mx, f32 my) {
  cheese_input_t input = {.mouse_x = mx, .mouse_y = my};

  cheese_begin(cheese, frame_arena, null, &g_renderer, input, 0.016f);
  cheese_button(cheese, null, (cheese_semantics_t){0}, 0.0f, 0.0f, 100.0f,
                40.0f, cheese_val_str(""), null, 0, 0, 0);
  cheese_end(cheese);
}

int main(void) {
  arena_t *frame_arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(frame_arena);
  cheese_set_cursor_callback(&cheese, cursor_cb, null);

  frame(&cheese, frame_arena, 500.0f, 500.0f); // nothing hovered
  frame(&cheese, frame_arena, 50.0f, 20.0f);   // over the button
  frame(&cheese, frame_arena, 50.0f, 20.0f);   // still over it: no re-emit
  frame(&cheese, frame_arena, 500.0f, 500.0f); // left it

  assert(g_count == 3 && "the callback fires once per change, not per frame");
  assert(g_seq[0] == CHEESE_CURSOR_DEFAULT);
  assert(g_seq[1] == CHEESE_CURSOR_POINTER);
  assert(g_seq[2] == CHEESE_CURSOR_DEFAULT);
  assert(cheese_cursor_current(&cheese) == CHEESE_CURSOR_DEFAULT);

  arena_free(frame_arena);
  return 0;
}
