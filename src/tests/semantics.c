/***********************************/

#include <assert.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/semantics.h>

/***********************************/

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = {0};
  cheese.frame_arena = frame;

  cheese_semantics_begin(&cheese);

  cheese_semantics_begin_node(&cheese, (cheese_semantics_t){.key = "sidebar"},
                              CHEESE_ROLE_CONTAINER, "Sidebar",
                              (cheese_rect_t){0, 0, 200, 600});

  cheese_semantics_emit(&cheese, (cheese_semantics_t){.key = "shuffle"},
                        CHEESE_ROLE_CHECKBOX, "Shuffle", null,
                        CHEESE_STATE_CHECKED | CHEESE_STATE_HOVERED,
                        (cheese_rect_t){8, 8, 120, 24});

  cheese_semantics_end_node(&cheese);

  cheese_semantics_node_t *checkbox = cheese_semantics_find(&cheese, "shuffle");
  assert(checkbox != null && "checkbox keyed 'shuffle' must be in the tree");
  assert(checkbox->role == CHEESE_ROLE_CHECKBOX);
  assert(checkbox->state & CHEESE_STATE_CHECKED);
  assert((checkbox->state & CHEESE_STATE_HOVERED) &&
         "hover state must live on the node");
  assert(strcmp(checkbox->name, "Shuffle") == 0);

  cheese_semantics_node_t *sidebar = cheese_semantics_find(&cheese, "sidebar");
  assert(sidebar != null);
  assert(sidebar->role == CHEESE_ROLE_CONTAINER);
  assert(sidebar->first_child >= 0);

  cheese_semantics_node_t *child =
      cheese_semantics_first_child(&cheese, sidebar);
  assert(child == checkbox && "the checkbox must be a child of the sidebar");

  assert(cheese_semantics_find(&cheese, "nope") == null);

  arena_free(frame);
  return 0;
}
