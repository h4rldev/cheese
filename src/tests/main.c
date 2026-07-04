#include <unistd.h>

#include <bread/event.h>
#include <bread/input.h>
#include <bread/types.h>
#include <bread/window.h>

#include <butter/render.h>
#include <butter/texture.h>
#include <butter/types.h>

#include <cheese/container.h>
#include <cheese/draw.h>
#include <cheese/init.h>
#include <cheese/layout.h>
#include <cheese/log.h>
#include <cheese/style.h>
#include <cheese/types.h>
#include <cheese/widgets.h>

#include <renderers/butter.h>

#include <htils/arena.h>
#include <htils/string.h>

typedef struct {
  bread_window_t *window;
  butter_t *butter;
  arena_t *arena;

  cheese_renderer_t renderer;
  cheese_t cheese;
  cheese_font_t *font;

  f32 mouse_x, mouse_y;
  u32 mouse_buttons;
  f32 scroll_h, scroll_v;
  u32 key_mods;
  f32 delta_time;
} app_state_t;

static void event_callback(bread_event_t *event, void *userdata) {
  app_state_t *state = (app_state_t *)userdata;

  switch (event->type) {
  case BREAD_EVENT_MOUSE_MOVE:
    state->mouse_x = event->data.mouse_move.x;
    state->mouse_y = event->data.mouse_move.y;
    break;
  case BREAD_EVENT_MOUSE_PRESS: {
    u32 mask = 0;
    switch (event->data.mouse_button.button) {
    case BREAD_MOUSE_BUTTON_LEFT:
      mask = CHEESE_MOUSE_LEFT;
      break;
    case BREAD_MOUSE_BUTTON_RIGHT:
      mask = CHEESE_MOUSE_RIGHT;
      break;
    case BREAD_MOUSE_BUTTON_MIDDLE:
      mask = CHEESE_MOUSE_MIDDLE;
      break;
    default:
      break;
    }
    state->mouse_buttons |= mask;
  } break;

  case BREAD_EVENT_MOUSE_RELEASE: {
    u32 mask = 0;
    switch (event->data.mouse_button.button) {
    case BREAD_MOUSE_BUTTON_LEFT:
      mask = CHEESE_MOUSE_LEFT;
      break;
    case BREAD_MOUSE_BUTTON_RIGHT:
      mask = CHEESE_MOUSE_RIGHT;
      break;
    case BREAD_MOUSE_BUTTON_MIDDLE:
      mask = CHEESE_MOUSE_MIDDLE;
      break;
    default:
      break;
    }
    state->mouse_buttons &= ~mask;
  } break;

  case BREAD_EVENT_MOUSE_SCROLL: {
    state->scroll_h += event->data.mouse_scroll.dx;
    state->scroll_v += event->data.mouse_scroll.dy;
  } break;

  case BREAD_EVENT_WINDOW_RESIZE:
    butter_set_pending_resize(state->butter, event->data.resize.width,
                              event->data.resize.height);
    break;

  default:
    break;
  }
}

static void cursor_callback(void *userdata, cheese_cursor_t cursor) {
  app_state_t *state = (app_state_t *)userdata;
  bread_window_t *window = state->window;

  cheese_log_debug("Cursor callback: %d", cursor);

  switch (cursor) {
  case CHEESE_CURSOR_DEFAULT:
    bread_set_cursor(window, BREAD_CURSOR_DEFAULT);
    break;
  case CHEESE_CURSOR_POINTER:
    bread_set_cursor(window, BREAD_CURSOR_POINTER);
    break;
  default:
    break;
  }
}

static void render_callback(vk_command_buffer_t cmd,
                            const butter_frame_t *frame, void *userdata) {
  app_state_t *state = (app_state_t *)userdata;

  cheese_begin(&state->cheese, &state->renderer, state->mouse_x, state->mouse_y,
               state->mouse_buttons, state->scroll_h, state->scroll_v,
               state->key_mods, state->delta_time);

  cheese_draw_texture(&state->cheese, 500, 100, 100, 100, 0,
                      cheese_color_rgba(255, 255, 255, 255));

  /*cheese_draw_texture(&state->cheese, 100, 400, 512, 512, 1,
                      cheese_color_rgba(0, 0, 0, 255));*/

  cheese_style_t container_style = cheese_default_style();
  container_style.bg_color = cheese_color_rgba(30, 30, 56, 255);
  container_style.border_color = cheese_color_rgba(24, 24, 37, 255);
  container_style.text_color = cheese_color_rgba(255, 255, 255, 255);
  container_style.border_width = 1.0f;
  container_style.uniform_padding = true;
  container_style.padding.uniform = 30.0f;
  cheese_push_style(&state->cheese, container_style);

  cheese_begin_container_auto(&state->cheese, 600, 500);
  {
    string *label = string_from_cstr(state->arena, "Meow!");
    if (cheese_button_auto(&state->cheese, label, state->font)) {
      cheese_log_info("Meow :3");
    }

    cheese_style_set_font_size(&container_style, 40);

    string *label2 = string_from_cstr(state->arena, "Woof!");
    if (cheese_button_auto(&state->cheese, label2, state->font)) {
      cheese_log_info("Woof :DD");
    }
  }
  cheese_end_container(&state->cheese);

  cheese_draw_texture(&state->cheese, 700, 500, 1024, 1024,
                      state->font->active_variant->atlas_texture_id,
                      cheese_color_rgba(0, 0, 0, 255));

  cheese_pop_style(&state->cheese);
  cheese_end(&state->cheese);
}

