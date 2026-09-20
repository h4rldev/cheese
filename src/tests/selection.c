/***********************************/

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/selection.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

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

static const cstr *g_copied = null;

//
//
//

static void clip_set(void *userdata, const cstr *text) {
  (void)userdata;
  g_copied = text;
}

//
//
//

static void draw_label(cheese_t *cheese, cheese_font_t *font,
                       const string *label, b32 selectable) {
  if (selectable) {
    cheese_style_t on = cheese_style_new();
    cheese_style_set_selectable(&on, true);
    cheese_push_style(cheese, on);
  }

  cheese_draw_text(cheese, 0.0f, 16.0f, label, font, 0xFFFFFFFF, 1.0f);

  if (selectable)
    cheese_pop_style(cheese);
}

//
//
//

int main(void) {
  const cstr *path =
      "/run/current-system/sw/share/X11/fonts/MapleMono-Regular.ttf";
  if (access(path, R_OK) != 0)
    return 0;

  arena_t *persistent = arena_new(MiB(16), MiB(1));
  arena_t *font_arena = arena_new(MiB(32), MiB(1));
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_font_t *font = cheese_load_font(
      &g_renderer, font_arena, string_from_cstr(font_arena, path), 16);
  if (!font) {
    arena_free(frame);
    arena_free(font_arena);
    arena_free(persistent);
    return 0;
  }

  cheese_t cheese = cheese_default(persistent);
  cheese_set_clipboard(&cheese, null, clip_set, null);

  string label = {.base = (u8 *)"Hello world", .len = 11};

  cheese_begin(&cheese, frame, font, &g_renderer,
               (cheese_input_t){.window_w = 800, .window_h = 600}, 0.016f);
  draw_label(&cheese, font, &label, true);
  cheese_end(&cheese);
  assert(cheese.text_run_count == 1 && "selectable text is recorded");
  assert(!cheese.selection.active && "no selection yet");

  cheese_begin(&cheese, frame, font, &g_renderer,
               (cheese_input_t){.window_w = 800,
                                .window_h = 600,
                                .mouse_x = 1.0f,
                                .mouse_y = 16.0f,
                                .mouse_buttons = CHEESE_MOUSE_LEFT},
               0.016f);
  draw_label(&cheese, font, &label, true);
  cheese_end(&cheese);
  assert(cheese.selection.active && "a press starts a selection");
  assert(cheese.selection.anchor == 0 && "the anchor is at the press point");
  assert(cheese.active_id == CHEESE_SELECTION_ID &&
         "the selection owns the pointer");

  cheese_begin(&cheese, frame, font, &g_renderer,
               (cheese_input_t){.window_w = 800,
                                .window_h = 600,
                                .mouse_x = 48.0f,
                                .mouse_y = 16.0f,
                                .mouse_buttons = CHEESE_MOUSE_LEFT},
               0.016f);
  draw_label(&cheese, font, &label, true);
  cheese_end(&cheese);
  assert(cheese_selection_active(&cheese) && "dragging extends the selection");
  assert(cheese.selection.focus > 0 && "the focus moved past the anchor");
  u32 expected = cheese.selection.focus;

  cheese_begin(&cheese, frame, font, &g_renderer,
               (cheese_input_t){.window_w = 800,
                                .window_h = 600,
                                .mouse_x = 48.0f,
                                .mouse_y = 16.0f,
                                .key_mods = CHEESE_MOD_CTRL,
                                .key_events = {{.key = CHEESE_KEY_C}},
                                .key_event_count = 1},
               0.016f);
  draw_label(&cheese, font, &label, true);
  cheese_end(&cheese);
  assert(g_copied && "Ctrl+C copies the selection");
  assert(strlen(g_copied) == expected && "the copy is the selected length");
  assert(memcmp(g_copied, "Hello world", expected) == 0 &&
         "the copy is the selected text");

  cheese_begin(&cheese, frame, font, &g_renderer,
               (cheese_input_t){.window_w = 800,
                                .window_h = 600,
                                .mouse_x = -100.0f,
                                .mouse_y = 16.0f,
                                .mouse_buttons = CHEESE_MOUSE_LEFT},
               0.016f);
  draw_label(&cheese, font, &label, true);
  cheese_end(&cheese);
  assert(!cheese.selection.active && "a press outside clears the selection");

  cheese_begin(&cheese, frame, font, &g_renderer,
               (cheese_input_t){.window_w = 800, .window_h = 600}, 0.016f);
  draw_label(&cheese, font, &label, false);
  cheese_end(&cheese);
  assert(cheese.text_run_count == 0 && "non-selectable text is not recorded");

  cheese_font_destroy(&g_renderer, font);
  cheese_font_system_destroy();
  arena_free(frame);
  arena_free(font_arena);
  arena_free(persistent);
  return 0;
}
