/***********************************/

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

#include <cheese.h>

#include <renderers/sdl3.h>

#include "gallery.h"

/***********************************/

typedef struct {
  f32 wheel_x, wheel_y;
  cheese_key_event_t key_events[CHEESE_MAX_KEY_EVENTS];
  u32 key_event_count;

  SDL_Cursor *cursors[CHEESE_CURSOR_MAX];
  b32 cursor_owned[CHEESE_CURSOR_MAX];
} example_t;

//
//
//

static u32 cheese_mods(SDL_Keymod mod) {
  u32 out = 0;
  if (mod & SDL_KMOD_SHIFT)
    out |= CHEESE_MOD_SHIFT;
  if (mod & SDL_KMOD_CTRL)
    out |= CHEESE_MOD_CTRL;
  if (mod & SDL_KMOD_ALT)
    out |= CHEESE_MOD_ALT;
  if (mod & SDL_KMOD_GUI)
    out |= CHEESE_MOD_SUPER;
  return out;
}

//
//
//

static cheese_key_t cheese_key_from_sdl(SDL_Keycode key) {
  if (key >= SDLK_A && key <= SDLK_Z)
    return (cheese_key_t)(CHEESE_KEY_A + (key - SDLK_A));
  if (key >= SDLK_0 && key <= SDLK_9)
    return (cheese_key_t)(CHEESE_KEY_0 + (key - SDLK_0));

  switch (key) {
  case SDLK_TAB:
    return CHEESE_KEY_TAB;
  case SDLK_RETURN:
  case SDLK_KP_ENTER:
    return CHEESE_KEY_ENTER;
  case SDLK_SPACE:
    return CHEESE_KEY_SPACE;
  case SDLK_ESCAPE:
    return CHEESE_KEY_ESCAPE;
  case SDLK_BACKSPACE:
    return CHEESE_KEY_BACKSPACE;
  case SDLK_DELETE:
    return CHEESE_KEY_DELETE;
  case SDLK_LEFT:
    return CHEESE_KEY_LEFT;
  case SDLK_RIGHT:
    return CHEESE_KEY_RIGHT;
  case SDLK_UP:
    return CHEESE_KEY_UP;
  case SDLK_DOWN:
    return CHEESE_KEY_DOWN;
  case SDLK_HOME:
    return CHEESE_KEY_HOME;
  case SDLK_END:
    return CHEESE_KEY_END;
  case SDLK_PAGEUP:
    return CHEESE_KEY_PAGE_UP;
  case SDLK_PAGEDOWN:
    return CHEESE_KEY_PAGE_DOWN;
  default:
    return CHEESE_KEY_UNKNOWN;
  }
}

//
//
//

static void example_push_key(example_t *example, cheese_key_t key,
                             u32 codepoint, u32 mods) {
  if (example->key_event_count >= CHEESE_MAX_KEY_EVENTS)
    return;
  example->key_events[example->key_event_count++] =
      (cheese_key_event_t){key, codepoint, mods, false};
}

// Decodes the UTF-8 text SDL hands us into cheese codepoint events.
static void example_push_text(example_t *example, const char *text, u32 mods) {
  const unsigned char *p = (const unsigned char *)text;
  while (*p) {
    u32 cp = *p;
    u32 extra = 0;
    if (cp >= 0xF0) {
      cp &= 0x07;
      extra = 3;
    } else if (cp >= 0xE0) {
      cp &= 0x0F;
      extra = 2;
    } else if (cp >= 0xC0) {
      cp &= 0x1F;
      extra = 1;
    }
    p++;
    for (u32 i = 0; i < extra && (*p & 0xC0) == 0x80; i++, p++)
      cp = (cp << 6) | (*p & 0x3F);

    if (cp == ' ' || cp < 32 || cp == 127)
      continue; // space is handled by the key-down path
    example_push_key(example, CHEESE_KEY_UNKNOWN, cp, mods);
  }
}

//
//
//

static cheese_input_t example_input(example_t *example, SDL_Window *window) {
  cheese_input_t input = {0};

  SDL_MouseButtonFlags buttons =
      SDL_GetMouseState(&input.mouse_x, &input.mouse_y);
  if (buttons & SDL_BUTTON_LMASK)
    input.mouse_buttons |= CHEESE_MOUSE_LEFT;
  if (buttons & SDL_BUTTON_RMASK)
    input.mouse_buttons |= CHEESE_MOUSE_RIGHT;
  if (buttons & SDL_BUTTON_MMASK)
    input.mouse_buttons |= CHEESE_MOUSE_MIDDLE;

  i32 w = 0, h = 0;
  SDL_GetWindowSizeInPixels(window, &w, &h);
  input.window_w = (f32)w;
  input.window_h = (f32)h;

  input.scroll_x = example->wheel_x;
  input.scroll_y = example->wheel_y;
  input.key_mods = cheese_mods(SDL_GetModState());

  input.key_event_count = example->key_event_count;
  for (u32 i = 0; i < example->key_event_count; i++)
    input.key_events[i] = example->key_events[i];

  example->wheel_x = 0.0f;
  example->wheel_y = 0.0f;
  example->key_event_count = 0;

  return input;
}

//
//
//

