/***********************************/

#include <threads.h>

#include <htils/atomic_types.h>
#include <htils/basictypes.h>
#include <htils/darray.h>
#include <htils/string.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/state.h>

/***********************************/

static void cheese_state_wake_if_bound(cheese_state_store_t *store,
                                       b32 was_bound) {
  if (!was_bound)
    return;

  if (atomic_load(&store->batch_depth) > 0)
    atomic_store(&store->batch_pending, true);
  else
    atomic_store(&store->frame_needed, true);
}

//
//
//

static void cheese_state_write_finish(cheese_state_t *state, b32 changed) {
  if (!changed)
    return;

  atomic_fetch_add(&state->version, 1);
  cheese_state_wake_if_bound(state->store, atomic_load(&state->bound));
}

//
//
//

static cheese_state_t *cheese_state_type_check(const cheese_state_t *state,
                                               cheese_state_type_t type,
                                               const cstr *op) {
  if (!state) {
    cheese_log_error("%s: Invalid state", op);
    return null;
  }

  if (state->type != type) {
    cheese_log_error("%s: State type mismatch on '%s'", op, state->name);
    return null;
  }

  return (cheese_state_t *)state;
}

//
//
//

static cstr *cheese_state_str_dup(arena_t *arena, const cstr *value) {
  if (!value)
    value = "";

  u64 len = strlen(value);
  cstr *copy = arena_alloc(arena, cstr, len + 1);
  memcpy(copy, value, len);
  copy[len] = '\0';
  return copy;
}

//
//
//

static void cheese_state_mark_bound(cheese_state_t *state) {
  atomic_store(&state->bound, true);
}

//
//
//

static cheese_state_t *cheese_state_get_or_create(cheese_state_store_t *store,
                                                  const cstr *name,
                                                  cheese_state_type_t type,
                                                  const void *initial) {
  if (!store || !name || !*name) {
    cheese_log_error("cheese_state_get_or_create: Invalid parameters");
    return null;
  }

  mtx_lock(&store->mtx);
  for (u64 i = 0; i < da_len(store->states); i++) {
    cheese_state_t *existing = store->states[i];
    if (strcmp(existing->name, name) != 0)
      continue;

    if (existing->type != type) {
      cheese_log_error("cheese_state_get_or_create: '%s' already exists with a "
                       "different type",
                       name);
      mtx_unlock(&store->mtx);
      return null;
    }

    mtx_unlock(&store->mtx);
    return existing;
  }

  cheese_state_t *state = arena_alloc_zeroed(store->arena, cheese_state_t, 1);
  state->name = cheese_state_str_dup(store->arena, name);
  state->type = type;
  state->store = store;
  state->version = ATOMIC_VAR_INIT(0);
  state->bound = ATOMIC_VAR_INIT(false);
  state->next_sub_id = 1;

  switch (type) {
  case CHEESE_STATE_BOOL:
    state->value.as_b32 = ATOMIC_VAR_INIT(*(const b32 *)initial);
    break;

  case CHEESE_STATE_I32:
    state->value.as_i32 = ATOMIC_VAR_INIT(*(const i32 *)initial);
    break;

  case CHEESE_STATE_U32:
    state->value.as_u32 = ATOMIC_VAR_INIT(*(const u32 *)initial);
    break;

  case CHEESE_STATE_F32:
    state->value.as_f32 = ATOMIC_VAR_INIT(*(const f32 *)initial);
    break;

  case CHEESE_STATE_F64:
    state->value.as_f64 = ATOMIC_VAR_INIT(*(const f64 *)initial);
    break;

  case CHEESE_STATE_STR:
    if (initial && *(const cstr *)initial)
      state->value.as_str =
          cheese_state_str_dup(store->arena, (const cstr *)initial);
    else
      state->value.as_str = null;
    break;

  default:
    break;
  }

  da_append(store->arena, store->states, state);
  mtx_unlock(&store->mtx);
  return state;
}

//
//
//

