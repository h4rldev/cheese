/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>

#include <cheese/render/draw.h>

#include <cheese/style.h>

/***********************************/

static i32 g_rect_calls;
static i32 g_gradient_calls;

static cheese_color_t g_rect_color;
static cheese_gradient_t g_gradient;

static void rect_rect(void *userdata, cheese_corners_t radius, f32 x, f32 y,
                      f32 w, f32 h, cheese_color_t color) {
  (void)userdata;
  (void)radius;
  (void)x;
  (void)y;
  (void)w;
  (void)h;

  g_rect_calls++;
  g_rect_color = color;
}

//
//
//

static void gradient_rect(void *userdata, cheese_corners_t radius, f32 x, f32 y,
                          f32 w, f32 h, cheese_gradient_t colors) {
  (void)userdata;
  (void)radius;
  (void)x;
  (void)y;
  (void)w;
  (void)h;

  g_gradient_calls++;
  g_gradient = colors;
}

//
//
//

static void stub(void *userdata) { (void)userdata; }

//
//
//

static cheese_renderer_t g_flat_renderer = {
    .draw_rect = rect_rect,
    .flush_deferred = stub,
    .flush_draws = stub,
};

//
//
//

static cheese_renderer_t g_gradient_renderer = {
    .draw_rect = rect_rect,
    .draw_rect_gradient = gradient_rect,
    .flush_draws = stub,
    .flush_deferred = stub,
};

//
//
//

static void reset_counts(void) {
  g_rect_calls = 0;
  g_gradient_calls = 0;
  g_rect_color = 0;
  g_gradient = (cheese_gradient_t){0};
}

//
//
//

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_gradient_t g = {
      .top_left = cheese_color_rgb(10, 0, 0),
      .top_right = cheese_color_rgb(20, 0, 0),
      .bottom_left = cheese_color_rgb(30, 0, 0),
      .bottom_right = cheese_color_rgb(40, 0, 0),
  };

  cheese_t cheese = cheese_default(frame);
  cheese_input_t input = {0};

  cheese_begin(&cheese, frame, null, &g_gradient_renderer, input, 0.016f);

  u32 prop = cheese.core_props.bg_gradient;
  assert(prop != 0 && "core bg/gradient prip is registered");

  cheese_style_t empty = cheese_style_new();
  assert(cheese_style_prop_get_gradient(&empty, prop, (cheese_gradient_t){0})
             .top_left == 0);

  cheese_style_prop_set_gradient(&cheese, &empty, prop, g);
  cheese_gradient_t got =
      cheese_style_prop_get_gradient(&empty, prop, (cheese_gradient_t){0});

  assert(got.top_left == g.top_left);
  assert(got.top_right == g.top_right);
  assert(got.bottom_left == g.bottom_left);
  assert(got.bottom_right == g.bottom_right);

  cheese_style_t absent = cheese_style_new();
  assert(cheese_style_prop_get_gradient(&absent, prop,
                                        (cheese_gradient_t){.top_left = 0xDEAD})
             .top_left == 0xDEAD);

  reset_counts();
  cheese_draw_rect_gradient(&cheese, (cheese_corners_t){0}, 0, 0, 10, 10, g);
  assert(g_gradient_calls == 1 && g_rect_calls == 0);
  assert(g_gradient.top_left == g.top_left);
  assert(g_gradient.bottom_right == g.bottom_right);

  reset_counts();
  cheese_begin(&cheese, frame, null, &g_flat_renderer, input, 0.016f);
  cheese_draw_rect_gradient(&cheese, (cheese_corners_t){0}, 0, 0, 10, 10, g);
  assert(g_gradient_calls == 0 && g_rect_calls == 1);
  assert(g_rect_color == g.top_left && "fallback uses the top-left colour");

  reset_counts();
  cheese_begin(&cheese, frame, null, &g_gradient_renderer, input, 0.016f);
  cheese_style_t bg = cheese_style_new();
  cheese_style_prop_set_gradient(&cheese, &bg, prop, g);
  cheese_draw_bg(&cheese, &bg, 0, 0, 10, 10, cheese_color_rgb(1, 2, 3), 0,
                 1.0f);
  assert(g_gradient_calls == 1 && g_rect_calls == 0);
  assert(g_gradient.bottom_left == g.bottom_left);

  reset_counts();
  cheese_style_t flat = cheese_style_new();
  cheese_draw_bg(&cheese, &flat, 0, 0, 10, 10, cheese_color_rgb(1, 2, 3), 0,
                 1.0f);
  assert(g_gradient_calls == 0 && g_rect_calls == 1);
  assert(g_rect_color == cheese_color_rgb(1, 2, 3));

  reset_counts();
  cheese_draw_bg(&cheese, &flat, 0, 0, 10, 10, cheese_color_rgb(1, 2, 3), 0,
                 0.5f);
  assert(g_rect_calls == 1);
  assert(g_rect_color == cheese_color_rgba(1, 2, 3, 128) &&
         "alpha scales the flat fill");

  arena_free(frame);
  return 0;
}
