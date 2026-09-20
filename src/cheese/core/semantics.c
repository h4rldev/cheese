/***********************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/darray.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/semantics.h>

/***********************************/

static u64 cheese_semantics_hash(const cstr *str) {
  u64 hash = 1469598103934665603ULL; // FNV-1a

  while (str && *str) {
    hash ^= (u8)*str++;
    hash *= 1099511628211ULL;
  }

  return hash;
}

//
//
//

static u64 cheese_semantics_resolve_id(const cheese_semantics_t *semantics,
                                       cheese_role_t role, u64 order) {
  if (semantics->key)
    return cheese_semantics_hash(semantics->key);

  if (semantics->name)
    return cheese_semantics_hash(semantics->name) ^ ((u64)role << 32);

  return ((u64)role << 40) ^ order;
}

//
//
//

static i32 cheese_semantics_link(cheese_t *cheese,
                                 cheese_semantics_node_t node) {
  da_append(cheese->frame_arena, cheese->semantics_nodes, node);

  i32 index = (i32)da_len(cheese->semantics_nodes) - 1;
  i32 parent = cheese->semantics_current_parent;

  if (parent < 0)
    return index;

  cheese->semantics_nodes[index].parent = parent;

  i32 *first = &cheese->semantics_nodes[parent].first_child;
  i32 *last = &cheese->semantics_nodes[parent].last_child;

  if (*first < 0)
    *first = index;
  else
    cheese->semantics_nodes[*last].next_sibling = index;

  *last = index;
  return index;
}

//
//
//

static const cstr *cheese_semantics_strdup(arena_t *arena, const cstr *src) {
  if (!src)
    return null;

  size_t len = strlen(src) + 1;
  cstr *copy = arena_alloc(arena, cstr, len);
  memcpy(copy, src, len);
  return copy;
}

//
//
//

void cheese_semantics_begin(cheese_t *cheese) {
  if (!cheese || !cheese->frame_arena) {
    cheese_log_error("cheese_semantics_begin: Invalid cheese or frame arena");
    return;
  }

  da_new(cheese->frame_arena, cheese->semantics_nodes, 32);

  cheese->semantics_order = 0;
  cheese->semantics_current_parent = -1;

  cheese_semantics_node_t root = {
      .id = 0,
      .role = CHEESE_ROLE_ROOT,
      .parent = -1,
      .first_child = -1,
      .last_child = -1,
      .next_sibling = -1,
      .bounds = {0, 0, 0, 0},
  };

  cheese_semantics_link(cheese, root);
  cheese->semantics_current_parent = 0;
}

u64 cheese_semantics_peek_id(cheese_t *cheese, cheese_semantics_t semantics,
                             cheese_role_t fallback_role) {
  if (!cheese)
    return 0;

  cheese_role_t role = semantics.role ? semantics.role : fallback_role;
  return cheese_semantics_resolve_id(&semantics, role, cheese->semantics_order);
}

i32 cheese_semantics_emit(cheese_t *cheese, cheese_semantics_t semantics,
                          cheese_role_t fallback_role,
                          const cstr *fallback_name, const cstr *value,
                          u32 state, cheese_rect_t bounds) {
  if (!cheese || !cheese->semantics_nodes) {
    cheese_log_error("cheese_semantics_emit: No active semantics tree");
    return -1;
  }

  cheese_role_t role = semantics.role ? semantics.role : fallback_role;
  const cstr *name = semantics.name ? semantics.name : fallback_name;

  u64 id =
      cheese_semantics_resolve_id(&semantics, role, cheese->semantics_order++);

  u32 node_state = state;
  if (cheese->focus_id && id == cheese->focus_id)
    node_state |= CHEESE_STATE_FOCUSED;

  if (cheese->active_id && id == cheese->active_id)
    node_state |= CHEESE_STATE_ACTIVE;

  cheese_semantics_node_t node = {
      .id = id,
      .role = role,
      .state = node_state,
      .parent = -1,
      .first_child = -1,
      .last_child = -1,
      .next_sibling = -1,
      .bounds = bounds,
      .key = semantics.key,
      .name = name,
      .value = value,
      .description = semantics.description,
  };

  return cheese_semantics_link(cheese, node);
}

i32 cheese_semantics_begin_node(cheese_t *cheese, cheese_semantics_t semantics,
                                cheese_role_t fallback_role,
                                const cstr *fallback_name,
                                cheese_rect_t bounds) {
  i32 index = cheese_semantics_emit(cheese, semantics, fallback_role,
                                    fallback_name, null, 0, bounds);
  if (index >= 0)
    cheese->semantics_current_parent = index;

  return index;
}

void cheese_semantics_end_node(cheese_t *cheese) {
  if (!cheese || !cheese->semantics_nodes)
    return;

  i32 parent = cheese->semantics_current_parent;
  if (parent >= 0)
    cheese->semantics_current_parent = cheese->semantics_nodes[parent].parent;
}