cheese_state_store_t *cheese_state_store_new(arena_t *arena) {
  if (!arena) {
    cheese_log_error("cheese_state_store_new: Invalid arena");
    return null;
  }

  cheese_state_store_t *store =
      arena_alloc_zeroed(arena, cheese_state_store_t, 1);
  store->arena = arena;
  store->generation = ATOMIC_VAR_INIT(0);
  store->frame_needed = ATOMIC_VAR_INIT(false);
  store->batch_depth = ATOMIC_VAR_INIT(0);
  store->batch_pending = ATOMIC_VAR_INIT(false);

  da_new(store->arena, store->states, 8);
  if (mtx_init(&store->mtx, mtx_plain) != thrd_success) {
    cheese_log_error(
        "cheese_state_store_new: Failed to create state store mutex");
    return null;
  }

  return store;
}

void cheese_state_set_b32(cheese_state_t *state, b32 value) {
  if (!state)
    return;

  b32 changed = (atomic_load(&state->value.as_b32) != value);
  if (changed)
    atomic_store(&state->value.as_b32, value);

  cheese_state_write_finish(state, changed);
}

void cheese_state_set_i32(cheese_state_t *state, i32 value) {
  if (!state)
    return;

  b32 changed = (atomic_load(&state->value.as_i32) != value);
  if (changed)
    atomic_store(&state->value.as_i32, value);

  cheese_state_write_finish(state, changed);
}

void cheese_state_set_u32(cheese_state_t *state, u32 value) {
  if (!state)
    return;

  b32 changed = (atomic_load(&state->value.as_u32) != value);
  if (changed)
    atomic_store(&state->value.as_u32, value);

  cheese_state_write_finish(state, changed);
}

void cheese_state_set_f32(cheese_state_t *state, f32 value) {
  if (!state)
    return;

  b32 changed = (atomic_load(&state->value.as_f32) != value);
  if (changed)
    atomic_store(&state->value.as_f32, value);

  cheese_state_write_finish(state, changed);
}

void cheese_state_set_f64(cheese_state_t *state, f64 value) {
  if (!state)
    return;

  b32 changed = (atomic_load(&state->value.as_f64) != value);
  if (changed)
    atomic_store(&state->value.as_f64, value);

  cheese_state_write_finish(state, changed);
}

void cheese_state_set_str(cheese_state_t *state, const cstr *value) {
  if (!state)
    return;

  if (!value)
    value = "";

  cstr *cur = atomic_load(&state->value.as_str);
  b32 changed = (!cur && *value) || (cur && strcmp(cur, value) != 0);
  if (!changed)
    return;

  cheese_state_store_t *store = state->store;
  cstr *copy = cheese_state_str_dup(store->arena, value);
  atomic_store(&state->value.as_str, copy);
  cheese_state_write_finish(state, changed);
}

b32 cheese_state_get_b32(const cheese_state_t *state) {
  const cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_BOOL, "cheese_state_get_b32");
  if (!checked)
    return false;

  return atomic_load(&checked->value.as_b32);
}

i32 cheese_state_get_i32(const cheese_state_t *state) {
  const cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_I32, "cheese_state_get_i32");
  if (!checked)
    return 0;

  return atomic_load(&checked->value.as_i32);
}

u32 cheese_state_get_u32(const cheese_state_t *state) {
  const cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_U32, "cheese_state_get_u32");
  if (!checked)
    return 0;

  return atomic_load(&checked->value.as_u32);
}

f32 cheese_state_get_f32(const cheese_state_t *state) {
  const cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_F32, "cheese_state_get_f32");
  if (!checked)
    return 0.0f;

  return atomic_load(&checked->value.as_f32);
}

f64 cheese_state_get_f64(const cheese_state_t *state) {
  const cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_F64, "cheese_state_get_f64");
  if (!checked)
    return 0.0;

  return atomic_load(&checked->value.as_f64);
}

const cstr *cheese_state_get_str(const cheese_state_t *state) {
  const cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_STR, "cheese_state_get_str");
  if (!checked)
    return "";

  cstr *p = atomic_load(&checked->value.as_str);
  return p ? p : "";
}

