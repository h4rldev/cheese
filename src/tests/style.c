/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>

#include <cheese/style.h>

/***********************************/

static void stub(void *userdata) { (void)userdata; }

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

  cheese_begin(&cheese, frame, null, &g_renderer, input, 0.016f);

  cheese_style_t base = cheese_style_new();
  cheese_style_set_bg_color(&base, 1);
  cheese_style_set_hover_color(&base, 2);
  cheese_style_set_pressed_color(&base, 3);
  cheese_style_set_disabled_color(&base, 4);
  cheese_style_set_focus_color(&base, 5);
  cheese_push_style(&cheese, base);

  cheese_style_t style;
  cheese_style_resolve(&cheese, &style, null);
  assert(style.bg_color == 1);

  cheese_style_apply_state(&style, CHEESE_STATE_HOVERED);
  assert(style.bg_color == 2);
  cheese_style_apply_state(&style, CHEESE_STATE_PRESSED);
  assert(style.bg_color == 3);
  cheese_style_apply_state(&style, CHEESE_STATE_HOVERED | CHEESE_STATE_PRESSED);
  assert(style.bg_color == 3 && "pressed outranks hover");
  cheese_style_apply_state(&style,
                           CHEESE_STATE_DISABLED | CHEESE_STATE_PRESSED);
  assert(style.bg_color == 4 && "disabled outranks pressed");

  cheese_style_t layered = cheese_style_new();
  cheese_style_set_bg_color(&layered, cheese_color_rgb(0, 0, 0));
  cheese_style_set_state_layer_color(&layered, cheese_color_rgb(255, 255, 255));

  cheese_style_apply_state(&layered, CHEESE_STATE_HOVERED);
  assert(layered.bg_color == cheese_color_rgb(20, 20, 20) &&
         "hover blends the state layer at 8%");

  cheese_style_t pressed = cheese_style_new();
  cheese_style_set_bg_color(&pressed, cheese_color_rgb(0, 0, 0));
  cheese_style_set_state_layer_color(&pressed, cheese_color_rgb(255, 255, 255));
  cheese_style_apply_state(&pressed, CHEESE_STATE_PRESSED);
  assert(pressed.bg_color == cheese_color_rgb(30, 30, 30) &&
         "pressed blends the state layer at 12%");

  cheese_style_t explicit = cheese_style_new();
  cheese_style_set_bg_color(&explicit, cheese_color_rgb(0, 0, 0));
  cheese_style_set_hover_color(&explicit, cheese_color_rgb(9, 9, 9));
  cheese_style_set_state_layer_color(&explicit,
                                     cheese_color_rgb(255, 255, 255));
  cheese_style_apply_state(&explicit, CHEESE_STATE_HOVERED);
  assert(explicit.bg_color == cheese_color_rgb(9, 9, 9) &&
         "an explicit state colour wins over the layer");

  cheese_style_t first = cheese_style_new();
  cheese_style_set_bg_color(&first, 0x22);
  cheese_style_class_register(&cheese, "first", first);

  cheese_style_t second = cheese_style_new();
  cheese_style_set_bg_color(&second, 0x33);
  cheese_style_class_register(&cheese, "second", second);

  cheese_style_t ab;
  cheese_style_resolve(&cheese, &ab, "first second");
  assert(ab.bg_color == 0x33 && "a later class wins (ordered merge)");

  cheese_style_t ba;
  cheese_style_resolve(&cheese, &ba, "second first");
  assert(ba.bg_color == 0x22 && "class order decides");

  cheese_style_t none;
  cheese_style_resolve(&cheese, &none, "missing");
  assert(none.bg_color == 1 && "the style stack still shows through");
  u32 thumb_color = cheese_style_prop_register(&cheese, "test/thumb/color",
                                               CHEESE_PROP_COLOR);
  u32 thumb_radius =
      cheese_style_prop_register(&cheese, "test/thumb/radius", CHEESE_PROP_F32);
  u32 thumb_sides =
      cheese_style_prop_register(&cheese, "test/thumb/sides", CHEESE_PROP_U32);
  assert(thumb_color != 0 && thumb_radius != 0 && thumb_color != thumb_radius);
  assert(cheese_style_prop_register(&cheese, "test/thumb/color",
                                    CHEESE_PROP_COLOR) == thumb_color &&
         "property ids are stable");

  cheese_style_t props = cheese_style_new();
  cheese_style_prop_set_color(&cheese, &props, thumb_color,
                              cheese_color_rgb(10, 10, 10));
  cheese_style_prop_set_f32(&cheese, &props, thumb_radius, 4.0f);
  cheese_style_prop_set_u32(&cheese, &props, thumb_sides, 5);
  cheese_push_style(&cheese, props);

  cheese_style_t resolved;
  cheese_style_resolve(&cheese, &resolved, null);

  assert(cheese_style_prop_get_color(&resolved, thumb_color, 0) ==
             cheese_color_rgb(10, 10, 10) &&
         "a pushed property merges into resolve");
  assert(cheese_style_prop_get_f32(&resolved, thumb_radius, -1.0f) == 4.0f);
  assert(cheese_style_prop_get_u32(&resolved, thumb_sides, 0) == 5 &&
         "a u32 property round-trips");
  assert(cheese_style_prop_get_u32(&resolved, thumb_radius, 99) == 99 &&
         "an f32 property does not answer a u32 read");
  assert(cheese_style_prop_get_f32(&resolved, thumb_color, -1.0f) == -1.0f &&
         "a colour property does not answer an f32 read");

  cheese_prop_t absent;
  assert(!cheese_style_prop_get(&resolved, 9999, &absent) &&
         "an unknown property is absent");

  cheese_style_t transparent = cheese_style_new();
  cheese_style_prop_set_color(&cheese, &transparent, thumb_color, 0);
  assert(cheese_style_prop_get_color(&transparent, thumb_color, 0x12345678) ==
             0 &&
         "a transparent property value is representable");

  cheese_style_t cls = cheese_style_new();
  cheese_style_prop_set_color(&cheese, &cls, thumb_color,
                              cheese_color_rgb(20, 20, 20));
  cheese_style_class_register(&cheese, "prop-test", cls);

  cheese_style_t overridden;
  cheese_style_resolve(&cheese, &overridden, "prop-test");
  assert(cheese_style_prop_get_color(&overridden, thumb_color, 0) ==
             cheese_color_rgb(20, 20, 20) &&
         "class override recurses into properties");
  assert(cheese_style_prop_get_f32(&overridden, thumb_radius, -1.0f) == 4.0f &&
         "the pushed property survives the class override");

  cheese_style_t boxed = cheese_style_new();
  cheese_style_set_padding(&boxed, 1.0f, 2.0f, 3.0f, 4.0f);
  cheese_style_set_margin(&boxed, 5.0f, 6.0f, 7.0f, 8.0f);
  assert(cheese_style_get_pad_left(&boxed) == 1.0f &&
         cheese_style_get_pad_bottom(&boxed) == 2.0f &&
         cheese_style_get_pad_top(&boxed) == 3.0f &&
         cheese_style_get_pad_right(&boxed) == 4.0f &&
         "padding getters read back the four-edge setter");
  assert(cheese_style_get_margin_left(&boxed) == 5.0f &&
         cheese_style_get_margin_bottom(&boxed) == 6.0f &&
         cheese_style_get_margin_top(&boxed) == 7.0f &&
         cheese_style_get_margin_right(&boxed) == 8.0f &&
         "margin getters read back the four-edge setter");
  assert(cheese_style_get_margin_width(&boxed) == 13.0f &&
         cheese_style_get_margin_height(&boxed) == 13.0f &&
         "margin width/height sum the opposing edges");

  cheese_end(&cheese);
  arena_free(frame);
  return 0;
}
