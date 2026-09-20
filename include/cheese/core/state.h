#ifndef CHEESE_CORE_STATE_H
#define CHEESE_CORE_STATE_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Create a state store backed by a persistent arena.
 * @details The store is standalone and thread-independent: @ref cheese_begin
 * never touches it, and values live until the arena is cleared or freed.
 *
 * @param arena The persistent arena to allocate from.
 *
 * @return The new store.
 *
 * @pre @c arena must be valid and cannot be `null`.
 */
cheese_state_store_t *cheese_state_store_new(arena_t *arena);

//
//
//

/**
 * @brief Write a b32 value.
 * @details Callable from any thread. Bumps the value's version and wakes a
 * redraw if the value is currently bound.
 *
 * @param state The value.
 * @param value The b32 to store.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
void cheese_state_set_b32(cheese_state_t *state, b32 value);

//
//
//

/**
 * @brief Write an i32 value.
 * @details Callable from any thread. Bumps the value's version and wakes a
 * redraw if the value is currently bound.
 *
 * @param state The value.
 * @param value The i32 to store.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
void cheese_state_set_i32(cheese_state_t *state, i32 value);

//
//
//

/**
 * @brief Write a u32 value.
 * @details Callable from any thread. Bumps the value's version and wakes a
 * redraw if the value is currently bound.
 *
 * @param state The value.
 * @param value The u32 to store.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
void cheese_state_set_u32(cheese_state_t *state, u32 value);

//
//
//

/**
 * @brief Write an f32 value.
 * @details Callable from any thread. Bumps the value's version and wakes a
 * redraw if the value is currently bound.
 *
 * @param state The value.
 * @param value The f32 to store.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
void cheese_state_set_f32(cheese_state_t *state, f32 value);

//
//
//

/**
 * @brief Write an f64 value.
 * @details Callable from any thread. Bumps the value's version and wakes a
 * redraw if the value is currently bound.
 *
 * @param state The value.
 * @param value The f64 to store.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
void cheese_state_set_f64(cheese_state_t *state, f64 value);

//
//
//

/**
 * @brief Write a string value.
 * @details Callable from any thread. Bumps the value's version and wakes a
 * redraw if the value is currently bound. The string is copied into the
 * store's arena.
 *
 * @param state The value.
 * @param value The string to store; copied into the store's arena.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
void cheese_state_set_str(cheese_state_t *state, const cstr *value);

//
//
//

/**
 * @brief Read a b32 value.
 * @details Thread-safe, no side effects - never binds or flags the value.
 *
 * @param state The value.
 *
 * @return The current b32.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
b32 cheese_state_get_b32(const cheese_state_t *state);

//
//
//

/**
 * @brief Read an i32 value.
 * @details Thread-safe and side-effect free: it never binds the value or marks
 * it draw-relevant.
 *
 * @param state The value.
 *
 * @return The current i32.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
i32 cheese_state_get_i32(const cheese_state_t *state);

//
//
//

/**
 * @brief Read a u32 value.
 * @details Thread-safe and side-effect free: it never binds the value or marks
 * it draw-relevant.
 *
 * @param state The value.
 *
 * @return The current u32.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
u32 cheese_state_get_u32(const cheese_state_t *state);

//
//
//

/**
 * @brief Read an f32 value.
 * @details Thread-safe and side-effect free: it never binds the value or marks
 * it draw-relevant.
 *
 * @param state The value.
 *
 * @return The current f32.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
f32 cheese_state_get_f32(const cheese_state_t *state);

//
//
//

/**
 * @brief Read an f64 value.
 * @details Thread-safe and side-effect free: it never binds the value or marks
 * it draw-relevant.
 *
 * @param state The value.
 *
 * @return The current f64.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
f64 cheese_state_get_f64(const cheese_state_t *state);

//
//
//

/**
 * @brief Read a string value and mark it bound.
 * @details Draw path only. A bound value wakes a redraw when it changes.
 *
 * @param state The value.
 *
 * @return The current string.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
const cstr *cheese_state_get_str(const cheese_state_t *state);

//
//
//

/**
 * @brief Read a b32 value AND mark it bound.
 * @details Draw path only. A bound value wakes a redraw when it changes.
 *
 * @param state The value.
 *
 * @return The current b32.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
b32 cheese_state_bind_b32(cheese_state_t *state);

//
//
//

/**
 * @brief Read an i32 value and mark it bound.
 * @details Draw path only. A bound value wakes a redraw when it changes.
 *
 * @param state The value.
 *
 * @return The current i32.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
i32 cheese_state_bind_i32(cheese_state_t *state);

//
//
//

/**
 * @brief Read a u32 value and mark it bound.
 * @details Draw path only. A bound value wakes a redraw when it changes.
 *
 * @param state The value.
 *
 * @return The current u32.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
u32 cheese_state_bind_u32(cheese_state_t *state);

//
//
//

/**
 * @brief Read an f32 value and mark it bound.
 * @details Draw path only. A bound value wakes a redraw when it changes.
 *
 * @param state The value.
 *
 * @return The current f32.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
f32 cheese_state_bind_f32(cheese_state_t *state);

//
//
//

/**
 * @brief Read an f64 value and mark it bound.
 * @details Draw path only. A bound value wakes a redraw when it changes.
 *
 * @param state The value.
 *
 * @return The current f64.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
f64 cheese_state_bind_f64(cheese_state_t *state);

//
//
//

/**
 * @brief Read a string value and mark it bound.
 * @details Draw path only. A bound value wakes a redraw when it changes.
 *
 * @param state The value.
 *
 * @return The current string.
 *
 * @pre @c state must be valid and cannot be `null`.
 */
