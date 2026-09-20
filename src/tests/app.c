/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/app.h>
#include <cheese/core/init.h>

/***********************************/

static u32 g_calls[4];

static void stub(void *userdata) { (void)userdata; }

//
//
//

static cheese_app_result_t on_start(cheese_t *cheese, void *userdata) {
  (void)cheese;
  (void)userdata;
  g_calls[0]++;
  return CHEESE_CONTINUE;
}

//
//
//

static cheese_app_result_t on_update(cheese_t *cheese, f32 dt, void *userdata) {
  (void)cheese;
  (void)dt;
  (void)userdata;
  g_calls[1]++;
  return CHEESE_CONTINUE;
}

//
//
//

static void on_render(cheese_t *cheese, void *userdata) {
  (void)cheese;
  (void)userdata;
  g_calls[2]++;
}

//
//
//

static void on_stop(cheese_t *cheese, void *userdata) {
  (void)cheese;
  (void)userdata;
  g_calls[3]++;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = cheese_default(frame);
  cheese_input_t input = {0};

  cheese_app_t app = {
      .userdata = null,
      .start = on_start,
      .update = on_update,
      .render = on_render,
      .stop = on_stop,
  };

  assert(cheese_app_start(&cheese, &app) == CHEESE_CONTINUE);
  assert(g_calls[0] == 1);

  cheese_begin(&cheese, frame, null, &g_renderer, input, 0.016f);
  assert(cheese_app_update(&cheese, &app, 0.016f) == CHEESE_CONTINUE);
  cheese_app_render(&cheese, &app);
  cheese_end(&cheese);

  assert(g_calls[1] == 1 && "update ran");
  assert(g_calls[2] == 1 && "render ran in-frame");

  cheese_app_stop(&cheese, &app);
  assert(g_calls[3] == 1);

  assert(cheese_app_start(null, &app) == CHEESE_CONTINUE &&
         "null cheese is a no-op");

  arena_free(frame);
  return 0;
}