b32 cheese_state_bind_b32(cheese_state_t *state) {
  cheese_state_t *checked = cheese_state_type_check(state, CHEESE_STATE_BOOL,
                                                    "cheese_state_bind_b32");

  if (!checked)
    return false;

  cheese_state_mark_bound(checked);
  return atomic_load(&checked->value.as_b32);
}

i32 cheese_state_bind_i32(cheese_state_t *state) {
  cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_I32, "cheese_state_bind_i32");

  if (!checked)
    return 0;

  cheese_state_mark_bound(checked);
  return atomic_load(&checked->value.as_i32);
}

u32 cheese_state_bind_u32(cheese_state_t *state) {
  cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_U32, "cheese_state_bind_u32");

  if (!checked)
    return 0;

  cheese_state_mark_bound(checked);
  return atomic_load(&checked->value.as_u32);
}

f32 cheese_state_bind_f32(cheese_state_t *state) {
  cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_F32, "cheese_state_bind_f32");

  if (!checked)
    return 0.0f;

  cheese_state_mark_bound(checked);
  return atomic_load(&checked->value.as_f32);
}

f64 cheese_state_bind_f64(cheese_state_t *state) {
  cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_F64, "cheese_state_bind_f64");

  if (!checked)
    return 0.0;

  cheese_state_mark_bound(checked);
  return atomic_load(&checked->value.as_f64);
}

const cstr *cheese_state_bind_str(cheese_state_t *state) {
  cheese_state_t *checked =
      cheese_state_type_check(state, CHEESE_STATE_STR, "cheese_state_bind_str");

  if (!checked)
    return "";

  cheese_state_mark_bound(checked);
  cstr *p = atomic_load(&checked->value.as_str);
  return p ? p : "";
}

cheese_state_t *cheese_state_b32(cheese_state_store_t *store, const cstr *name,
                                 b32 initial) {
  b32 i = initial;
  return cheese_state_get_or_create(store, name, CHEESE_STATE_BOOL, &i);
}

cheese_state_t *cheese_state_i32(cheese_state_store_t *store, const cstr *name,
                                 i32 initial) {
  i32 i = initial;
  return cheese_state_get_or_create(store, name, CHEESE_STATE_I32, &i);
}

cheese_state_t *cheese_state_u32(cheese_state_store_t *store, const cstr *name,
                                 u32 initial) {
  u32 i = initial;
  return cheese_state_get_or_create(store, name, CHEESE_STATE_U32, &i);
}

cheese_state_t *cheese_state_f32(cheese_state_store_t *store, const cstr *name,
                                 f32 initial) {
  f32 i = initial;
  return cheese_state_get_or_create(store, name, CHEESE_STATE_F32, &i);
}

cheese_state_t *cheese_state_f64(cheese_state_store_t *store, const cstr *name,
                                 f64 initial) {
  f64 i = initial;
  return cheese_state_get_or_create(store, name, CHEESE_STATE_F64, &i);
}

cheese_state_t *cheese_state_str(cheese_state_store_t *store, const cstr *name,
                                 const cstr *initial) {
  return cheese_state_get_or_create(store, name, CHEESE_STATE_STR, initial);
}

u64 cheese_state_version(const cheese_state_t *state) {
  if (!state)
    return 0;

  return atomic_load(&state->version);
}

u64 cheese_state_generation(const cheese_state_store_t *store) {
  if (!store)
    return 0;

  return atomic_load(&store->generation);
}

b32 cheese_state_frame_needed(const cheese_state_store_t *store) {
  if (!store)
    return false;

  return (b32)atomic_load(&store->frame_needed);
}

void cheese_state_clear_frame_needed(cheese_state_store_t *store) {
  if (!store)
    return;

  atomic_store(&store->frame_needed, false);
}

void cheese_state_batch_begin(cheese_state_store_t *store) {
  if (!store)
    return;

  atomic_fetch_add(&store->batch_depth, 1);
}

void cheese_state_batch_end(cheese_state_store_t *store) {
  if (!store)
    return;

  if (atomic_fetch_sub(&store->batch_depth, 1) == 1) {
    atomic_fetch_add(&store->generation, 1);
    if (atomic_load(&store->batch_pending)) {
      atomic_store(&store->batch_pending, false);
      atomic_store(&store->frame_needed, true);
    }
  }
}