int main(void) {
  arena_t *perm_arena = arena_new(GiB(1), MiB(16));
  arena_t *frame_arena = arena_new(GiB(1), MiB(16));

  bread_window_t window = {
      .width = 800,
      .height = 600,
      .arena = perm_arena,
  };

  bread_window_init(&window);

  bread_backend_type_t bread_backend = bread_get_backend_type();
  butter_backend_t butter_backend = (bread_backend == BREAD_BACKEND_X11)
                                        ? BUTTER_BACKEND_XCB
                                        : BUTTER_BACKEND_WAYLAND;
  bread_surface_t surface = bread_window_get_surface(&window);
  butter_surface_info_t surface_info = {
      .backend = butter_backend,
      .handle = surface.handle,
      .display = surface.display,
  };

  butter_t *butter = butter_init(perm_arena, &surface_info, "Cheese Test :3",
                                 true, window.width, window.height);
  if (!butter) {
    cheese_log_error("Failed to initialize butter");
    bread_window_destroy(&window);
    return 1;
  }

  butter_set_clear_color(butter, 255, 255, 255, 255);
  butter_set_vsync(butter, true);
  butter_init_texture_upload(butter, 16);

  bread_cursor_init(&window);

  cheese_renderer_t renderer =
      cheese_create_butter_renderer(butter, perm_arena);
  if (!renderer.draw_rect || !renderer.draw_texture || !renderer.draw_line ||
      !renderer.draw_arc || !renderer.draw_text || !renderer.push_clip ||
      !renderer.pop_clip || !renderer.create_texture ||
      !renderer.delete_texture) {
    butter_end(butter);
    bread_window_destroy(&window);
    cheese_log_error("Failed to create renderer, or its incomplete");
    return 1;
  }

  if (!cheese_font_system_init()) {
    cheese_log_error("Failed to initialize font system");
    butter_end(butter);
    bread_window_destroy(&window);
    return 1;
  }

  const cstr *font_path =
      "/run/current-system/sw/share/X11/fonts/MapleMono-Regular.ttf";
  if (access(font_path, R_OK) != 0) {
    cheese_log_error("Failed to access font path '%s'", font_path);
    butter_end(butter);
    bread_window_destroy(&window);
    cheese_font_system_destroy();
    return 1;
  }

  arena_t *font_arena = arena_new(GiB(1), MiB(16));
  const string *font_path_str = string_from_cstr(font_arena, font_path);
  cheese_log_debug("Loading font with path '%s'", font_path);
  cheese_font_t *font =
      cheese_load_font(&renderer, font_arena, font_path_str, 20);
  if (!font) {
    cheese_log_error("Failed to load font with the path '%s'", font_path);
  }

  cheese_t cheese = cheese_default();

  app_state_t state = {0};
  state.cheese = cheese;
  state.window = &window;
  state.butter = butter;
  state.renderer = renderer;
  state.arena = perm_arena;
  state.font = font;
  state.mouse_x = 0;
  state.mouse_y = 0;
  state.mouse_buttons = 0;
  state.scroll_h = 0;
  state.scroll_v = 0;
  state.key_mods = 0;
  state.delta_time = 0;

  cheese_set_cursor_callback(&state.cheese, cursor_callback, &state);
  butter_set_draw_callback(butter, render_callback, &state);
  bread_window_set_event_callback(&window, event_callback, &state);
  bread_window_set_min_size(&window, 400, 300);

  butter_start_render_thread(butter, frame_arena);

  while (!bread_window_should_close(&window)) {
    bread_window_poll(&window);

    butter_request_frame(butter);

    arena_clear(frame_arena);
  }

  bread_cursor_cleanup(&window);
  butter_stop_render_thread(butter);
  butter_stop_texture_uploads(butter);

  vkDeviceWaitIdle(butter->device);

  cheese_font_destroy(&renderer, font);
  cheese_font_system_destroy();
  cheese_destroy_butter_renderer(&renderer);

  butter_end(butter);
  bread_window_destroy(&window);

  arena_free(frame_arena);
  arena_free(perm_arena);
  arena_free(font_arena);

  return 0;
}
