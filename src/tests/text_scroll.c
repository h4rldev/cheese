/***********************************/

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/state.h>

#include <cheese/render/font.h>

#include <cheese/widgets/text.h>

/***********************************/

static void stub(void *userdata) { (void)userdata; }

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

static void stub_text(void *userdata, f32 x, f32 y, const string *text,
                      cheese_font_t *font, cheese_color_t color, f32 scale) {
  (void)userdata;
  (void)x;
  (void)y;
  (void)text;
  (void)font;
  (void)color;
  (void)scale;
}

static i32 stub_create_texture(void *userdata, u32 width, u32 height,
                               cheese_texture_format_t format, const u8 *data) {
  (void)userdata;
  (void)width;
  (void)height;
  (void)format;
  (void)data;
  return 1;
}

static void stub_delete_texture(void *userdata, u32 texture_id) {
  (void)userdata;
  (void)texture_id;
}

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
    .draw_text = stub_text,
    .create_texture = stub_create_texture,
    .delete_texture = stub_delete_texture,
};

int main(void) {
  const cstr *path =
      "/run/current-system/sw/share/X11/fonts/MapleMono-Regular.ttf";
  if (access(path, R_OK) != 0)
    return 0; // no system font: nothing to measure, skip

  arena_t *font_arena = arena_new(MiB(32), MiB(1));
  cheese_font_t *font = cheese_load_font(
      &g_renderer, font_arena, string_from_cstr(font_arena, path), 16);
  if (!font) {
    arena_free(font_arena);
    return 0;
  }

  arena_t *frame = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(frame);
  cheese_semantics_t sem = {.key = "t"};

  cheese_state_t *text = cheese_state_str(
      cheese.store, "t",
      "l0\nl1\nl2\nl3\nl4\nl5\nl6\nl7\nl8\nl9\nl10\nl11\nl12\nl13\nl14\nl15");
  cheese_state_t *scroll = cheese_state_f32(cheese.store, "s", 0.0f);
  cheese_text_input_t st = {0};
  st.caret = st.anchor = (i32)strlen(cheese_state_get_str(text));

  cheese_input_t away = {.mouse_x = 1000.0f, .mouse_y = 1000.0f};

  cheese_begin(&cheese, frame, font, &g_renderer, away, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 120.0f, 40.0f,
                    cheese_val_state(text), &st, cheese_val_state(scroll),
                    CHEESE_TEXT_INPUT_WRAP, font);
  cheese_end(&cheese);

  f32 at_caret = cheese_state_get_f32(scroll);
  assert(at_caret > 0.0f && "the caret at the end scrolls the viewport down");

  cheese_input_t wheel_up = {
      .mouse_x = 10.0f, .mouse_y = 10.0f, .scroll_y = 1.0f};
  cheese_begin(&cheese, frame, font, &g_renderer, wheel_up, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 120.0f, 40.0f,
                    cheese_val_state(text), &st, cheese_val_state(scroll),
                    CHEESE_TEXT_INPUT_WRAP, font);
  cheese_end(&cheese);

  assert(cheese_state_get_f32(scroll) < at_caret &&
         "the wheel scrolls away from the caret instead of snapping back");

  for (i32 i = 0; i < 40; i++) {
    cheese_begin(&cheese, frame, font, &g_renderer, wheel_up, 0.016f);
    cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 120.0f, 40.0f,
                      cheese_val_state(text), &st, cheese_val_state(scroll),
                      CHEESE_TEXT_INPUT_WRAP, font);
    cheese_end(&cheese);
  }
  assert(cheese_state_get_f32(scroll) == 0.0f &&
         "the wheel reaches the top while the caret stays put");

  cheese_font_destroy(&g_renderer, font);
  cheese_font_system_destroy();

  arena_free(frame);
  arena_free(font_arena);
  return 0;
}
