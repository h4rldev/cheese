/***********************************/

#include <assert.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>

#include <cheese/widgets/text.h>

/***********************************/

static char clip_buf[256];

static const cstr *clip_get(void *ud) {
  (void)ud;
  return clip_buf[0] ? clip_buf : null;
}

static void clip_set(void *ud, const cstr *text) {
  (void)ud;
  strncpy(clip_buf, text, sizeof(clip_buf) - 1);
  clip_buf[sizeof(clip_buf) - 1] = '\0';
}

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
  cheese_set_clipboard(&cheese, clip_get, clip_set, null);

  cheese_begin(&cheese, arena, null, &g_renderer,
               (cheese_input_t){.window_w = 800, .window_h = 600}, 0.016f);

  char buf[64];
  u32 len;
  cheese_text_input_t st;

  strcpy(buf, "hello world");
  len = strlen(buf);
  st = (cheese_text_input_t){.caret = 5, .anchor = 0};
  assert(cheese_text_edit_copy(&cheese, &st, buf) && "copy selection");
  assert(strcmp(clip_buf, "hello") == 0);

  st = (cheese_text_input_t){.caret = 2, .anchor = 2};
  assert(!cheese_text_edit_copy(&cheese, &st, buf) && "no selection, no copy");

  strcpy(buf, "hello world");
  len = strlen(buf);
  st = (cheese_text_input_t){.caret = 5, .anchor = 0};
  assert(cheese_text_edit_cut(&cheese, &st, buf, sizeof(buf), &len));
  assert(strcmp(buf, " world") == 0 && "cut removes the selection");
  assert(strcmp(clip_buf, "hello") == 0);
  assert(st.caret == 0 && st.anchor == 0);

  strcpy(clip_buf, "XYZ");
  strcpy(buf, "ab");
  len = strlen(buf);
  st = (cheese_text_input_t){.caret = 1, .anchor = 1};
  assert(cheese_text_edit_paste(&cheese, &st, buf, sizeof(buf), &len, true));
  assert(strcmp(buf, "aXYZb") == 0 && "paste inserts at the caret");
  assert(st.caret == 4);

  strcpy(clip_buf, "1\n2");
  strcpy(buf, "abc");
  len = strlen(buf);
  st = (cheese_text_input_t){.caret = 3, .anchor = 0};
  assert(cheese_text_edit_paste(&cheese, &st, buf, sizeof(buf), &len, false));
  assert(strcmp(buf, "12") == 0 && "newlines stripped when single-line");

  cheese_end(&cheese);
  arena_free(arena);
  return 0;
}
