/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/semantics.h>

#include <cheese/style/resolve.h>
#include <cheese/style/theme.h>
#include <cheese/style/value.h>

#include <cheese/widgets/button.h>

/***********************************/

static cheese_color_t g_last_rect;
static cheese_color_t g_last_border;

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
  g_last_rect = color;
}

//
//
//

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
  g_last_border = color;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
    .draw_border = stub_border,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = cheese_default(frame);
  cheese_input_t input = {.mouse_x = 1000.0f, .mouse_y = 1000.0f};

  assert(cheese_theme_palette(&cheese) == null && "no palette before apply");

  cheese_theme_t dark = cheese_theme_dark();
  cheese_theme_t light = cheese_theme_light();
  assert(light.primary != dark.primary && "the presets differ");

  cheese_theme_apply(&cheese, &dark);
  assert(cheese_theme_palette(&cheese)->primary == dark.primary &&
         "apply stores the palette");

  cheese_begin(&cheese, frame, null, &g_renderer, input, 0.016f);

  assert(cheese_theme_palette(&cheese)->surface == dark.surface &&
         "the palette persists into the frame");

  cheese_style_t button_style;
  cheese_style_resolve(&cheese, &button_style, CHEESE_CLASS_BUTTON);
  assert(button_style.bg_color == dark.primary &&
         "the built-in button class is registered");
  assert(button_style.text_color == dark.on_primary);

  cheese_style_t hovered_style;
  cheese_style_resolve(&cheese, &hovered_style, CHEESE_CLASS_BUTTON);
  cheese_style_apply_state(&hovered_style, CHEESE_STATE_HOVERED);
  assert(hovered_style.bg_color != dark.primary &&
         "hover applies the M3 state layer");

  cheese_style_t checkbox_style;
  cheese_style_resolve(&cheese, &checkbox_style, CHEESE_CLASS_CHECKBOX);
  assert(checkbox_style.corner_radius.top_left == dark.radius_sm &&
         "the built-in checkbox class is registered");

  g_last_rect = 0;
  cheese_button(&cheese, null, (cheese_semantics_t){0}, 0.0f, 0.0f, 100.0f,
                40.0f, cheese_val_str(""), null, 0, 0, 0);
  assert(g_last_rect == dark.primary &&
         "a bare button resolves its built-in role class");

  assert(cheese_theme_color(&cheese, "accent", 0xDEAD) == 0xDEAD &&
         "missing variables fall back");

  cheese_theme_set_color(&cheese, "accent", 0xABCDEF);
  assert(cheese_theme_color(&cheese, "accent", 0) == 0xABCDEF);

  cheese_theme_set_color(&cheese, "accent", 0x123456);
  assert(cheese_theme_color(&cheese, "accent", 0) == 0x123456 &&
         "a set overwrites");

  cheese_theme_set_f32(&cheese, "radius", 8.0f);
  assert(cheese_theme_f32(&cheese, "radius", 0.0f) == 8.0f);

  cheese_theme_set_u32(&cheese, "font", 20);
  assert(cheese_theme_u32(&cheese, "font", 0) == 20);

  cheese_theme_set_b32(&cheese, "compact", true);
  assert(cheese_theme_b32(&cheese, "compact", false) &&
         "a b32 variable round-trips");
  assert(cheese_theme_b32(&cheese, "missing", true) &&
         "a missing b32 variable falls back");

  assert(cheese_theme_f32(&cheese, "accent", 1.5f) == 1.5f &&
         "a mistyped lookup falls back");

  cheese_style_t bare_role;
  cheese_style_resolve_scoped(&cheese, &bare_role, CHEESE_ROLE_BUTTON, null,
                              null);
  assert(bare_role.bg_color == dark.primary &&
         "a bare role resolves its background");
  assert(bare_role.text_color == dark.on_primary &&
         "a role default wins over inherited frame text");

  cheese_end(&cheese);

  u64 button_id = 0;
  for (u64 i = 0; i < cheese_semantics_count(&cheese); i++) {
    cheese_semantics_node_t *n = cheese_semantics_at(&cheese, i);
    if (n->role == CHEESE_ROLE_BUTTON)
      button_id = n->id;
  }
  assert(button_id != 0 && "the button emits a semantics node");

  cheese.focus_id = button_id;

  cheese_begin(&cheese, frame, null, &g_renderer, input, 0.016f);
  g_last_border = 0;
  cheese_button(&cheese, null, (cheese_semantics_t){0}, 0.0f, 0.0f, 100.0f,
                40.0f, cheese_val_str(""), null, 0, 0, 0);
  cheese_end(&cheese);
  assert(g_last_border == dark.primary &&
         "a focused widget draws its focus ring");

  arena_free(frame);
  return 0;
}
