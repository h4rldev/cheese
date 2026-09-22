/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>

#include <cheese/style/resolve.h>
#include <cheese/style/value.h>

/***********************************/

static void stub(void *ud) { (void)ud; }

static void stub_rect(void *ud, cheese_corners_t r, f32 x, f32 y, f32 w, f32 h,
                      cheese_color_t c) {
  (void)ud;
  (void)r;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)c;
}

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
};

int main(void) {
  arena_t *arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(arena);
  cheese_begin(&cheese, arena, null, &g_renderer,
               (cheese_input_t){.window_w = 800, .window_h = 600}, 0.016f);

  cheese_style_t s;
  cheese_style_resolve_scoped(&cheese, &s, CHEESE_ROLE_LABEL, null, null);
  assert(!cheese_style_get_selectable(&s) && "selectable is off by default");

  cheese_style_t on = cheese_style_new();
  cheese_style_set_selectable(&on, true);
  cheese_push_style(&cheese, on);

  cheese_style_resolve_scoped(&cheese, &s, CHEESE_ROLE_LABEL, null, null);
  assert(cheese_style_get_selectable(&s) && "inherits from the scope");
  assert(cheese_style_get_selectable(cheese_current_style(&cheese)) &&
         "the cached draw-time style reflects the widget");

  cheese_style_t off = cheese_style_new();
  cheese_style_set_selectable(&off, false);
  cheese_style_resolve_scoped(&cheese, &s, CHEESE_ROLE_LABEL, null, &off);
  assert(!cheese_style_get_selectable(&s) && "explicit wins over inherited");

  cheese_pop_style(&cheese);
  cheese_end(&cheese);
  arena_free(arena);
  return 0;
}