const cstr *cheese_state_bind_str(cheese_state_t *state);

//
//
//

/**
 * @brief Get-or-create the store's b32 value named @c name.
 * @details Same name + same type returns the existing handle; same name +
 * different type logs an error and returns null. The handle is pointer-stable
 * for the store's lifetime.
 *
 * @param store The store.
 * @param name The value's name.
 * @param initial The value to create it with.
 *
 * @return The value, or null on a name/type conflict.
 *
 * @pre @c store and @c name must be valid and cannot be `null`.
 */
cheese_state_t *cheese_state_b32(cheese_state_store_t *store, const cstr *name,
                                 b32 initial);

//
//
//

/**
 * @brief Get-or-create the store's i32 value named @c name.
 * @details The same name and type returns the existing handle; the same name
 * with a different type logs an error and returns null. The handle is
 * pointer-stable for the store's lifetime.
 *
 * @param store The store.
 * @param name The value's name.
 * @param initial The i32 to create it with.
 *
 * @return The value, or null on a name/type conflict.
 *
 * @pre @c store and @c name must be valid and cannot be `null`.
 */
cheese_state_t *cheese_state_i32(cheese_state_store_t *store, const cstr *name,
                                 i32 initial);

//
//
//

/**
 * @brief Get-or-create the store's u32 value named @c name.
 * @details The same name and type returns the existing handle; the same name
 * with a different type logs an error and returns null. The handle is
 * pointer-stable for the store's lifetime.
 *
 * @param store The store.
 * @param name The value's name.
 * @param initial The u32 to create it with.
 *
 * @return The value, or null on a name/type conflict.
 *
 * @pre @c store and @c name must be valid and cannot be `null`.
 */
cheese_state_t *cheese_state_u32(cheese_state_store_t *store, const cstr *name,
                                 u32 initial);

//
//
//

/**
 * @brief Get-or-create the store's f32 value named @c name.
 * @details The same name and type returns the existing handle; the same name
 * with a different type logs an error and returns null. The handle is
 * pointer-stable for the store's lifetime.
 *
 * @param store The store.
 * @param name The value's name.
 * @param initial The f32 to create it with.
 *
 * @return The value, or null on a name/type conflict.
 *
 * @pre @c store and @c name must be valid and cannot be `null`.
 */
cheese_state_t *cheese_state_f32(cheese_state_store_t *store, const cstr *name,
                                 f32 initial);

//
//
//

/**
 * @brief Get-or-create the store's f64 value named @c name.
 * @details The same name and type returns the existing handle; the same name
 * with a different type logs an error and returns null. The handle is
 * pointer-stable for the store's lifetime.
 *
 * @param store The store.
 * @param name The value's name.
 * @param initial The f64 to create it with.
 *
 * @return The value, or null on a name/type conflict.
 *
 * @pre @c store and @c name must be valid and cannot be `null`.
 */
cheese_state_t *cheese_state_f64(cheese_state_store_t *store, const cstr *name,
                                 f64 initial);

//
//
//

/**
 * @brief Get-or-create the store's string value named @c name.
 * @details The same name and type returns the existing handle; the same name
 * with a different type logs an error and returns null. The initial string is
 * copied into the store's arena, and the handle is pointer-stable for the
 * store's lifetime.
 *
 * @param store The store.
 * @param name The value's name.
 * @param initial The string to create it with; copied into the store's arena.
 *
 * @return The value, or null on a name/type conflict.
 *
 * @pre @c store and @c name must be valid and cannot be `null`.
 */
