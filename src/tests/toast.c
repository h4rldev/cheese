/***********************************/

#include <assert.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/darray.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/semantics.h>

#include <cheese/patterns/toast.h>

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

int main(void) {
  arena_t *arena = arena_new(MiB(16), MiB(1));
  cheese_toast_stack_t stack;
  cheese_toast_stack_init(&stack, arena, 2);

  cheese_toast_push(&stack, "one", 1.0f);
  cheese_toast_push(&stack, "two", 1.0f);

  char buffer[16];
  strcpy(buffer, "three");
  cheese_toast_push(&stack, buffer, 1.0f);
  buffer[0] = 'X';

  assert(stack.count == 2 && "capacity drops the oldest");
  assert(strcmp(stack.items[0].text, "two") == 0 && "the oldest was dropped");
  assert(strcmp(stack.items[1].text, "three") == 0 && "text is copied on push");

  cheese_toast_tick(&stack, 0.5f);
  assert(stack.count == 2 && "toasts stay live until their lifetime ends");
  cheese_toast_tick(&stack, 0.6f);
  assert(stack.count == 0 && "expired toasts are dropped");

  cheese_toast_push(&stack, "hello", 2.0f);

  cheese_t cheese = cheese_default(arena);
  cheese_input_t input = {.window_w = 800.0f, .window_h = 600.0f};
  cheese_begin(&cheese, arena, null, &g_renderer, input, 0.016f);
  cheese_toast_draw(&cheese, null, &stack, null);
  assert(da_len(cheese.overlays) == 1 && "one overlay per live toast");
  cheese_end(&cheese);

  b32 emitted = false;
  for (u64 i = 0; i < cheese_semantics_count(&cheese); i++) {
    cheese_semantics_node_t *n = cheese_semantics_at(&cheese, i);
    if (n->role == CHEESE_ROLE_LABEL && n->name &&
        strcmp(n->name, "hello") == 0)
      emitted = true;
  }
  assert(emitted && "toasts emit a LABEL semantics node");

  arena_free(arena);
  return 0;
}