const cstr *cheese_semantics_role_name(cheese_role_t role) {
  switch (role) {
  case CHEESE_ROLE_ROOT:
    return "root";
  case CHEESE_ROLE_CONTAINER:
    return "container";
  case CHEESE_ROLE_BUTTON:
    return "button";
  case CHEESE_ROLE_CHECKBOX:
    return "checkbox";
  case CHEESE_ROLE_RADIO:
    return "radio";
  case CHEESE_ROLE_SLIDER:
    return "slider";
  case CHEESE_ROLE_SCROLLBAR:
    return "scrollbar";
  case CHEESE_ROLE_PROGRESS_BAR:
    return "progress_bar";
  case CHEESE_ROLE_TAB:
    return "tab";
  case CHEESE_ROLE_TEXT_INPUT:
    return "text_input";
  case CHEESE_ROLE_LABEL:
    return "label";
  case CHEESE_ROLE_IMAGE:
    return "image";
  case CHEESE_ROLE_DROPDOWN:
    return "dropdown";
  case CHEESE_ROLE_LIST:
    return "list";
  case CHEESE_ROLE_SCROLL:
    return "scroll";
  default:
    return "none";
  }
}

u64 cheese_semantics_count(const cheese_t *cheese) {
  return cheese ? da_len(cheese->semantics_nodes) : 0;
}

cheese_semantics_node_t *cheese_semantics_root(cheese_t *cheese) {
  return cheese_semantics_at(cheese, 0);
}

cheese_semantics_node_t *cheese_semantics_at(cheese_t *cheese, u64 index) {
  if (!cheese || index >= da_len(cheese->semantics_nodes))
    return null;

  return &cheese->semantics_nodes[index];
}

cheese_semantics_node_t *
cheese_semantics_first_child(cheese_t *cheese,
                             const cheese_semantics_node_t *node) {
  if (!cheese || !node || node->first_child < 0)
    return null;

  return cheese_semantics_at(cheese, (u64)node->first_child);
}

cheese_semantics_node_t *cheese_semantics_find(cheese_t *cheese,
                                               const cstr *key) {
  if (!cheese || !key)
    return null;

  for (u64 i = 0; i < da_len(cheese->semantics_nodes); i++) {
    const cstr *node_key = cheese->semantics_nodes[i].key;
    if (node_key && strcmp(node_key, key) == 0)
      return &cheese->semantics_nodes[i];
  }

  return null;
}

const cstr *cheese_semantics_format(cheese_t *cheese, const cstr *fmt, ...) {
  if (!cheese || !cheese->frame_arena || !fmt)
    return "";

  va_list args;
  va_list copy;
  va_start(args, fmt);
  va_copy(copy, args);

  int len = vsnprintf(null, 0, fmt, copy);
  va_end(copy);

  if (len < 0) {
    va_end(args);
    return "";
  }

  cstr *buffer = arena_alloc(cheese->frame_arena, cstr, (u64)len + 1);
  vsnprintf(buffer, (u64)len + 1, fmt, args);
  va_end(args);

  return buffer;
}

cheese_semantics_node_t *
cheese_semantics_next_sibling(cheese_t *cheese,
                              const cheese_semantics_node_t *node) {
  if (!cheese || !node || node->next_sibling < 0)
    return null;

  return cheese_semantics_at(cheese, (u64)node->next_sibling);
}

void cheese_semantics_snapshot_enable(cheese_t *cheese, arena_t *arena) {
  if (!cheese)
    return;

  if (!cheese->semantics_snapshot_init) {
    mtx_init(&cheese->semantics_snapshot_mtx, mtx_plain);
    cheese->semantics_snapshot_init = true;
  }

  cheese->semantics_snapshot_arena = arena;
}

void cheese_semantics_snapshot_disable(cheese_t *cheese) {
  cheese_semantics_snapshot_enable(cheese, null);
}

void cheese_semantics_snapshot_capture(cheese_t *cheese) {
  if (!cheese || !cheese->semantics_snapshot_arena || !cheese->semantics_nodes)
    return;

  arena_t *arena = cheese->semantics_snapshot_arena;
  u64 count = da_len(cheese->semantics_nodes);
  cheese_semantics_node_t *source = cheese->semantics_nodes;

  mtx_lock(&cheese->semantics_snapshot_mtx);
  arena_clear(arena);

  cheese_semantics_node_t *copy = null;
  if (count) {
    copy = arena_alloc(arena, cheese_semantics_node_t, count);
  }

  for (u64 i = 0; i < count; i++) {
    copy[i] = source[i];
    copy[i].key = cheese_semantics_strdup(arena, source[i].key);
    copy[i].name = cheese_semantics_strdup(arena, source[i].name);
    copy[i].value = cheese_semantics_strdup(arena, source[i].value);
    copy[i].description = cheese_semantics_strdup(arena, source[i].description);
  }

  cheese->semantics_snapshot = copy;
  cheese->semantics_snapshot_count = count;
  mtx_unlock(&cheese->semantics_snapshot_mtx);
}

void cheese_semantics_snapshot_begin(cheese_t *cheese,
                                     cheese_semantics_snapshot_t *out) {
  if (!out)
    return;

  out->nodes = null;
  out->count = 0;

  if (!cheese || !cheese->semantics_snapshot_init)
    return;

  mtx_lock(&cheese->semantics_snapshot_mtx);
  cheese->semantics_snapshot_held = true;
  out->nodes = cheese->semantics_snapshot;
  out->count = cheese->semantics_snapshot_count;
}

void cheese_semantics_snapshot_end(cheese_t *cheese) {
  if (!cheese || !cheese->semantics_snapshot_held)
    return;

  cheese->semantics_snapshot_held = false;
  mtx_unlock(&cheese->semantics_snapshot_mtx);
}