cheese_state_t *cheese_state_str(cheese_state_store_t *store, const cstr *name,
                                 const cstr *initial);

//
//
//

/**
 * @brief Monotonic version of a value (the N-th change ever).
 *
 * @param state The value.
 *
 * @return The current version.
 */
u64 cheese_state_version(const cheese_state_t *state);

//
//
//

/**
 * @brief Coarse "any change happened" counter for main-tick dispatch.
 *
 * @param store The store.
 *
 * @return The store's generation counter.
 */
u64 cheese_state_generation(const cheese_state_store_t *store);

//
//
//

/**
 * @brief Whether a bound value changed since the last clear.
 *
 * @param store The store.
 *
 * @return True if a redraw is needed.
 */
b32 cheese_state_frame_needed(const cheese_state_store_t *store);

//
//
//

/**
 * @brief Clear the coalesced frame-needed flag.
 * @details Call after requesting one frame.
 *
 * @param store The store.
 */
void cheese_state_clear_frame_needed(cheese_state_store_t *store);

//
//
//

/**
 * @brief Begin grouping writes so they're observed atomically.
 * @details Sets inside a batch bump versions but defer the redraw wake until
 * @ref cheese_state_batch_end, which fires it once. Batches may nest.
 *
 * @param store The store.
 *
 * @pre @c store must be valid and cannot be `null`.
 */
void cheese_state_batch_begin(cheese_state_store_t *store);

//
//
//

/**
 * @brief End a write batch opened by @ref cheese_state_batch_begin.
 * @details Fires the deferred redraw wake once the outermost batch closes.
 *
 * @param store The store.
 */
void cheese_state_batch_end(cheese_state_store_t *store);

//
//
//

/**
 * @brief Subscribe to a state value's changes.
 * @details The handler runs on the next @ref cheese_state_dispatch after the
 * value changes; it is not called for the value's current state.
 *
 * @param state The state value to watch.
 * @param handler The callback to invoke on change.
 * @param userdata Passed through to @c handler.
 *
 * @return A subscription id (greater than 0), or 0 if it couldn't subscribe.
 *
 * @pre @c state and @c handler must be valid and cannot be `null`.
 */
u64 cheese_state_subscribe(cheese_state_t *state,
                           cheese_state_handler_t handler, void *userdata);

//
//
//

/**
 * @brief Remove a subscription.
 *
 * @param state The state value the subscription was made on.
 * @param id The id returned by @ref cheese_state_subscribe.
 */
void cheese_state_unsubscribe(cheese_state_t *state, u64 id);

//
//
//

/**
 * @brief Invoke handlers for every state whose version advanced.
 * @details Call once per tick. States are visited in creation order and their
 * handlers run in subscription order. Each subscription re-observes the state
 * after its handler returns, so a handler that writes state won't re-trigger
 * itself and dispatch cannot loop.
 *
 * @param store The store to dispatch.
 *
 * @pre @c store must be valid and cannot be `null`.
 * @pre Dispatch, state creation and (un)subscription share a thread; only the
 * get/set functions are cross-thread safe.
 */
void cheese_state_dispatch(cheese_state_store_t *store);

//
//
//

/**
 * @brief Read a b32 @ref cheese_value_t, binding it to its state.
 * @details Draw path only. When the value wraps a state value it reads and
 * binds it (see @ref cheese_state_bind_b32), otherwise it returns the literal.
 * Reading marks the value draw-relevant.
 *
 * @param value The binding to read.
 *
 * @return The current b32.
 */
b32 cheese_value_b32(cheese_value_t value);

//
//
//

/**
 * @brief Read an i32 @ref cheese_value_t, binding it to its state.
 * @details Draw path only. When the value wraps a state value it reads and
 * binds it (see @ref cheese_state_bind_i32), otherwise it returns the literal.
 * Reading marks the value draw-relevant.
 *
 * @param value The binding to read.
 *
 * @return The current i32.
 */
i32 cheese_value_i32(cheese_value_t value);

//
//
//

/**
 * @brief Read a u32 @ref cheese_value_t, binding it to its state.
 * @details Draw path only. When the value wraps a state value it reads and
 * binds it (see @ref cheese_state_bind_u32), otherwise it returns the literal.
 * Reading marks the value draw-relevant.
 *
 * @param value The binding to read.
 *
 * @return The current u32.
 */
