/***********************************/

#include <assert.h>
#include <math.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese.h>

/***********************************/

// A third-party widget: it lives entirely against the public umbrella header
// and adds two styling fields of its own (an f32 angle and a colour) with no
// edit to cheese's core. That is the point of the test.

static u32 gauge_needle_angle;
static u32 gauge_needle_color;

static u32 gauge_prop_needle_angle(cheese_t *cheese) {
  if (!gauge_needle_angle)
    gauge_needle_angle = cheese_style_prop_register(
        cheese, "gauge/needle/angle", CHEESE_PROP_F32);
  return gauge_needle_angle;
}

static u32 gauge_prop_needle_color(cheese_t *cheese) {
  if (!gauge_needle_color)
    gauge_needle_color = cheese_style_prop_register(
        cheese, "gauge/needle/color", CHEESE_PROP_COLOR);
  return gauge_needle_color;
}

// Thin wrappers, so call sites read like named setters.
static void gauge_style_set_needle_angle(cheese_t *cheese,
                                         cheese_style_t *style, f32 degrees) {
  cheese_style_prop_set_f32(cheese, style, gauge_prop_needle_angle(cheese),
                            degrees);
}

static void gauge_style_set_needle_color(cheese_t *cheese,
                                         cheese_style_t *style,
                                         cheese_color_t color) {
  cheese_style_prop_set_color(cheese, style, gauge_prop_needle_color(cheese),
                              color);
}

static void cheese_gauge(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, f32 x, f32 y, f32 w,
                         f32 h, f32 value) {
  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_NONE, classes, null);

  f32 angle =
      cheese_style_prop_get_f32(&style, gauge_prop_needle_angle(cheese), -1.0f);
  if (angle < 0.0f)
    angle = value * 360.0f;

  cheese_color_t color = cheese_style_prop_get_color(
      &style, gauge_prop_needle_color(cheese), style.text_color);
  if (color == 0)
    color = cheese_color_rgb(255, 255, 255);

  f32 cx = x + w * 0.5f;
  f32 cy = y + h * 0.5f;
  f32 radius = (w < h ? w : h) * 0.5f;
  f32 radians = angle * (3.14159265f / 180.0f);

  cheese_draw_line(cheese, cx, cy, cx + radius * sinf(radians),
                   cy - radius * cosf(radians), 2.0f, color);
}

//
//
//

static void stub(void *userdata) { (void)userdata; }

//
//
//

static f32 g_x2, g_y2;
static cheese_color_t g_color;

static void stub_line(void *userdata, f32 x1, f32 y1, f32 x2, f32 y2,
                      f32 thickness, cheese_color_t color) {
  (void)userdata;
  (void)x1;
  (void)y1;
  (void)thickness;
  g_x2 = x2;
  g_y2 = y2;
  g_color = color;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_line = stub_line,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = cheese_default(frame);
  cheese_input_t input = {0};

  cheese_begin(&cheese, frame, null, &g_renderer, input, 0.016f);
  {
    cheese_style_t needle = cheese_style_new();
    gauge_style_set_needle_angle(&cheese, &needle, 90.0f);
    gauge_style_set_needle_color(&cheese, &needle, cheese_color_rgb(1, 2, 3));
    cheese_style_class_register(&cheese, "test-gauge", needle);

    cheese_gauge(&cheese, "gauge test-gauge", (cheese_semantics_t){.key = "g"},
                 0.0f, 0.0f, 100.0f, 100.0f, 0.0f);
  }
  cheese_end(&cheese);

  assert(g_color == cheese_color_rgb(1, 2, 3) &&
         "a third-party property colours the needle");
  assert(fabsf(g_x2 - 100.0f) < 0.01f && fabsf(g_y2 - 50.0f) < 0.01f &&
         "a third-party property drives the needle geometry");

  arena_free(frame);
  return 0;
}
