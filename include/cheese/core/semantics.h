#ifndef CHEESE_CORE_SEMANTICS_H
#define CHEESE_CORE_SEMANTICS_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Reset the per-frame semantics tree.
 * @details Called by @ref cheese_begin; allocates the node array in the frame
 * arena and seeds the root. Widgets append to it as they draw.
 *
 * @param cheese The cheese context.
 *
 * @pre @c cheese must be valid and cannot be `null`.
 */
void cheese_semantics_begin(cheese_t *cheese);

//
//
//

/**
 * @brief Resolve the id a widget's next @ref cheese_semantics_emit will use.
 * @details Lets a widget know its own identity (e.g. to test focus) before it
 * emits. Must be followed by the matching emit in the same call.
 *
 * @param cheese The cheese context.
 * @param semantics The widget's descriptor.
 * @param fallback_role Role used when @c semantics.role is 0.
 *
 * @return The id the matching emit will assign.
 */
u64 cheese_semantics_peek_id(cheese_t *cheese, cheese_semantics_t semantics,
                             cheese_role_t fallback_role);

//
//
//

/**
 * @brief Emit a leaf node under the current parent.
 *
 * @param cheese The cheese context.
 * @param semantics The widget's descriptor (may be zeroed).
 * @param fallback_role Role used when @c semantics.role is 0.
 * @param fallback_name Name used when @c semantics.name is null.
 * @param value Optional stringified value.
 * @param state @ref cheese_semantics_state_t bits.
 * @param bounds The widget's screen rectangle.
 *
 * @return The new node's index, or -1.
 *
 * @pre @c cheese must be valid and cannot be `null`.
 */
i32 cheese_semantics_emit(cheese_t *cheese, cheese_semantics_t semantics,
                          cheese_role_t fallback_role,
                          const cstr *fallback_name, const cstr *value,
                          u32 state, cheese_rect_t bounds);

//
//
//

/**
 * @brief Emit a node that becomes the parent of later nodes.
 * @details Like @ref cheese_semantics_emit but without a value or state; close
 * it with @ref cheese_semantics_end_node.
 *
 * @param cheese The cheese context.
 * @param semantics The widget's descriptor (may be zeroed).
 * @param fallback_role Role used when @c semantics.role is 0.
 * @param fallback_name Name used when @c semantics.name is null.
 * @param bounds The node's screen rectangle.
 *
 * @return The new node's index, or -1.
 *
 * @pre @c cheese must be valid and cannot be `null`.
 */
i32 cheese_semantics_begin_node(cheese_t *cheese, cheese_semantics_t semantics,
                                cheese_role_t fallback_role,
                                const cstr *fallback_name,
                                cheese_rect_t bounds);

//
//
//

/**
 * @brief Close the node opened by @ref cheese_semantics_begin_node.
 *
 * @param cheese The cheese context.
 */
void cheese_semantics_end_node(cheese_t *cheese);

//
//
//

/**
 * @brief Number of nodes in the current frame's tree (root included).
 *
 * @param cheese The cheese context.
 *
 * @return The node count.
 */
u64 cheese_semantics_count(const cheese_t *cheese);

//
//
//

/**
 * @brief The root node.
 *
 * @param cheese The cheese context.
 *
 * @return The root, or null.
 */
cheese_semantics_node_t *cheese_semantics_root(cheese_t *cheese);

//
//
//

/**
 * @brief The node at @c index.
 *
 * @param cheese The cheese context.
 * @param index The node index.
 *
 * @return The node, or null.
 */
cheese_semantics_node_t *cheese_semantics_at(cheese_t *cheese, u64 index);

//
//
//

/**
 * @brief The first child of @c node.
 *
 * @param cheese The cheese context.
 * @param node The parent node.
 *
 * @return The first child, or null.
 */
cheese_semantics_node_t *
cheese_semantics_first_child(cheese_t *cheese,
                             const cheese_semantics_node_t *node);

//
//
//

/**
 * @brief The next sibling of @c node.
 *
 * @param cheese The cheese context.
 * @param node The node.
 *
 * @return The next sibling, or null.
 */
cheese_semantics_node_t *
cheese_semantics_next_sibling(cheese_t *cheese,
                              const cheese_semantics_node_t *node);

//
//
//

/**
 * @brief Find a node by its explicit @c key.
 *
 * @param cheese The cheese context.
 * @param key The node's explicit key.
 *
 * @return The node, or null.
 */
cheese_semantics_node_t *cheese_semantics_find(cheese_t *cheese,
                                               const cstr *key);

//
//
//

/**
 * @brief A stable, human-readable name for a role (for a11y/labels).
 *
 * @param role The role.
 *
 * @return The role's name.
 */
const cstr *cheese_semantics_role_name(cheese_role_t role);

//
//
//

/**
 * @brief Format a value into the frame arena for a node's @c value.
 *
 * @param cheese The cheese context.
 * @param fmt A printf-style format string.
 *
 * @return The formatted string (frame-arena owned).
 */
const cstr *cheese_semantics_format(cheese_t *cheese, const cstr *fmt, ...);

//
//
//

/**
 * @brief Capture each frame's semantics tree into @c arena (opt-in).
 * @details Once enabled, @ref cheese_end copies the completed tree (nodes plus
 * their strings) into @c arena every frame, replacing the previous copy - so an
 * a11y bridge (or any out-of-draw consumer) can read the last frame's tree from
 * any thread via @ref cheese_semantics_snapshot_begin. Off by default (zero
 * cost). Call on the app thread.
 *
 * @param cheese The cheese context.
 * @param arena Backing arena, or null to disable. The caller owns it and must
 * not touch it while a snapshot is held.
 */
void cheese_semantics_snapshot_enable(cheese_t *cheese, arena_t *arena);

//
//
//

/**
 * @brief Stop capturing the tree.
 *
 * @param cheese The cheese context.
 *
 * @see cheese_semantics_snapshot_enable
 */
void cheese_semantics_snapshot_disable(cheese_t *cheese);

//
//
//

/**
 * @brief Rebuild the snapshot from the current frame's tree.
 * @details Called by @ref cheese_end when a snapshot arena is set; a no-op
 * otherwise.
 */
void cheese_semantics_snapshot_capture(cheese_t *cheese);

//
//
//

/**
 * @brief Lock the last captured tree for reading.
 * @details Fills @c out with the previous frame's nodes and node count (the
 * root is index 0; links are indices). The lock is held until
 * @ref cheese_semantics_snapshot_end, so keep the read prompt and call the
 * matching end.
 *
 * @param cheese The cheese context.
 * @param out Filled with the snapshot view.
 */
void cheese_semantics_snapshot_begin(cheese_t *cheese,
                                     cheese_semantics_snapshot_t *out);

//
//
//

/**
 * @brief Unlock the snapshot opened by @ref cheese_semantics_snapshot_begin.
 */
void cheese_semantics_snapshot_end(cheese_t *cheese);

#endif // !CHEESE_CORE_SEMANTICS_H
