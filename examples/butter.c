/***********************************/

#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#include <bread/event.h>
#include <bread/input.h>
#include <bread/types.h>
#include <bread/window.h>

#include <butter/render.h>
#include <butter/texture.h>
#include <butter/types.h>

#include <cheese/debug.h>
#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/app.h>
#include <cheese/core/init.h>
#include <cheese/core/state.h>
#include <cheese/core/theme.h>

#include <cheese/render/font.h>

#include <renderers/butter.h>

#include "gallery.h"

#include <htils/arena.h>
#include <htils/string.h>

/***********************************/

#define use_validation false

typedef struct {
  bread_window_t *window;
  butter_t *butter;
  arena_t *arena;
  arena_t *frame_arena;

  cheese_renderer_t renderer;
  cheese_t cheese;
  cheese_app_t app;
  cheese_font_t *font;

  f32 mouse_x, mouse_y;
  u32 mouse_buttons;
  f32 scroll_h, scroll_v;
  u32 key_mods;
  f32 delta_time;
  atomic_i32 cursor_request;
  i32 cursor_applied;

  f64 last_cpu_s;
  f64 last_wall_s;
  f64 last_metrics_s;
  cheese_debug_metrics_t metrics;

  cheese_key_event_t key_events[CHEESE_MAX_KEY_EVENTS];
  u32 key_event_count;

  example_gallery_t gallery;
} app_state_t;

static void print_rss(const char *tag) {
  FILE *f = fopen("/proc/self/smaps_rollup", "r");
  if (!f) {
    cheese_log_warning("Could not open smaps_rollup");
    return;
  }
  char line[128];
  unsigned long rss = 0, pss = 0;
  while (fgets(line, sizeof line, f)) {
    if (sscanf(line, "Rss: %lu kB", &rss) == 1)
      continue;
    if (sscanf(line, "Pss: %lu kB", &pss) == 1)
      continue;
  }
  fclose(f);
  cheese_log_info("\n\n%s: Rss=%.2f MiB Pss=%.2f MiB\n\n", tag, rss / 1024.0,
                  pss / 1024.0);
}

//
//
//

static __attribute__((unused)) void dump_tree(cheese_t *cheese) {
  for (u64 i = 0; i < cheese_semantics_count(cheese); i++) {
    cheese_semantics_node_t *n = cheese_semantics_at(cheese, i);
    cheese_log_info("%*s%s %s  [%d,%d %ux%u] state=0x%x",
                    (int)(n->parent >= 0 ? 2 : 0), "",
                    cheese_semantics_role_name(n->role),
                    n->name ? n->name : (n->key ? n->key : ""), n->bounds.x,
                    n->bounds.y, n->bounds.w, n->bounds.h, n->state);
  }
}

//
//
//

static cheese_key_t cheese_key_from_bread(bread_key_t key) {
  switch (key) {
  case BREAD_KEY_TAB:
    return CHEESE_KEY_TAB;
  case BREAD_KEY_ENTER:
  case BREAD_KEY_KP_ENTER:
    return CHEESE_KEY_ENTER;
  case BREAD_KEY_SPACE:
    return CHEESE_KEY_SPACE;
  case BREAD_KEY_ESCAPE:
    return CHEESE_KEY_ESCAPE;
  case BREAD_KEY_BACKSPACE:
    return CHEESE_KEY_BACKSPACE;
  case BREAD_KEY_DELETE:
    return CHEESE_KEY_DELETE;
  case BREAD_KEY_LEFT:
    return CHEESE_KEY_LEFT;
  case BREAD_KEY_RIGHT:
    return CHEESE_KEY_RIGHT;
  case BREAD_KEY_UP:
    return CHEESE_KEY_UP;
  case BREAD_KEY_DOWN:
    return CHEESE_KEY_DOWN;
  case BREAD_KEY_HOME:
    return CHEESE_KEY_HOME;
  case BREAD_KEY_END:
    return CHEESE_KEY_END;
  case BREAD_KEY_PAGE_UP:
    return CHEESE_KEY_PAGE_UP;
  case BREAD_KEY_PAGE_DOWN:
    return CHEESE_KEY_PAGE_DOWN;
  default:
    return CHEESE_KEY_UNKNOWN;
  }
}

