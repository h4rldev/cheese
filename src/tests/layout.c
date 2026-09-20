/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/layout.h>

#include <cheese/widgets/container.h>

/***********************************/

static u32 g_clips = 0;

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

static void count_clip(void *userdata, f32 x, f32 y, f32 w, f32 h) {
  (void)userdata;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  g_clips++;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
    .push_clip = count_clip,
    .pop_clip = stub,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = cheese_default(frame);
  cheese_input_t input = {0};

  cheese_begin(&cheese, frame, null, &g_renderer, input, 0.016f);

  assert(cheese_size_resolve(cheese_fill(), 200.0f, 0.0f) == 200.0f);
  assert(cheese_size_resolve(cheese_pct(0.5f), 200.0f, 0.0f) == 100.0f);
  assert(cheese_size_resolve(cheese_px(30.0f), 200.0f, 0.0f) == 30.0f);
  assert(cheese_size_resolve(cheese_fit(), 200.0f, 42.0f) == 42.0f);

  cheese_size_t clamped = cheese_pct(0.9f);
  clamped.max = 50.0f;
  assert(cheese_size_resolve(clamped, 200.0f, 0.0f) == 50.0f && "max clamps");

  cheese_layout_t row = cheese_layout_default();
  row.x = 0.0f;
  row.y = 0.0f;
  row.width = 100.0f;
  row.height = 100.0f;
  row.origin_x = 0.0f;
  row.origin_y = 0.0f;
  row.spacing = 0.0f;
  cheese_push_layout(&cheese, row);

  f32 w = 40.0f;
  f32 h = 10.0f;
  f32 x, y;

  cheese_layout_place(&cheese, &w, &h, &x, &y);
  assert(x == 0.0f && y == 0.0f);
  cheese_layout_place(&cheese, &w, &h, &x, &y);
  assert(x == 40.0f);
  cheese_layout_place(&cheese, &w, &h, &x, &y);
  assert(x == 80.0f);

  cheese_layout_set_wrap(cheese_current_layout(&cheese), true);
  cheese_layout_place(&cheese, &w, &h, &x, &y);
  assert(x == 0.0f && y == 10.0f && "overflows onto the next line");

  cheese_layout_t col = cheese_layout_default();
  col.horizontal = false;
  col.width = 100.0f;
  col.height = 200.0f;
  col.origin_x = 0.0f;
  col.origin_y = 0.0f;
  cheese_push_layout(&cheese, col);
  cheese_layout_set_cross(cheese_current_layout(&cheese), CHEESE_SIZE_FILL,
                          0.0f);

  f32 cw = 10.0f;
  f32 ch = 10.0f;
  cheese_layout_place(&cheese, &cw, &ch, &x, &y);
  assert(cw == 100.0f && "fill stretches the cross axis");
  assert(y == 0.0f);

  cheese_layout_t grid = cheese_layout_default();
  grid.width = 100.0f;
  grid.height = 100.0f;
  grid.origin_x = 0.0f;
  grid.origin_y = 0.0f;
  grid.spacing = 0.0f;
  cheese_push_layout(&cheese, grid);
  cheese_layout_set_columns(cheese_current_layout(&cheese), 2);

  f32 gw = 40.0f;
  f32 gh = 10.0f;
  f32 gx, gy;

  cheese_layout_place(&cheese, &gw, &gh, &gx, &gy);
  assert(gw == 50.0f && "a grid forces the cell width");
  assert(gx == 0.0f);
  cheese_layout_place(&cheese, &gw, &gh, &gx, &gy);
  assert(gx == 50.0f);
  cheese_layout_place(&cheese, &gw, &gh, &gx, &gy);
  assert(gx == 0.0f && gy == 10.0f && "wraps after the columns");

  cheese_begin_scroll(&cheese, null, (cheese_semantics_t){0}, 10.0f, 10.0f,
                      cheese_px(100.0f), cheese_px(100.0f),
                      cheese_val_f32(20.0f), cheese_val_f32(30.0f));
  cheese_layout_t *scroll = cheese_current_layout(&cheese);
  assert(scroll->x == -10.0f && scroll->y == -20.0f &&
         "children are offset by -scroll");
  cheese_end_scroll(&cheese);
  assert(g_clips > 0 && "the viewport clips");

  cheese_layout_t anchor_box = cheese_layout_default();
  anchor_box.width = 100.0f;
  anchor_box.height = 100.0f;
  anchor_box.origin_x = 0.0f;
  anchor_box.origin_y = 0.0f;
  cheese_push_layout(&cheese, anchor_box);

  f32 aw = 20.0f, ah = 10.0f, ax, ay;
  cheese_push_anchor(&cheese, CHEESE_ANCHOR_BOTTOM_RIGHT, 5.0f, 5.0f);
  cheese_layout_place(&cheese, &aw, &ah, &ax, &ay);
  assert(ax == 75.0f && ay == 85.0f && "bottom-right anchor insets");
  cheese_pop_anchor(&cheese);

  cheese_push_anchor(&cheese, CHEESE_ANCHOR_CENTER, 0.0f, 0.0f);
  aw = 20.0f;
  ah = 10.0f;
  cheese_layout_place(&cheese, &aw, &ah, &ax, &ay);
  assert(ax == 40.0f && ay == 45.0f && "center anchor");
  cheese_pop_anchor(&cheese);

  f32 fw = 20.0f, fh = 10.0f, fx, fy;
  cheese_push_anchor(&cheese, CHEESE_ANCHOR_TOP_RIGHT, 0.0f, 0.0f);
  cheese_layout_place(&cheese, &fw, &fh, &fx, &fy);
  cheese_pop_anchor(&cheese);
  assert(fx == 80.0f && fy == 0.0f && "anchor leaves the flow cursor alone");

  cheese_pop_layout(&cheese);

  cheese_end(&cheese);
  arena_free(frame);
  return 0;
}
