/***********************************/

#include <assert.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>

#include <cheese/widgets/text.h>

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

static b32 edit_key(cheese_t *cheese, arena_t *frame, cheese_state_t *text,
                    cheese_text_input_t *st,
                    cheese_text_input_variant_t variant, cheese_input_t input,
                    const cstr *key) {
  cheese_begin(cheese, frame, null, &g_renderer, input, 0.016f);
  b32 changed = cheese_text_input(
      cheese, null, (cheese_semantics_t){.key = key}, 0.0f, 0.0f, 200.0f, 24.0f,
      cheese_val_state(text), st, cheese_val_f32(0.0f), variant, null);
  cheese_end(cheese);
  return changed;
}

//
//
//

static b32 edit(cheese_t *cheese, arena_t *frame, cheese_state_t *text,
                cheese_text_input_t *st, cheese_text_input_variant_t variant,
                cheese_input_t input) {
  return edit_key(cheese, frame, text, st, variant, input, "t");
}

//
//
//

static cheese_input_t key_event(cheese_key_t key, u32 codepoint, u32 mods) {
  cheese_input_t in = {0};
  in.key_event_count = 1;
  in.key_events[0] =
      (cheese_key_event_t){.key = key, .codepoint = codepoint, .mods = mods};
  return in;
}

