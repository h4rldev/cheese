/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>

#include <cheese/style/prop.h>
#include <cheese/style/resolve.h>
#include <cheese/style/value.h>

#include <cheese/widgets/slider.h>

/***********************************/

static void stub(void *userdata) { (void)userdata; }

//
//
//

static u64 find_id(cheese_t *cheese, const cstr *key) {
  for (u64 i = 0; i < cheese_semantics_count(cheese); i++) {
    cheese_semantics_node_t *n = cheese_semantics_at(cheese, i);
    if (n->key && strcmp(n->key, key) == 0)
      return n->id;
  }

  return 0;
}

//
//
//

static cheese_color_t g_last_rect_color;
static cheese_corners_t g_last_rect_radius;

static void stub_rect(void *userdata, cheese_corners_t radius, f32 x, f32 y,
                      f32 w, f32 h, cheese_color_t color) {
  (void)userdata;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  g_last_rect_color = color;
  g_last_rect_radius = radius;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = cheese_default(frame);
  cheese_state_t *value = cheese_state_f32(cheese.store, "value", 0.0f);

  cheese_input_t press = {
      .mouse_x = 50.0f,
      .mouse_y = 5.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };

  cheese_begin(&cheese, frame, null, &g_renderer, press, 0.016f);
  f32 v = cheese_slider(&cheese, null, (cheese_semantics_t){.key = "s"}, 0.0f,
                        0.0f, 100.0f, 10.0f, cheese_val_state(value));
  cheese_end(&cheese);

  assert(v == 0.5f && "a press on the track seeks");
  assert(cheese_state_get_f32(value) == 0.5f && "and writes the state");

  cheese_input_t drag = {
      .mouse_x = 80.0f,
      .mouse_y = 5.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };

  cheese_begin(&cheese, frame, null, &g_renderer, drag, 0.016f);
  v = cheese_slider(&cheese, null, (cheese_semantics_t){.key = "s"}, 0.0f, 0.0f,
                    100.0f, 10.0f, cheese_val_state(value));
  cheese_end(&cheese);
  assert(v == 0.8f && "drag tracks the cursor");

  cheese_input_t released = {
      .mouse_x = 20.0f,
      .mouse_y = 500.0f,
      .mouse_buttons = 0,
  };
  cheese_begin(&cheese, frame, null, &g_renderer, released, 0.016f);
  v = cheese_slider(&cheese, null, (cheese_semantics_t){.key = "s"}, 0.0f, 0.0f,
                    100.0f, 10.0f, cheese_val_state(value));
  cheese_end(&cheese);
  assert(v == 0.8f && "no drag without the button");

  cheese_input_t away = {.mouse_x = 500.0f, .mouse_y = 500.0f};
  cheese_begin(&cheese, frame, null, &g_renderer, away, 0.016f);
  cheese_slider(&cheese, null, (cheese_semantics_t){.key = "s"}, 0.0f, 0.0f,
                100.0f, 10.0f, cheese_val_state(value));
  cheese_end(&cheese);

  u64 sid = find_id(&cheese, "s");
  assert(sid != 0 && "the slider emits a node");
  cheese.focus_id = sid;

  cheese_input_t escape = {.mouse_x = 500.0f, .mouse_y = 500.0f};
  escape.key_event_count = 1;
  escape.key_events[0] = (cheese_key_event_t){.key = CHEESE_KEY_ESCAPE};

  cheese_begin(&cheese, frame, null, &g_renderer, escape, 0.016f);
  cheese_slider(&cheese, null, (cheese_semantics_t){.key = "s"}, 0.0f, 0.0f,
                100.0f, 10.0f, cheese_val_state(value));
  cheese_end(&cheese);
  assert(cheese.edit_id == 0 && "escape leaves edit mode");

  cheese_input_t left = {.mouse_x = 500.0f, .mouse_y = 500.0f};
  left.key_event_count = 1;
  left.key_events[0] = (cheese_key_event_t){.key = CHEESE_KEY_LEFT};

  cheese_begin(&cheese, frame, null, &g_renderer, left, 0.016f);
  v = cheese_slider(&cheese, null, (cheese_semantics_t){.key = "s"}, 0.0f, 0.0f,
                    100.0f, 10.0f, cheese_val_state(value));
  cheese_end(&cheese);
  assert(v == 0.8f && "a focused slider ignores arrows until activated");

  cheese_input_t enter = {.mouse_x = 500.0f, .mouse_y = 500.0f};
  enter.key_event_count = 1;
  enter.key_events[0] = (cheese_key_event_t){.key = CHEESE_KEY_ENTER};

  cheese_begin(&cheese, frame, null, &g_renderer, enter, 0.016f);
  cheese_slider(&cheese, null, (cheese_semantics_t){.key = "s"}, 0.0f, 0.0f,
                100.0f, 10.0f, cheese_val_state(value));
  cheese_end(&cheese);
  assert(cheese.edit_id == sid && "enter starts editing the slider");

  cheese_begin(&cheese, frame, null, &g_renderer, left, 0.016f);
  v = cheese_slider(&cheese, null, (cheese_semantics_t){.key = "s"}, 0.0f, 0.0f,
                    100.0f, 10.0f, cheese_val_state(value));
  cheese_end(&cheese);
  assert(v == 0.75f && "an editing slider steps with arrows");

  cheese_begin(&cheese, frame, null, &g_renderer, away, 0.016f);
  {
    u32 thumb_color = cheese_style_prop_register(&cheese, "slider/thumb/color",
                                                 CHEESE_PROP_COLOR);
    u32 thumb_radius = cheese_style_prop_register(
        &cheese, "slider/thumb/radius", CHEESE_PROP_F32);

    cheese_style_t thumb_style = cheese_style_new();
    cheese_style_prop_set_color(&cheese, &thumb_style, thumb_color,
                                cheese_color_rgb(1, 2, 3));
    cheese_style_prop_set_f32(&cheese, &thumb_style, thumb_radius, 7.0f);
    cheese_style_class_register(&cheese, "test-thumb", thumb_style);

    cheese_slider(&cheese, "slider test-thumb",
                  (cheese_semantics_t){.key = "s"}, 0.0f, 0.0f, 100.0f, 10.0f,
                  cheese_val_state(value));
  }
  cheese_end(&cheese);

  assert(g_last_rect_color == cheese_color_rgb(1, 2, 3) &&
         "the thumb property colours the nipple");
  assert(g_last_rect_radius.top_left == 7.0f &&
         "the thumb property sets the nipple radius");

  arena_free(frame);
  return 0;
}