//
//
//

static void push_key_event(app_state_t *state, cheese_key_t key, u32 codepoint,
                           u32 mods) {
  if (state->key_event_count >= CHEESE_MAX_KEY_EVENTS)
    return;
  state->key_events[state->key_event_count++] =
      (cheese_key_event_t){key, codepoint, mods, false};
}

//
//
//

static void event_callback(bread_event_t *event, void *userdata) {
  app_state_t *state = (app_state_t *)userdata;
  switch (event->type) {
  case BREAD_EVENT_KEY_PRESS: {
    bread_key_t bk = event->data.key.key;
    if (bk == BREAD_KEY_LEFT_SHIFT || bk == BREAD_KEY_RIGHT_SHIFT)
      state->key_mods |= CHEESE_MOD_SHIFT;
    else if (bk == BREAD_KEY_LEFT_CTRL || bk == BREAD_KEY_RIGHT_CTRL)
      state->key_mods |= CHEESE_MOD_CTRL;
    else if (bk == BREAD_KEY_LEFT_ALT || bk == BREAD_KEY_RIGHT_ALT)
      state->key_mods |= CHEESE_MOD_ALT;
    else if (bk == BREAD_KEY_LEFT_SUPER || bk == BREAD_KEY_RIGHT_SUPER)
      state->key_mods |= CHEESE_MOD_SUPER;

    cheese_key_t ck = cheese_key_from_bread(bk);
    u32 cp = bread_event_key_to_unicode(state->window, event);
    if (ck == CHEESE_KEY_SPACE)
      push_key_event(state, ck, ' ', state->key_mods);
    else if (ck != CHEESE_KEY_UNKNOWN)
      push_key_event(state, ck, 0, state->key_mods);

    if (cp && cp != ' ' && cp >= 32 && cp != 127)
      push_key_event(state, CHEESE_KEY_UNKNOWN, cp, state->key_mods);
  } break;

  case BREAD_EVENT_KEY_RELEASE: {
    bread_key_t bk = event->data.key.key;
    if (bk == BREAD_KEY_LEFT_SHIFT || bk == BREAD_KEY_RIGHT_SHIFT)
      state->key_mods &= ~CHEESE_MOD_SHIFT;
    else if (bk == BREAD_KEY_LEFT_CTRL || bk == BREAD_KEY_RIGHT_CTRL)
      state->key_mods &= ~CHEESE_MOD_CTRL;
    else if (bk == BREAD_KEY_LEFT_ALT || bk == BREAD_KEY_RIGHT_ALT)
      state->key_mods &= ~CHEESE_MOD_ALT;
    else if (bk == BREAD_KEY_LEFT_SUPER || bk == BREAD_KEY_RIGHT_SUPER)
      state->key_mods &= ~CHEESE_MOD_SUPER;
  } break;

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

  case BREAD_EVENT_MOUSE_SCROLL:
    state->scroll_h += event->data.mouse_scroll.dx;
    state->scroll_v += event->data.mouse_scroll.dy;
    break;

  case BREAD_EVENT_WINDOW_RESIZE:
    butter_set_pending_resize(state->butter, event->data.resize.width,
                              event->data.resize.height);
    break;

  default:
    break;
  }
}

//
//
//

static cheese_input_t build_input(app_state_t *state) {
  cheese_input_t input = {
      .mouse_x = state->mouse_x,
      .mouse_y = state->mouse_y,
      .mouse_buttons = state->mouse_buttons,
      .scroll_x = state->scroll_h,
      .scroll_y = state->scroll_v,
      .key_mods = state->key_mods,
  };

  for (u32 i = 0; i < state->key_event_count; i++)
    input.key_events[i] = state->key_events[i];
  input.key_event_count = state->key_event_count;

  return input;
}

