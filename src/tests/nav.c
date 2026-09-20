/***********************************/

#include <assert.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/semantics.h>

#include <cheese/widgets/button.h>
#include <cheese/widgets/text.h>

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

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
};

//
//
//

static cheese_input_t arrow(cheese_key_t key) {
  cheese_input_t in = {.mouse_x = 1000.0f, .mouse_y = 1000.0f};
  in.key_event_count = 1;
  in.key_events[0] = (cheese_key_event_t){.key = key};
  return in;
}

//
//
//

static u64 id_of(cheese_t *cheese, const cstr *key) {
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

static void draw_grid(cheese_t *cheese) {
  cheese_button(cheese, null, (cheese_semantics_t){.key = "tl"}, 0.0f, 0.0f,
                50.0f, 20.0f, cheese_val_str(""), null, 0, 0, 0);
  cheese_button(cheese, null, (cheese_semantics_t){.key = "tr"}, 100.0f, 0.0f,
                50.0f, 20.0f, cheese_val_str(""), null, 0, 0, 0);
  cheese_button(cheese, null, (cheese_semantics_t){.key = "bl"}, 0.0f, 100.0f,
                50.0f, 20.0f, cheese_val_str(""), null, 0, 0, 0);
  cheese_button(cheese, null, (cheese_semantics_t){.key = "br"}, 100.0f, 100.0f,
                50.0f, 20.0f, cheese_val_str(""), null, 0, 0, 0);
}

//
//
//

static cheese_text_input_t g_text_state;

static void draw_field_row(cheese_t *cheese) {
  cheese_text_input(cheese, null, (cheese_semantics_t){.key = "ti"}, 0.0f, 0.0f,
                    100.0f, 24.0f, cheese_val_str("x"), &g_text_state,
                    cheese_val_f32(0.0f), CHEESE_TEXT_INPUT_LINE, null);
  cheese_button(cheese, null, (cheese_semantics_t){.key = "b2"}, 200.0f, 0.0f,
                50.0f, 20.0f, cheese_val_str(""), null, 0, 0, 0);
}

//
//
//

int main(void) {
  arena_t *arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(arena);

  cheese_input_t away = {.mouse_x = 1000.0f, .mouse_y = 1000.0f};

  cheese_begin(&cheese, arena, null, &g_renderer, away, 0.016f);
  draw_grid(&cheese);
  cheese_end(&cheese);

  u64 tl = id_of(&cheese, "tl");
  u64 tr = id_of(&cheese, "tr");
  u64 bl = id_of(&cheese, "bl");
  u64 br = id_of(&cheese, "br");
  assert(tl && tr && bl && br && "the grid emits keyed nodes");

  cheese.focus_id = tl;

  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_RIGHT),
               0.016f);
  draw_grid(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == tr && "right moves to the same-row neighbour");

  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_DOWN),
               0.016f);
  draw_grid(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == br && "down moves to the column below");

  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_LEFT),
               0.016f);
  draw_grid(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == bl && "left moves back across the row");

  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_UP), 0.016f);
  draw_grid(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == tl && "up moves to the column above");

  cheese_set_arrow_nav(&cheese, false);
  cheese.focus_id = tl;

  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_RIGHT),
               0.016f);
  draw_grid(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == tl && "arrow nav can be disabled");

  cheese_set_arrow_nav(&cheese, true);

  cheese.focus_id = 0;

  cheese_begin(&cheese, arena, null, &g_renderer, away, 0.016f);
  draw_field_row(&cheese);
  cheese_end(&cheese);

  u64 ti = id_of(&cheese, "ti");
  u64 b2 = id_of(&cheese, "b2");
  assert(ti && b2 && "the field row emits keyed nodes");

  cheese.focus_id = ti;

  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_RIGHT),
               0.016f);
  draw_field_row(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == b2 &&
         "nav works while a text field is only focused");

  cheese.focus_id = ti;
  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_ENTER),
               0.016f);
  draw_field_row(&cheese);
  cheese_end(&cheese);
  assert(cheese.edit_id == ti && "enter starts editing");

  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_RIGHT),
               0.016f);
  draw_field_row(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == ti && "editing keeps the arrow keys");

  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_ESCAPE),
               0.016f);
  draw_field_row(&cheese);
  cheese_end(&cheese);
  assert(cheese.edit_id == 0 && "escape leaves edit mode");

  cheese_begin(&cheese, arena, null, &g_renderer, arrow(CHEESE_KEY_RIGHT),
               0.016f);
  draw_field_row(&cheese);
  cheese_end(&cheese);
  assert(cheese.focus_id == b2 && "nav resumes after escape");

  arena_free(arena);
  return 0;
}
