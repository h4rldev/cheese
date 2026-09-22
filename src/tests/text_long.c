/***********************************/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/state.h>

#include <cheese/widgets/text.h>

/***********************************/

static char g_clip[8192];
static void clip_set(void *ud, const cstr *text) {
  (void)ud;
  strncpy(g_clip, text, sizeof(g_clip) - 1);
  g_clip[sizeof(g_clip) - 1] = '\0';
}

static const cstr *clip_get(void *ud) {
  (void)ud;
  return g_clip[0] ? g_clip : null;
}

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

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
};

static void run(cheese_t *cheese, arena_t *frame, cheese_state_t *text,
                cheese_text_input_t *st, cheese_input_t in) {
  cheese_begin(cheese, frame, null, &g_renderer, in, 0.016f);
  cheese_text_input(cheese, null, (cheese_semantics_t){.key = "t"}, 0.0f, 0.0f,
                    200.0f, 24.0f, cheese_val_state(text), st,
                    cheese_val_f32(0.0f), CHEESE_TEXT_INPUT_LINE, null);
  cheese_end(cheese);
}

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(frame);
  cheese_set_clipboard(&cheese, clip_get, clip_set, null);
  cheese_text_input_t st = {0};
  cheese_input_t click = {
      .mouse_x = 1.0f,
      .mouse_y = 1.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };

  char big[1025];
  for (int i = 0; i < 1024; i++)
    big[i] = 'x';
  big[1024] = '\0';

  cheese_state_t *text = cheese_state_str(cheese.store, "text", big);
  run(&cheese, frame, text, &st, click);

  cheese_input_t select_all = {0};
  select_all.key_event_count = 2;
  select_all.key_events[0] = (cheese_key_event_t){
      .key = CHEESE_KEY_A, .codepoint = 'a', .mods = CHEESE_MOD_CTRL};
  select_all.key_events[1] = (cheese_key_event_t){
      .key = CHEESE_KEY_C, .codepoint = 'c', .mods = CHEESE_MOD_CTRL};
  run(&cheese, frame, text, &st, select_all);

  fprintf(stderr, "copy len = %zu (expect 1024)\n", strlen(g_clip));
  assert(strlen(g_clip) == 1024 && "ctrl+a ctrl+c copies the whole buffer");

  strcpy(g_clip, big);
  cheese_state_set_str(text, "hi");
  cheese_text_input_t st2 = {0};
  run(&cheese, frame, text, &st2, click);
  cheese_input_t paste = {0};
  paste.key_event_count = 1;
  paste.key_events[0] = (cheese_key_event_t){
      .key = CHEESE_KEY_V, .codepoint = 'v', .mods = CHEESE_MOD_CTRL};
  run(&cheese, frame, text, &st2, paste);
  fprintf(stderr, "paste len = %zu (expect 1026)\n",
          strlen(cheese_state_get_str(text)));
  assert(strlen(cheese_state_get_str(text)) == 1026 &&
         "paste inserts the whole clipboard");

  arena_free(frame);
  return 0;
}