//
//
//

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(frame);
  cheese_state_t *text = cheese_state_str(cheese.store, "text", "abc");
  cheese_text_input_t st = {0};

  cheese_input_t click = {
      .mouse_x = 1.0f,
      .mouse_y = 1.0f,
      .mouse_buttons = CHEESE_MOUSE_LEFT,
  };
  edit(&cheese, frame, text, &st, CHEESE_TEXT_INPUT_LINE, click);

  edit(&cheese, frame, text, &st, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_UNKNOWN, 'X', 0));
  edit(&cheese, frame, text, &st, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_UNKNOWN, 'y', 0));
  assert(strcmp(cheese_state_get_str(text), "Xyabc") == 0 &&
         "typing inserts at the caret");

  edit(&cheese, frame, text, &st, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_BACKSPACE, 0, 0));
  assert(strcmp(cheese_state_get_str(text), "Xabc") == 0 &&
         "backspace deletes before the caret");

  edit(&cheese, frame, text, &st, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_DELETE, 0, 0));
  assert(strcmp(cheese_state_get_str(text), "Xbc") == 0 &&
         "delete removes at the caret");

  cheese_input_t select_then_type = {0};
  select_then_type.key_event_count = 2;
  select_then_type.key_events[0] = (cheese_key_event_t){
      .key = CHEESE_KEY_RIGHT, .codepoint = 0, .mods = CHEESE_MOD_SHIFT};
  select_then_type.key_events[1] =
      (cheese_key_event_t){.key = CHEESE_KEY_UNKNOWN, .codepoint = 'Z'};
  edit(&cheese, frame, text, &st, CHEESE_TEXT_INPUT_LINE, select_then_type);
  assert(strcmp(cheese_state_get_str(text), "XZc") == 0 &&
         "typing replaces the selection");

  cheese_state_t *mtext = cheese_state_str(cheese.store, "mtext", "a\nb");
  cheese_text_input_t mst = {0};
  edit(&cheese, frame, mtext, &mst, CHEESE_TEXT_INPUT_MULTILINE, click);
  edit(&cheese, frame, mtext, &mst, CHEESE_TEXT_INPUT_MULTILINE,
       key_event(CHEESE_KEY_DOWN, 0, 0));
  edit(&cheese, frame, mtext, &mst, CHEESE_TEXT_INPUT_MULTILINE,
       key_event(CHEESE_KEY_UNKNOWN, 'Q', 0));
  assert(strcmp(cheese_state_get_str(mtext), "a\nQb") == 0 &&
         "down moves to the next line");

  edit(&cheese, frame, mtext, &mst, CHEESE_TEXT_INPUT_MULTILINE,
       key_event(CHEESE_KEY_ENTER, 0, 0));
  assert(strcmp(cheese_state_get_str(mtext), "a\nQ\nb") == 0 &&
         "enter inserts a newline in multiline");

  cheese_state_t *stext = cheese_state_str(cheese.store, "stext", "hi");
  cheese_text_input_t sst = {0};
  edit(&cheese, frame, stext, &sst, CHEESE_TEXT_INPUT_LINE, click);
  edit(&cheese, frame, stext, &sst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_ENTER, 0, 0));
  assert(strcmp(cheese_state_get_str(stext), "hi") == 0 &&
         "single line ignores enter");

  cheese_state_t *etext = cheese_state_str(cheese.store, "etext", "hi");
  cheese_text_input_t est = {0};
  cheese_input_t away = {.mouse_x = 1000.0f, .mouse_y = 1000.0f};

  edit_key(&cheese, frame, etext, &est, CHEESE_TEXT_INPUT_LINE, away, "e");
  u64 eid = find_id(&cheese, "e");
  assert(eid != 0 && "the field emits a node");

  cheese.focus_id = eid;

  edit_key(&cheese, frame, etext, &est, CHEESE_TEXT_INPUT_LINE,
           key_event(CHEESE_KEY_UNKNOWN, 'X', 0), "e");
  assert(strcmp(cheese_state_get_str(etext), "hi") == 0 &&
         "a focused field ignores typing until activated");

  edit_key(&cheese, frame, etext, &est, CHEESE_TEXT_INPUT_LINE,
           key_event(CHEESE_KEY_ENTER, 0, 0), "e");
  assert(cheese.edit_id == eid && "enter starts editing");

  edit_key(&cheese, frame, etext, &est, CHEESE_TEXT_INPUT_LINE,
           key_event(CHEESE_KEY_UNKNOWN, 'X', 0), "e");
  assert(strcmp(cheese_state_get_str(etext), "Xhi") == 0 &&
         "an editing field accepts typing");

  edit_key(&cheese, frame, etext, &est, CHEESE_TEXT_INPUT_LINE,
           key_event(CHEESE_KEY_ESCAPE, 0, 0), "e");
  assert(cheese.edit_id == 0 && "escape leaves edit mode");

  cheese_state_t *wtext =
      cheese_state_str(cheese.store, "wtext", "hello world");
  cheese_text_input_t wst = {0};
  edit(&cheese, frame, wtext, &wst, CHEESE_TEXT_INPUT_LINE, click);

  edit(&cheese, frame, wtext, &wst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_RIGHT, 0, CHEESE_MOD_CTRL));
  edit(&cheese, frame, wtext, &wst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_UNKNOWN, 'X', 0));
  assert(strcmp(cheese_state_get_str(wtext), "helloX world") == 0 &&
         "ctrl+right stops at the end of the word");

  edit(&cheese, frame, wtext, &wst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_BACKSPACE, 0, CHEESE_MOD_CTRL));
  assert(strcmp(cheese_state_get_str(wtext), " world") == 0 &&
         "ctrl+backspace deletes the word before the caret");

  edit(&cheese, frame, wtext, &wst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_A, 'a', CHEESE_MOD_CTRL));
  edit(&cheese, frame, wtext, &wst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_UNKNOWN, 'Z', 0));
  assert(strcmp(cheese_state_get_str(wtext), "Z") == 0 &&
         "ctrl+a selects all and typing replaces it");

  cheese_state_t *utext = cheese_state_str(cheese.store, "utext", "");
  cheese_text_input_t ust = {0};
  arena_t *undo_arena = arena_new(MiB(1), KiB(64));
  ust.undo_arena = undo_arena;
  edit(&cheese, frame, utext, &ust, CHEESE_TEXT_INPUT_LINE, click);

  edit(&cheese, frame, utext, &ust, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_UNKNOWN, 'a', 0));
  edit(&cheese, frame, utext, &ust, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_UNKNOWN, 'b', 0));
  edit(&cheese, frame, utext, &ust, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_UNKNOWN, 'c', 0));
  assert(strcmp(cheese_state_get_str(utext), "abc") == 0 &&
         "typing builds the text");

  edit(&cheese, frame, utext, &ust, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_Z, 'z', CHEESE_MOD_CTRL));
  assert(strcmp(cheese_state_get_str(utext), "") == 0 &&
         "ctrl+z undoes the typing burst");

  edit(&cheese, frame, utext, &ust, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_Z, 'Z', CHEESE_MOD_CTRL | CHEESE_MOD_SHIFT));
  assert(strcmp(cheese_state_get_str(utext), "abc") == 0 &&
         "ctrl+shift+z redoes it");

  arena_free(undo_arena);

  cheese_state_t *rtext = cheese_state_str(cheese.store, "rtext", "abc");
  cheese_text_input_t rst = {0};
  rst.readonly = true;
  edit(&cheese, frame, rtext, &rst, CHEESE_TEXT_INPUT_LINE, click);

  edit(&cheese, frame, rtext, &rst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_UNKNOWN, 'X', 0));
  assert(strcmp(cheese_state_get_str(rtext), "abc") == 0 &&
         "a read-only field rejects typing");

  edit(&cheese, frame, rtext, &rst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_RIGHT, 0, 0));
  edit(&cheese, frame, rtext, &rst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_BACKSPACE, 0, 0));
  assert(strcmp(cheese_state_get_str(rtext), "abc") == 0 &&
         "a read-only field rejects backspace");

  edit(&cheese, frame, rtext, &rst, CHEESE_TEXT_INPUT_LINE,
       key_event(CHEESE_KEY_A, 'a', CHEESE_MOD_CTRL));
  assert(rst.anchor == 0 && rst.caret == 3 &&
         "a read-only field still allows select-all");

  arena_free(frame);
  return 0;
}
