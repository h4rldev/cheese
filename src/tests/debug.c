/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/debug.h>
#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/semantics.h>

/***********************************/

static u32 g_lines = 0;
static u32 g_texts = 0;

static void stub(void *userdata) { (void)userdata; }

//
//
//

static void count_line(void *userdata, f32 x1, f32 y1, f32 x2, f32 y2,
                       f32 thickness, cheese_color_t color) {
  (void)userdata;
  (void)x1;
  (void)y1;
  (void)x2;
  (void)y2;
  (void)thickness;
  (void)color;
  g_lines++;
}

//
//
//

static void count_text(void *userdata, f32 x, f32 y, const string *text,
                       cheese_font_t *font, cheese_color_t color, f32 scale) {
  (void)userdata;
  (void)x;
  (void)y;
  (void)text;
  (void)font;
  (void)color;
  (void)scale;
  g_texts++;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_line = count_line,
    .draw_text = count_text,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = {0};
  cheese_input_t input = {0};

  cheese_begin(&cheese, frame, null, &g_renderer, input, 0.016f);

  cheese_semantics_emit(&cheese, (cheese_semantics_t){.key = "a"},
                        CHEESE_ROLE_BUTTON, "A", null, 0,
                        (cheese_rect_t){1, 1, 10, 10});
  cheese_semantics_emit(&cheese, (cheese_semantics_t){.key = "b"},
                        CHEESE_ROLE_CHECKBOX, "B", null, 0,
                        (cheese_rect_t){20, 20, 10, 10});

  cheese_debug_overlay(&cheese, null);
  assert(g_lines == 8 && "one outline (4 lines) per bounded node");
  assert(g_texts == 0 && "no font means no labels");

  cheese_end(&cheese);
  arena_free(frame);
  return 0;
}