u64 cheese_state_subscribe(cheese_state_t *state,
                           cheese_state_handler_t handler, void *userdata) {
  if (!state || !handler) {
    cheese_log_error("cheese_state_subscribe: Invalid parameters");
    return 0;
  }

  if (!state->subs)
    da_new(state->store->arena, state->subs, 2);

  cheese_state_subscription_t sub = {
      .id = state->next_sub_id++,
      .handler = handler,
      .userdata = userdata,
      .last_version = atomic_load(&state->version),
  };

  da_append(state->store->arena, state->subs, sub);
  return sub.id;
}

void cheese_state_unsubscribe(cheese_state_t *state, u64 id) {
  if (!state || !state->subs || id == 0)
    return;

  for (u64 i = 0; i < da_len(state->subs); i++) {
    if (state->subs[i].id != id)
      continue;

    for (u64 j = i + 1; j < da_len(state->subs); j++)
      state->subs[j - 1] = state->subs[j];

    da_pop(state->subs);
    return;
  }
}

void cheese_state_dispatch(cheese_state_store_t *store) {
  if (!store)
    return;

  for (u64 i = 0; i < da_len(store->states); i++) {
    cheese_state_t *state = store->states[i];
    u64 version = atomic_load(&state->version);

    for (u64 j = 0; j < da_len(state->subs); j++) {
      cheese_state_subscription_t *sub = &state->subs[j];
      if (sub->last_version == version)
        continue;

      sub->handler(state, sub->userdata);
      sub->last_version = atomic_load(&state->version);
    }
  }
}

b32 cheese_value_b32(cheese_value_t value) {
  if (value.state)
    return cheese_state_bind_b32(value.state);

  return value.literal.b32;
}

i32 cheese_value_i32(cheese_value_t value) {
  if (value.state)
    return cheese_state_bind_i32(value.state);

  return value.literal.i32;
}

u32 cheese_value_u32(cheese_value_t value) {
  if (value.state)
    return cheese_state_bind_u32(value.state);

  return value.literal.u32;
}

f32 cheese_value_f32(cheese_value_t value) {
  if (value.state)
    return cheese_state_bind_f32(value.state);

  return value.literal.f32;
}

f64 cheese_value_f64(cheese_value_t value) {
  if (value.state)
    return cheese_state_bind_f64(value.state);

  return value.literal.f64;
}

const cstr *cheese_value_str(cheese_value_t value) {
  if (value.state)
    return cheese_state_bind_str(value.state);

  return value.literal.str ? value.literal.str : "";
}

void cheese_value_set_b32(cheese_value_t value, b32 v) {
  if (value.state)
    cheese_state_set_b32(value.state, v);
}

void cheese_value_set_i32(cheese_value_t value, i32 v) {
  if (value.state)
    cheese_state_set_i32(value.state, v);
}

void cheese_value_set_u32(cheese_value_t value, u32 v) {
  if (value.state)
    cheese_state_set_u32(value.state, v);
}

void cheese_value_set_f32(cheese_value_t value, f32 v) {
  if (value.state)
    cheese_state_set_f32(value.state, v);
}

void cheese_value_set_f64(cheese_value_t value, f64 v) {
  if (value.state)
    cheese_state_set_f64(value.state, v);
}

void cheese_value_set_str(cheese_value_t value, const cstr *v) {
  if (value.state)
    cheese_state_set_str(value.state, v);
}

void cheese_dispatch(cheese_t *cheese) {
  if (!cheese)
    return;

  cheese_state_dispatch(cheese->store);
}

b32 cheese_needs_frame(const cheese_t *cheese) {
  if (!cheese)
    return false;

  return cheese_state_frame_needed(cheese->store) || cheese->input_frame_needed;
}

void cheese_clear_frame_needed(cheese_t *cheese) {
  if (!cheese)
    return;

  cheese_state_clear_frame_needed(cheese->store);
  cheese->input_frame_needed = false;
}

void cheese_request_frame(cheese_t *cheese) {
  if (!cheese || !cheese->store)
    return;

  atomic_store(&cheese->store->frame_needed, true);
}
