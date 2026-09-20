/***********************************/

#include <raylib.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

#include <cheese.h>

#include <renderers/raylib.h>

#include "gallery.h"

/***********************************/

static u32 current_mods(void) {
  u32 mods = 0;
  if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
    mods |= CHEESE_MOD_SHIFT;
  if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
    mods |= CHEESE_MOD_CTRL;
  if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))
    mods |= CHEESE_MOD_ALT;
  if (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER))
    mods |= CHEESE_MOD_SUPER;
  return mods;
}

//
//
//

static cheese_key_t key_from_raylib(i32 key) {
  if (key >= KEY_A && key <= KEY_Z)
    return (cheese_key_t)(CHEESE_KEY_A + (key - KEY_A));
  if (key >= KEY_ZERO && key <= KEY_NINE)
    return (cheese_key_t)(CHEESE_KEY_0 + (key - KEY_ZERO));

  switch (key) {
  case KEY_TAB:
    return CHEESE_KEY_TAB;
  case KEY_ENTER:
  case KEY_KP_ENTER:
    return CHEESE_KEY_ENTER;
  case KEY_ESCAPE:
    return CHEESE_KEY_ESCAPE;
  case KEY_BACKSPACE:
    return CHEESE_KEY_BACKSPACE;
  case KEY_DELETE:
    return CHEESE_KEY_DELETE;
  case KEY_LEFT:
    return CHEESE_KEY_LEFT;
  case KEY_RIGHT:
    return CHEESE_KEY_RIGHT;
  case KEY_UP:
    return CHEESE_KEY_UP;
  case KEY_DOWN:
    return CHEESE_KEY_DOWN;
  case KEY_HOME:
    return CHEESE_KEY_HOME;
  case KEY_END:
    return CHEESE_KEY_END;
  case KEY_PAGE_UP:
    return CHEESE_KEY_PAGE_UP;
  case KEY_PAGE_DOWN:
    return CHEESE_KEY_PAGE_DOWN;
  default:
    return CHEESE_KEY_UNKNOWN;
  }
}

//
//
//

static void push_event(cheese_input_t *input, cheese_key_t key, u32 codepoint,
                       u32 mods) {
  if (input->key_event_count >= CHEESE_MAX_KEY_EVENTS)
    return;
  input->key_events[input->key_event_count++] =
      (cheese_key_event_t){key, codepoint, mods, false};
}

//
//
//

static cheese_input_t build_input(void) {
  cheese_input_t input = {0};
  input.mouse_x = (f32)GetMouseX();
  input.mouse_y = (f32)GetMouseY();
  input.window_w = (f32)GetScreenWidth();
  input.window_h = (f32)GetScreenHeight();
  input.scroll_y = GetMouseWheelMove();
  input.key_mods = current_mods();

  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    input.mouse_buttons |= CHEESE_MOUSE_LEFT;
  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    input.mouse_buttons |= CHEESE_MOUSE_RIGHT;
  if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    input.mouse_buttons |= CHEESE_MOUSE_MIDDLE;

  i32 key;
  while ((key = GetKeyPressed()) != 0) {
    if (key == KEY_SPACE) {
      push_event(&input, CHEESE_KEY_SPACE, ' ', input.key_mods);
      continue;
    }
    push_event(&input, key_from_raylib(key), 0, input.key_mods);
  }

  i32 cp;
  while ((cp = GetCharPressed()) != 0) {
    if (cp == ' ' || cp < 32 || cp == 127)
      continue;
    push_event(&input, CHEESE_KEY_UNKNOWN, (u32)cp, input.key_mods);
  }

  return input;
}

//
//
//

static void cursor_callback(void *userdata, cheese_cursor_t cursor) {
  (void)userdata;
  i32 raylib_cursor = MOUSE_CURSOR_DEFAULT;
  switch (cursor) {
  case CHEESE_CURSOR_POINTER:
  case CHEESE_CURSOR_HAND:
    raylib_cursor = MOUSE_CURSOR_POINTING_HAND;
    break;
  case CHEESE_CURSOR_TEXT:
    raylib_cursor = MOUSE_CURSOR_IBEAM;
    break;
  case CHEESE_CURSOR_MOVE:
    raylib_cursor = MOUSE_CURSOR_RESIZE_ALL;
    break;
  case CHEESE_CURSOR_RESIZE_EW:
    raylib_cursor = MOUSE_CURSOR_RESIZE_EW;
    break;
  case CHEESE_CURSOR_RESIZE_NS:
    raylib_cursor = MOUSE_CURSOR_RESIZE_NS;
    break;
  case CHEESE_CURSOR_RESIZE_NESW:
    raylib_cursor = MOUSE_CURSOR_RESIZE_NESW;
    break;
  case CHEESE_CURSOR_RESIZE_NWSE:
    raylib_cursor = MOUSE_CURSOR_RESIZE_NWSE;
    break;
  case CHEESE_CURSOR_NOT_ALLOWED:
    raylib_cursor = MOUSE_CURSOR_NOT_ALLOWED;
    break;
  default:
    break;
  }
  SetMouseCursor(raylib_cursor);
}

//
//
//

int main(int argc, char **argv) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
  InitWindow(800, 600, "cheese + raylib");
  SetWindowMinSize(400, 300);

  arena_t *persistent = arena_new(GiB(1), MiB(1));
  arena_t *frame_arena = arena_new(GiB(1), MiB(1));
  arena_t *font_arena = arena_new(GiB(1), MiB(1));

  cheese_renderer_t renderer = cheese_create_raylib_renderer(persistent);
  if (!renderer.draw_rect || !renderer.draw_text || !renderer.create_texture) {
    cheese_log_error("Failed to create the raylib renderer");
    return 1;
  }

  if (!cheese_font_system_init()) {
    cheese_log_error("Failed to initialize the font system");
    return 1;
  }

  const cstr *font_path = argc > 1 ? argv[1]
                                   : "/run/current-system/sw/share/X11/fonts/"
                                     "MapleMono-Regular.ttf";
  cheese_font_t *font = cheese_load_font(
      &renderer, font_arena, string_from_cstr(font_arena, font_path), 20);
  if (!font) {
    cheese_log_error("Failed to load the font '%s'", font_path);
    return 1;
  }

  cheese_t cheese = cheese_default(persistent);
  cheese_set_cursor_callback(&cheese, cursor_callback, null);

  cheese_theme_t theme = cheese_theme_dark();
  cheese_theme_apply(&cheese, &theme);

  example_gallery_t gallery;
  gallery_init(&gallery, cheese.store, persistent, font);

  f32 last_time = (f32)GetTime();
  while (!WindowShouldClose()) {
    f32 now = (f32)GetTime();
    f32 dt = now - last_time;
    last_time = now;

    gallery_tick(&gallery, dt);

    BeginDrawing();
    ClearBackground((Color){24, 24, 37, 255});

    cheese_input_t input = build_input();
    cheese_begin(&cheese, frame_arena, font, &renderer, input, dt);
    gallery_draw(&cheese, &gallery);
    cheese_end(&cheese);

    EndDrawing();
    arena_clear(frame_arena);
  }

  cheese_font_destroy(&renderer, font);
  cheese_font_system_destroy();
  CloseWindow();

  arena_free(font_arena);
  arena_free(frame_arena);
  arena_free(persistent);
  return 0;
}
