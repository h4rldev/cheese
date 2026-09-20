/***********************************/

#include <assert.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>

#include <cheese/widgets/slider.h>

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
  arena_t *persistent = arena_new(MiB(16), MiB(1));
  arena_t *frame = arena_new(MiB(16), MiB(1));
  arena_t *snapshot_arena = arena_new(MiB(16), MiB(1));

  cheese_t cheese = cheese_default(persistent);
  cheese_semantics_snapshot_enable(&cheese, snapshot_arena);

  cheese_state_t *value = cheese_state_f32(cheese.store, "v", 0.5f);

  cheese_begin(&cheese, frame, null, &g_renderer, (cheese_input_t){0}, 0.016f);
  cheese_slider(&cheese, null, (cheese_semantics_t){.key = "s"}, 0.0f, 0.0f,
                100.0f, 10.0f, cheese_val_state(value));
  cheese_end(&cheese);

  arena_clear(frame);

  cheese_semantics_snapshot_t view;
  cheese_semantics_snapshot_begin(&cheese, &view);

  assert(view.count >= 2 && "root plus the slider");
  assert(view.nodes[0].role == CHEESE_ROLE_ROOT && "root is index 0");

  const cheese_semantics_node_t *slider = null;
  for (u64 i = 0; i < view.count; i++)
    if (view.nodes[i].role == CHEESE_ROLE_SLIDER)
      slider = &view.nodes[i];

  assert(slider && "the slider node is in the snapshot");
  assert(slider->value && strcmp(slider->value, "50%") == 0 &&
         "value strings survive the frame arena");

  cheese_semantics_snapshot_end(&cheese);

  arena_free(frame);
  arena_free(snapshot_arena);
  arena_free(persistent);
  return 0;
}