//
//
//

static void cursor_callback(void *userdata, cheese_cursor_t cursor) {
  app_state_t *state = (app_state_t *)userdata;
  atomic_store(&state->cursor_request, (i32)cursor);
}

//
// App harness callbacks
//

static cheese_app_result_t app_start(cheese_t *cheese, void *userdata) {
  app_state_t *state = (app_state_t *)userdata;
  gallery_init(&state->gallery, cheese->store, state->arena, state->font);
  return CHEESE_CONTINUE;
}

//
//
//

static cheese_app_result_t app_update(cheese_t *cheese, f32 dt,
                                      void *userdata) {
  (void)cheese;
  (void)dt;
  (void)userdata;
  return CHEESE_CONTINUE;
}

//
//
//

static void gather_metrics(app_state_t *state, cheese_debug_metrics_t *m) {
  memset(m, 0, sizeof(*m));

  FILE *f = fopen("/proc/self/smaps_rollup", "r");
  if (f) {
    char line[128];
    unsigned long rss = 0, pss = 0;
    while (fgets(line, sizeof(line), f)) {
      if (sscanf(line, "Rss: %lu kB", &rss) == 1)
        continue;
      if (sscanf(line, "Pss: %lu kB", &pss) == 1)
        continue;
    }
    fclose(f);
    m->rss_mib = (f32)rss / 1024.0f;
    m->pss_mib = (f32)pss / 1024.0f;
  }

  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) {
    f64 cpu = (f64)ru.ru_utime.tv_sec + (f64)ru.ru_utime.tv_usec / 1e6 +
              (f64)ru.ru_stime.tv_sec + (f64)ru.ru_stime.tv_usec / 1e6;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    f64 wall = (f64)now.tv_sec + (f64)now.tv_nsec / 1e9;
    if (state->last_wall_s > 0.0 && wall > state->last_wall_s)
      m->cpu_pct = (f32)((cpu - state->last_cpu_s) /
                         (wall - state->last_wall_s) * 100.0);
    state->last_cpu_s = cpu;
    state->last_wall_s = wall;
  }

  butter_stats_t stats = {0};
  if (butter_get_stats(state->butter, &stats)) {
    m->frame_ms = stats.cpu_frame_ms;
    m->fps = stats.frame_rate;
    m->gpu_pct = stats.gpu_usage_pct;
    m->vram_used_mib = (f32)stats.vram_used / (1024.0f * 1024.0f);
    m->vram_total_mib = (f32)stats.vram_total / (1024.0f * 1024.0f);
  }
}

//
//
//

static void app_render(cheese_t *cheese, void *userdata) {
  app_state_t *state = (app_state_t *)userdata;

  gallery_tick(&state->gallery, cheese->delta_time);
  gallery_draw(cheese, &state->gallery);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  f64 now_s = (f64)now.tv_sec + (f64)now.tv_nsec / 1e9;
  if (now_s - state->last_metrics_s >= 0.2) {
    state->last_metrics_s = now_s;
    gather_metrics(state, &state->metrics);
  }
  cheese_debug_monitor(cheese, state->font, .0f, 8.0f, &state->metrics);
}

//
//
//

static void app_stop(cheese_t *cheese, void *userdata) {
  (void)cheese;
  (void)userdata;
}

//
//
//

static void render_callback(vk_command_buffer_t cmd,
                            const butter_frame_t *frame, void *userdata) {
  (void)cmd;
  app_state_t *state = (app_state_t *)userdata;

  cheese_input_t input = build_input(state);
  input.window_w = (f32)frame->extent.width;
  input.window_h = (f32)frame->extent.height;

  state->key_event_count = 0;
  state->scroll_h = 0.0f;
  state->scroll_v = 0.0f;

  cheese_begin(&state->cheese, state->frame_arena, state->font,
               &state->renderer, input, state->delta_time);

  cheese_app_render(&state->cheese, &state->app);

  // dump_tree(&state->cheese);
  cheese_end(&state->cheese);
}

//
//
//

