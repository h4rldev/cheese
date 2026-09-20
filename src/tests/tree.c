/***********************************/

#include <assert.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/semantics.h>

#include <cheese/widgets/progress.h>

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
  cheese_input_t input = {0};

  cheese_begin(&cheese, frame, null, &g_renderer, input, 0.016f);

  cheese_semantics_begin_node(&cheese, (cheese_semantics_t){.key = "panel"},
                              CHEESE_ROLE_CONTAINER, "Panel",
                              (cheese_rect_t){0, 0, 100, 100});
  cheese_semantics_emit(&cheese, (cheese_semantics_t){.key = "a"},
                        CHEESE_ROLE_BUTTON, "A", null, 0,
                        (cheese_rect_t){0, 0, 10, 10});
  cheese_semantics_emit(&cheese, (cheese_semantics_t){.key = "b"},
                        CHEESE_ROLE_BUTTON, "B", null, 0,
                        (cheese_rect_t){0, 0, 10, 10});
  cheese_semantics_end_node(&cheese);

  cheese_semantics_node_t *panel = cheese_semantics_find(&cheese, "panel");
  assert(panel != null && panel->role == CHEESE_ROLE_CONTAINER);

  cheese_semantics_node_t *a = cheese_semantics_first_child(&cheese, panel);
  assert(a != null && strcmp(a->name, "A") == 0);

  cheese_semantics_node_t *b = cheese_semantics_next_sibling(&cheese, a);
  assert(b != null && strcmp(b->name, "B") == 0);
  assert(cheese_semantics_next_sibling(&cheese, b) == null);

  assert(strcmp(cheese_semantics_role_name(panel->role), "container") == 0);
  assert(strcmp(cheese_semantics_role_name(a->role), "button") == 0);

  u64 peek = cheese_semantics_peek_id(
      &cheese, (cheese_semantics_t){.name = "X"}, CHEESE_ROLE_LABEL);
  i32 index = cheese_semantics_emit(&cheese, (cheese_semantics_t){.name = "X"},
                                    CHEESE_ROLE_LABEL, null, null, 0,
                                    (cheese_rect_t){0, 0, 5, 5});
  assert(cheese_semantics_at(&cheese, (u64)index)->id == peek &&
         "peek_id must match the following emit's id");

  assert(strcmp(cheese_semantics_format(&cheese, "%d%%", 50), "50%") == 0);

  cheese_progress_bar(&cheese, null, (cheese_semantics_t){.key = "bar"}, 0, 0,
                      10, 4, cheese_val_f32(0.5f));
  cheese_semantics_node_t *bar = cheese_semantics_find(&cheese, "bar");
  assert(bar != null && bar->value != null);
  assert(strcmp(bar->value, "50%") == 0 && "the bar reports its value");

  cheese_end(&cheese);
  arena_free(frame);
  return 0;
}
