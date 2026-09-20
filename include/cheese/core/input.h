#ifndef CHEESE_CORE_INPUT_H
#define CHEESE_CORE_INPUT_H

/***********************************/

#include <cheese/types.h>

/***********************************/

/**
 * @brief Whether a non-repeat press of @c key happened this frame.
 *
 * @param cheese The cheese context.
 * @param key The key to test.
 *
 * @return true if @c key was pressed this frame.
 */
b32 cheese_key_pressed(const cheese_t *cheese, cheese_key_t key);

//
//
//

/**
 * @brief Number of key events delivered this frame.
 *
 * @param cheese The cheese context.
 *
 * @return The event count.
 */
u32 cheese_key_event_count(const cheese_t *cheese);

//
//
//

/**
 * @brief The key event at @c index, or null.
 *
 * @param cheese The cheese context.
 * @param index Event index, from @c 0 to @ref cheese_key_event_count minus one.
 *
 * @return The event, or null if out of range.
 */
const cheese_key_event_t *cheese_key_event_at(const cheese_t *cheese,
                                              u32 index);

//
//
//

/**
 * @brief Move keyboard focus through the focusable nodes of the tree.
 * @details Walks the current frame's semantics tree in call order (which is
 * preorder). @c dir @c >0 selects the next node, @c <0 the previous; moving
 * past an end wraps. When nothing is focused, moves to the first (or last)
 * focusable.
 *
 * @param cheese The cheese context.
 * @param dir Direction to move (@c +1 next, @c -1 previous).
 */
void cheese_focus_move(cheese_t *cheese, i32 dir);

//
//
//

/**
 * @brief Move focus to the nearest focusable node in a direction.
 * @details Scores candidates by bounds centre: axis distance + twice the
 * off-axis distance. Nothing focused → first focusable. Call directly from a
 * custom widget that wants its own arrow keys.
 */
void cheese_focus_move_dir(cheese_t *cheese, cheese_focus_dir_t dir);

//
//
//

/**
 * @brief Enable/disable automatic spatial arrow-key focus movement (default
 * on).
 */
void cheese_set_arrow_nav(cheese_t *cheese, b32 enabled);

//
//
//

/**
 * @brief Apply this frame's arrow keys to spatial focus movement.
 * @internal Called from @ref cheese_end; not for consumer use.
 */
void cheese_focus_arrows(cheese_t *cheese);

//
//
//

/**
 * @brief Claim the pointer for @c id (on press), recording the press origin.
 * @details Held until the primary button is released (cleared in
 * @ref cheese_begin). Lets a widget keep receiving a drag after the cursor
 * leaves its bounds.
 *
 * @param cheese The cheese context.
 * @param id The widget's identity.
 */
void cheese_capture(cheese_t *cheese, u64 id);

//
//
//

/**
 * @brief Whether @c id currently owns the pointer.
 *
 * @param cheese The cheese context.
 * @param id The widget's identity.
 *
 * @return True if @c id owns the pointer.
 */
b32 cheese_captured(const cheese_t *cheese, u64 id);

#endif // !CHEESE_CORE_INPUT_H