static SDL_SystemCursor sdl_cursor_for(cheese_cursor_t cursor) {
  switch (cursor) {
  case CHEESE_CURSOR_POINTER:
  case CHEESE_CURSOR_HAND:
    return SDL_SYSTEM_CURSOR_POINTER;
  case CHEESE_CURSOR_TEXT:
    return SDL_SYSTEM_CURSOR_TEXT;
  case CHEESE_CURSOR_MOVE:
    return SDL_SYSTEM_CURSOR_MOVE;
  case CHEESE_CURSOR_RESIZE_EW:
    return SDL_SYSTEM_CURSOR_EW_RESIZE;
  case CHEESE_CURSOR_RESIZE_NS:
    return SDL_SYSTEM_CURSOR_NS_RESIZE;
  case CHEESE_CURSOR_RESIZE_NESW:
    return SDL_SYSTEM_CURSOR_NESW_RESIZE;
  case CHEESE_CURSOR_RESIZE_NWSE:
    return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
  case CHEESE_CURSOR_NOT_ALLOWED:
    return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
  case CHEESE_CURSOR_WAIT:
    return SDL_SYSTEM_CURSOR_WAIT;
  default:
    return SDL_SYSTEM_CURSOR_DEFAULT;
  }
}

// cheese emits DEFAULT at cheese_begin and the hovered widget emits its own
// cursor during draw, so applying each call immediately would set the cursor
// twice per frame (default -> hover) and the change lands late on Wayland.
// Record the frame's last request and apply it once, only when it changes.
// cheese emits the frame's cursor once, on change, so apply it directly.
static void example_cursor_callback(void *userdata, cheese_cursor_t cursor) {
  example_t *example = (example_t *)userdata;
  if (cursor < 0 || cursor >= CHEESE_CURSOR_MAX)
    return;

  SDL_Cursor *sdl_cursor = example->cursors[cursor];
  if (!sdl_cursor) {
    if (cursor == CHEESE_CURSOR_DEFAULT) {
      sdl_cursor = SDL_GetDefaultCursor(); // library-owned
    } else {
      sdl_cursor = SDL_CreateSystemCursor(sdl_cursor_for(cursor));
      example->cursor_owned[cursor] = true;
    }
    if (!sdl_cursor)
      sdl_cursor = SDL_GetDefaultCursor();
    example->cursors[cursor] = sdl_cursor;
  }

  SDL_SetCursor(sdl_cursor);
}

//
//
//

int main(int argc, char **argv) {
  // Prefer the native backend on a Wayland session. SDL's X11 backend ignores
  // a cursor set while it has no mouse focus and only re-applies it on the next
  // pointer event, so SDL_SetCursor appears to lag.
  if (!SDL_getenv("SDL_VIDEODRIVER"))
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");

  if (!SDL_Init(SDL_INIT_VIDEO))
    return 1;

  cheese_log_info("SDL3 video driver: %s", SDL_GetCurrentVideoDriver());

  // SDL's 2D renderer has no MSAA switch and its Vulkan backend never reads the
  // GL attributes, so prefer a GL driver to get 4x shape AA (the list falls
  // back to vulkan/software where GL is unavailable).
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl,opengles2,vulkan,software");
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  SDL_Window *window = null;
  SDL_Renderer *sdl = null;
  if (!SDL_CreateWindowAndRenderer("cheese + SDL3", 800, 600,
                                   SDL_WINDOW_RESIZABLE, &window, &sdl))
    return 1;

  SDL_SetWindowMinimumSize(window, 400, 300);
  SDL_StartTextInput(window);

  {
    int samples = 0;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples);
    cheese_log_info("SDL3 renderer: %s (%d samples)", SDL_GetRendererName(sdl),
                    samples);
  }

  arena_t *persistent = arena_new(GiB(1), MiB(1));
  arena_t *frame_arena = arena_new(GiB(1), MiB(1));
  arena_t *font_arena = arena_new(GiB(1), MiB(1));

  cheese_renderer_t renderer = cheese_create_sdl3_renderer(sdl, persistent);
  if (!renderer.draw_rect || !renderer.draw_text || !renderer.create_texture) {
    cheese_log_error("Failed to create the SDL3 renderer");
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

  cheese_theme_t theme = cheese_theme_dark();
  cheese_theme_apply(&cheese, &theme);

  example_t example = {0};

  cheese_set_cursor_callback(&cheese, example_cursor_callback, &example);

  example_gallery_t gallery;
  gallery_init(&gallery, cheese.store, persistent, font);

  u64 last = SDL_GetTicks();
  b32 running = true;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        running = false;
        break;
      case SDL_EVENT_KEY_DOWN: {
        u32 mods = cheese_mods(event.key.mod);
        if (event.key.key == SDLK_SPACE)
          example_push_key(&example, CHEESE_KEY_SPACE, ' ', mods);
        else
          example_push_key(&example, cheese_key_from_sdl(event.key.key), 0,
                           mods);
      } break;
      case SDL_EVENT_TEXT_INPUT:
        example_push_text(&example, event.text.text,
                          cheese_mods(SDL_GetModState()));
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        example.wheel_x += event.wheel.x;
        example.wheel_y += event.wheel.y;
        break;
      default:
        break;
      }
    }

    u64 now = SDL_GetTicks();
    f32 dt = (f32)(now - last) / 1000.0f;
    last = now;

    gallery_tick(&gallery, dt);

    SDL_SetRenderDrawColor(sdl, 24, 24, 37, 255);
    SDL_RenderClear(sdl);

    cheese_input_t input = example_input(&example, window);
    cheese_begin(&cheese, frame_arena, font, &renderer, input, dt);
    gallery_draw(&cheese, &gallery);
    cheese_end(&cheese);

    SDL_RenderPresent(sdl);
    arena_clear(frame_arena);
  }

  cheese_font_destroy(&renderer, font);
  cheese_font_system_destroy();

  for (u32 i = 0; i < CHEESE_CURSOR_MAX; i++)
    if (example.cursor_owned[i] && example.cursors[i])
      SDL_DestroyCursor(example.cursors[i]);

  SDL_DestroyRenderer(sdl);
  SDL_DestroyWindow(window);
  SDL_Quit();

  arena_free(font_arena);
  arena_free(frame_arena);
  arena_free(persistent);
  return 0;
}