u32 cheese_value_u32(cheese_value_t value);

//
//
//

/**
 * @brief Read an f32 @ref cheese_value_t, binding it to its state.
 * @details Draw path only. When the value wraps a state value it reads and
 * binds it (see @ref cheese_state_bind_f32), otherwise it returns the literal.
 * Reading marks the value draw-relevant.
 *
 * @param value The binding to read.
 *
 * @return The current f32.
 */
f32 cheese_value_f32(cheese_value_t value);

//
//
//

/**
 * @brief Read an f64 @ref cheese_value_t, binding it to its state.
 * @details Draw path only. When the value wraps a state value it reads and
 * binds it (see @ref cheese_state_bind_f64), otherwise it returns the literal.
 * Reading marks the value draw-relevant.
 *
 * @param value The binding to read.
 *
 * @return The current f64.
 */
f64 cheese_value_f64(cheese_value_t value);

//
//
//

/**
 * @brief Read a string @ref cheese_value_t, binding it to its state.
 * @details Draw path only. When the value wraps a state value it reads and
 * binds it (see @ref cheese_state_bind_str), otherwise it returns the literal.
 * Reading marks the value draw-relevant.
 *
 * @param value The binding to read.
 *
 * @return The current string.
 */
const cstr *cheese_value_str(cheese_value_t value);

//
//
//

/**
 * @brief Write a b32 @ref cheese_value_t to its state, if bound.
 * @details A literal binding has no store, so the caller keeps the written
 * value (widgets also return it).
 *
 * @param value The binding to write.
 * @param v The value to write.
 */
void cheese_value_set_b32(cheese_value_t value, b32 v);

//
//
//

/**
 * @brief Write an i32 @ref cheese_value_t to its state, if bound.
 * @details A literal binding has no store, so the caller keeps the written
 * value (widgets also return it).
 *
 * @param value The binding to write.
 * @param v The i32 to write.
 */
void cheese_value_set_i32(cheese_value_t value, i32 v);

//
//
//

/**
 * @brief Write a u32 @ref cheese_value_t to its state, if bound.
 * @details A literal binding has no store, so the caller keeps the written
 * value (widgets also return it).
 *
 * @param value The binding to write.
 * @param v The u32 to write.
 */
void cheese_value_set_u32(cheese_value_t value, u32 v);

//
//
//

/**
 * @brief Write an f32 @ref cheese_value_t to its state, if bound.
 * @details A literal binding has no store, so the caller keeps the written
 * value (widgets also return it).
 *
 * @param value The binding to write.
 * @param v The f32 to write.
 */
void cheese_value_set_f32(cheese_value_t value, f32 v);

//
//
//

/**
 * @brief Write an f64 @ref cheese_value_t to its state, if bound.
 * @details A literal binding has no store, so the caller keeps the written
 * value (widgets also return it).
 *
 * @param value The binding to write.
 * @param v The f64 to write.
 */
void cheese_value_set_f64(cheese_value_t value, f64 v);

//
//
//

/**
 * @brief Write a string @ref cheese_value_t to its state, if bound.
 * @details A literal binding has no store, so the caller keeps the written
 * value (widgets also return it).
 *
 * @param value The binding to write.
 * @param v The string to write.
 */
void cheese_value_set_str(cheese_value_t value, const cstr *v);

//
//
//

/**
 * @brief Dispatch pending changes via the store attached to @c cheese.
 *
 * @param cheese The cheese context.
 */
void cheese_dispatch(cheese_t *cheese);

//
//
//

/**
 * @brief Whether the store attached to @c cheese needs a frame.
 *
 * @param cheese The cheese context.
 *
 * @return True if a redraw is needed.
 */
b32 cheese_needs_frame(const cheese_t *cheese);

//
//
//

/**
 * @brief Clear the frame-needed flag on @c cheese's store.
 *
 * @param cheese The cheese context.
 */
void cheese_clear_frame_needed(cheese_t *cheese);

//
//
//

/**
 * @brief Wake a frame from inside a draw.
 * @details Sets the store's frame-needed flag, so a widget that animates
 * between input events (a blinking caret, a running animation) keeps the app
 * requesting frames. Callable from the render thread; the flag is atomic and is
 * cleared by @ref cheese_clear_frame_needed.
 *
 * @param cheese The cheese context.
 *
 * @see cheese_needs_frame
 */
void cheese_request_frame(cheese_t *cheese);

#endif // !CHEESE_CORE_STATE_H
