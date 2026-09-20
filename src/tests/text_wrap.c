/***********************************/

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>

#include <cheese/render/font.h>

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

static char g_text[64];
static u32 g_text_len;

//
//
//

static void stub_text(void *userdata, f32 x, f32 y, const string *text,
                      cheese_font_t *font, cheese_color_t color, f32 scale) {
  (void)userdata;
  (void)x;
  (void)y;
  (void)font;
  (void)color;
  (void)scale;

  g_text_len = (u32)min(text->len, (u64)(sizeof(g_text) - 1));
  memcpy(g_text, text->base, g_text_len);
  g_text[g_text_len] = '\0';
}

//
//
//

static i32 stub_create_texture(void *userdata, u32 width, u32 height,
                               cheese_texture_format_t format, const u8 *data) {
  (void)userdata;
  (void)width;
  (void)height;
  (void)format;
  (void)data;
  return 1;
}

//
//
//

static void stub_delete_texture(void *userdata, u32 texture_id) {
  (void)userdata;
  (void)texture_id;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
    .draw_text = stub_text,
    .create_texture = stub_create_texture,
    .delete_texture = stub_delete_texture,
};

//
//
//

static u64 find_text_id(cheese_t *cheese) {
  for (u64 i = 0; i < cheese_semantics_count(cheese); i++) {
    cheese_semantics_node_t *n = cheese_semantics_at(cheese, i);
    if (n->role == CHEESE_ROLE_TEXT_INPUT)
      return n->id;
  }

  return 0;
}

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
      "the quick brown fox jumps over the lazy dog and keeps on running");

  cheese_state_t *scroll = cheese_state_f32(cheese.store, "scroll", 0.0f);
  cheese_text_input_t wrap_state = {0};

  cheese_input_t away = {.mouse_x = 1000.0f, .mouse_y = 1000.0f};

  cheese_begin(&cheese, frame, font, &g_renderer, away, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 120.0f, 40.0f,
                    cheese_val_state(text), &wrap_state,
                    cheese_val_state(scroll), CHEESE_TEXT_INPUT_WRAP, font);
  cheese_end(&cheese);
  assert(wrap_state.content_h > wrap_state.viewport_h &&
         "a long line soft-wraps past the viewport");

  cheese_state_t *mtext =
      cheese_state_str(cheese.store, "mt", cheese_state_get_str(text));
  cheese_state_t *mscroll = cheese_state_f32(cheese.store, "ms", 0.0f);
  cheese_text_input_t ml_state = {0};

  cheese_begin(&cheese, frame, font, &g_renderer, away, 0.016f);
  cheese_text_input(
      &cheese, null, sem, 0.0f, 0.0f, 120.0f, 40.0f, cheese_val_state(mtext),
      &ml_state, cheese_val_state(mscroll), CHEESE_TEXT_INPUT_MULTILINE, font);
  cheese_end(&cheese);
  assert(ml_state.content_h < wrap_state.content_h &&
         "without wrap the same text stays one line");

  u64 id = find_text_id(&cheese);
  assert(id != 0 && "the field emits a semantics node");

  cheese.focus_id = id;
  wrap_state.caret = wrap_state.anchor =
      (i32)strlen(cheese_state_get_str(text));

  cheese_begin(&cheese, frame, font, &g_renderer, away, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 120.0f, 40.0f,
                    cheese_val_state(text), &wrap_state,
                    cheese_val_state(scroll), CHEESE_TEXT_INPUT_WRAP, font);
  cheese_end(&cheese);
  assert(cheese_state_get_f32(scroll) > 0.0f &&
         "the caret scrolls the viewport to stay visible");

  cheese_state_set_f32(scroll, 1.0e6f);

  cheese_begin(&cheese, frame, font, &g_renderer, away, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 120.0f, 40.0f,
                    cheese_val_state(text), &wrap_state,
                    cheese_val_state(scroll), CHEESE_TEXT_INPUT_WRAP, font);
  cheese_end(&cheese);

  f32 max_scroll = wrap_state.content_h - wrap_state.viewport_h;
  assert(cheese_state_get_f32(scroll) <= max_scroll &&
         "the scroll clamps to the content");

  cheese_state_t *wtext = cheese_state_str(cheese.store, "w", "hello world");
  cheese_text_input_t wstate = {0};
  cheese_input_t press = {
      .mouse_x = 26.0f, .mouse_y = 8.0f, .mouse_buttons = CHEESE_MOUSE_LEFT};
  cheese_input_t release = {.mouse_x = 26.0f, .mouse_y = 8.0f};

  cheese_begin(&cheese, frame, font, &g_renderer, press, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 24.0f,
                    cheese_val_state(wtext), &wstate, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_LINE, font);
  cheese_end(&cheese);
  assert(wstate.click_count == 1 && wstate.anchor == wstate.caret &&
         "a single click only places the caret");

  cheese_begin(&cheese, frame, font, &g_renderer, release, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 24.0f,
                    cheese_val_state(wtext), &wstate, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_LINE, font);
  cheese_end(&cheese);

  cheese_begin(&cheese, frame, font, &g_renderer, press, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 24.0f,
                    cheese_val_state(wtext), &wstate, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_LINE, font);
  cheese_end(&cheese);
  assert(wstate.click_count == 2 && wstate.anchor == 0 && wstate.caret == 5 &&
         "a double click selects the word");

  cheese_begin(&cheese, frame, font, &g_renderer, release, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 24.0f,
                    cheese_val_state(wtext), &wstate, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_LINE, font);
  cheese_end(&cheese);

  cheese_begin(&cheese, frame, font, &g_renderer, press, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 24.0f,
                    cheese_val_state(wtext), &wstate, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_LINE, font);
  cheese_end(&cheese);
  assert(wstate.click_count == 3 && wstate.anchor == 0 && wstate.caret == 11 &&
         "a triple click selects the line");

  cheese_state_t *phtext = cheese_state_str(cheese.store, "ph", "");
  cheese_text_input_t phstate = {0};
  phstate.placeholder = "type here";

  g_text_len = 0;
  g_text[0] = '\0';

  cheese_begin(&cheese, frame, font, &g_renderer, away, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 24.0f,
                    cheese_val_state(phtext), &phstate, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_LINE, font);
  cheese_end(&cheese);
  assert(strcmp(g_text, "type here") == 0 &&
         "an empty field draws its placeholder");

  cheese_state_set_str(phtext, "filled");

  g_text_len = 0;
  g_text[0] = '\0';

  cheese_begin(&cheese, frame, font, &g_renderer, away, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 24.0f,
                    cheese_val_state(phtext), &phstate, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_LINE, font);
  cheese_end(&cheese);
  assert(strcmp(g_text, "filled") == 0 &&
         "a filled field draws its text, not the placeholder");

  cheese_state_t *mltext = cheese_state_str(cheese.store, "ml", "aa\nbb");
  cheese_text_input_t mldrag = {0};
  f32 lh = font->active_variant->line_height;
  cheese_input_t line0 = {
      .mouse_x = 2.0f, .mouse_y = 2.0f, .mouse_buttons = CHEESE_MOUSE_LEFT};
  cheese_input_t line1 = {.mouse_x = 2.0f,
                          .mouse_y = lh + 6.0f,
                          .mouse_buttons = CHEESE_MOUSE_LEFT};

  cheese_begin(&cheese, frame, font, &g_renderer, line0, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 60.0f,
                    cheese_val_state(mltext), &mldrag, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_MULTILINE, font);
  cheese_end(&cheese);

  cheese_begin(&cheese, frame, font, &g_renderer, line1, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 60.0f,
                    cheese_val_state(mltext), &mldrag, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_MULTILINE, font);
  cheese_end(&cheese);
  assert(mldrag.anchor == 0 && mldrag.caret == 3 &&
         "a drag across lines selects the newline too");

  cheese_clear_frame_needed(&cheese);
  u32 blink_prop =
      cheese_prop_register(&cheese, CHEESE_PROP_CARET_BLINK, CHEESE_PROP_F32);
  cheese_style_t caret_style = cheese_style_new();
  cheese_style_set_prop_f32(&cheese, &caret_style, blink_prop, 1.0f);

  cheese_state_t *ctext = cheese_state_str(cheese.store, "ct", "hi");
  cheese_text_input_t cstate = {0};
  cheese_input_t cidle = {.mouse_x = 2.0f, .mouse_y = 2.0f};

  cheese_begin(&cheese, frame, font, &g_renderer, cidle, 0.016f);
  cheese_push_style(&cheese, caret_style);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 24.0f,
                    cheese_val_state(ctext), &cstate, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_LINE, font);
  cheese_pop_style(&cheese);
  cheese_end(&cheese);
  assert(cheese_needs_frame(&cheese) && "a blinking caret keeps waking frames");

  cheese_state_t *masktext = cheese_state_str(cheese.store, "mk", "secret");
  cheese_text_input_t mstate = {0};
  mstate.masked = true;

  g_text_len = 0;
  g_text[0] = '\0';
  cheese_begin(&cheese, frame, font, &g_renderer, cidle, 0.016f);
  cheese_text_input(&cheese, null, sem, 0.0f, 0.0f, 200.0f, 24.0f,
                    cheese_val_state(masktext), &mstate, cheese_val_f32(0.0f),
                    CHEESE_TEXT_INPUT_LINE, font);
  cheese_end(&cheese);
  assert(strcmp(g_text, "******") == 0 &&
         "a masked field draws one star per codepoint");

  cheese_font_destroy(&g_renderer, font);
  cheese_font_system_destroy();

  arena_free(frame);
  arena_free(font_arena);
  return 0;
}
