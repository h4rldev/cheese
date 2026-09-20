/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>

#include <cheese/widgets/widget.h>

/***********************************/

static void stub(void *userdata) { (void)userdata; }

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
};

static cheese_input_t g_input;
static cheese_widget_t g_scope;
static b32 g_activated;

static void frame(cheese_t *cheese, arena_t *arena) {
  cheese_begin(cheese, arena, null, &g_renderer, g_input, 0.016f);
  cheese_widget_begin(cheese, (cheese_semantics_t){.key = "w"},
                      CHEESE_ROLE_BUTTON, 0.0f, 0.0f, 100.0f, 40.0f, &g_scope);
  g_activated = cheese_widget_activated(cheese, &g_scope);
  cheese_end(cheese);
}

//
//
//

int main(void) {
  arena_t *arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(arena);

  g_input = (cheese_input_t){.mouse_x = 50.0f, .mouse_y = 20.0f};
  frame(&cheese, arena);
  assert(g_scope.hovered && (g_scope.state & CHEESE_STATE_HOVERED) &&
         "the scope reports hover");
  assert(!g_activated && "hover alone does not activate");

  g_input = (cheese_input_t){.mouse_x = 500.0f, .mouse_y = 500.0f};
  frame(&cheese, arena);
  assert(!g_scope.hovered && !(g_scope.state & CHEESE_STATE_HOVERED) &&
         "the scope reports no hover outside");

  g_input = (cheese_input_t){
      .mouse_x = 50.0f, .mouse_y = 20.0f, .mouse_buttons = CHEESE_MOUSE_LEFT};
  frame(&cheese, arena);
  assert(g_activated && g_scope.pressed && "a left click activates");

  g_input = (cheese_input_t){.mouse_x = 50.0f, .mouse_y = 20.0f};
  frame(&cheese, arena);

  u64 id = g_scope.id;
  cheese.focus_id = id;

  cheese_input_t enter = {.mouse_x = 500.0f, .mouse_y = 500.0f};
  enter.key_event_count = 1;
  enter.key_events[0] = (cheese_key_event_t){.key = CHEESE_KEY_ENTER};
  g_input = enter;
  frame(&cheese, arena);
  assert(g_scope.focused && (g_scope.state & CHEESE_STATE_FOCUSED) &&
         "the scope reports focus");
  assert(g_activated && "enter activates the focused scope");

  arena_free(arena);
  return 0;
}