int main(void) {
  arena_t *perm_arena = arena_new(GiB(1), MiB(1));
  arena_t *frame_arena = arena_new(GiB(1), MiB(1));
  print_rss("perm + frame arena");

  bread_window_t window = {
      .width = 800,
      .height = 600,
      .arena = perm_arena,
  };

  bread_window_init(&window);
  print_rss("bread window init");

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

  butter_init_config_t config = butter_init_config_default();
  config.app_name = "Cheese Test :3";
  config.use_validation_layers = use_validation;
  config.width = window.width;
  config.height = window.height;
  config.aa_mode = BUTTER_AA_MSAA;
  config.aa_samples = 8;
  config.max_render_width = 3840;
  config.max_render_height = 2160;

  butter_t *butter = butter_init(perm_arena, &surface_info, &config);
  print_rss("butter");
  if (!butter) {
    cheese_log_error("Failed to initialize butter");
    bread_window_destroy(&window);
    return 1;
  }

  butter_set_clear_color(butter, 0, 0, 0, 1);
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

  arena_t *font_arena = arena_new(GiB(1), MiB(1));
  const string *font_path_str = string_from_cstr(font_arena, font_path);
  cheese_log_debug("Loading font with path '%s'", font_path);
  cheese_font_t *font =
      cheese_load_font(&renderer, font_arena, font_path_str, 20);
  print_rss("font");

  if (!font) {
    cheese_log_error("Failed to load font with the path '%s'", font_path);
  }

  cheese_t cheese = cheese_default(perm_arena);

  cheese_theme_t theme = cheese_theme_dark();
  cheese_theme_apply(&cheese, &theme);

  app_state_t state = {0};
  state.cheese = cheese;
  state.window = &window;
  state.butter = butter;
  state.renderer = renderer;
  state.arena = perm_arena;
  state.frame_arena = frame_arena;
  state.font = font;
  state.app = (cheese_app_t){
      .userdata = &state,
      .start = app_start,
      .update = app_update,
      .render = app_render,
      .stop = app_stop,
  };

  cheese_set_cursor_callback(&state.cheese, cursor_callback, &state);
  butter_set_draw_callback(butter, render_callback, &state);
  bread_window_set_event_callback(&window, event_callback, &state);
  bread_window_set_min_size(&window, 400, 300);
  state.cursor_applied = BREAD_CURSOR_DEFAULT;
  atomic_store(&state.cursor_request, BREAD_CURSOR_DEFAULT);

  cheese_app_start(&state.cheese, &state.app);

  butter_start_render_thread(butter, frame_arena);

  struct timespec last;
  clock_gettime(CLOCK_MONOTONIC, &last);

  b32 frame_in_flight = false;

  while (!bread_window_should_close(&window)) {
    bread_window_poll(&window);

    i32 want_cursor = atomic_load(&state.cursor_request);
    if (want_cursor != state.cursor_applied) {
      state.cursor_applied = want_cursor;
      bread_set_cursor(&window, (bread_cursor_type_t)want_cursor);
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    state.delta_time = (f32)(now.tv_sec - last.tv_sec) +
                       (f32)(now.tv_nsec - last.tv_nsec) / 1000000000.0f;
    last = now;

    cheese_app_update(&state.cheese, &state.app, state.delta_time);

    if (frame_in_flight && butter_frame_completed(butter)) {
      frame_in_flight = false;
      arena_clear(frame_arena);
    }

    cheese_input_t input = build_input(&state);
    input.window_w = (f32)state.window->width;
    input.window_h = (f32)state.window->height;
    cheese_input(&state.cheese, input);

    if (!frame_in_flight && cheese_needs_frame(&state.cheese)) {
      cheese_clear_frame_needed(&state.cheese);
      butter_request_frame(butter);
      frame_in_flight = true;
    }

    thrd_sleep(&(struct timespec){.tv_nsec = 1000000}, NULL);
  }

  cheese_app_stop(&state.cheese, &state.app);
  print_rss("cheese end");

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
