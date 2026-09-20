#ifndef CHEESE_PATTERNS_TOAST_H
#define CHEESE_PATTERNS_TOAST_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief One queued toast.
 * @details @c text points into the stack's arena, so it outlives the caller's
 * buffer. @c remaining counts down in seconds; @c duration is the original
 * lifetime (for the fade).
 */
typedef struct {
  const cstr *text;
  f32 remaining;
  f32 duration;
} cheese_toast_t;

/**
 * @brief A bounded stack of toasts (caller-owned).
 * @details A pattern/composite, not a core widget: built on @ref cheese_overlay
 * (+ @ref cheese_anim_f32_t if you want motion) and @ref cheese_style_t. The
 * caller pushes, ticks with the frame's delta time, and draws once per frame.
 */
typedef struct {
  cheese_toast_t *items; // capacity slots, arena-owned
  u32 count;
  u32 capacity;
  arena_t *arena;
} cheese_toast_stack_t;

//
//
//

/**
 * @brief Initialise a toast stack.
 * @param stack The stack.
 * @param arena Backing arena for the slots and copied text.
 * @param capacity Maximum live toasts (oldest is dropped when full).
 */
void cheese_toast_stack_init(cheese_toast_stack_t *stack, arena_t *arena,
                             u32 capacity);

//
//
//

/**
 * @brief Push a toast; @c text is copied into the stack's arena.
 *
 * @param stack The stack.
 * @param text The message.
 * @param duration Lifetime in seconds.
 */
void cheese_toast_push(cheese_toast_stack_t *stack, const cstr *text,
                       f32 duration);

//
//
//

/**
 * @brief Age the toasts and drop the expired ones.
 *
 * @param stack The stack.
 * @param delta_time Seconds since the last tick.
 */
void cheese_toast_tick(cheese_toast_stack_t *stack, f32 delta_time);

//
//
//

/**
 * @brief Draw the stack (top-right, fading out) as overlays.
 * @details Registers one clipped overlay per live toast, so they float above
 * the tree and block input beneath them like any overlay.
 *
 * @param cheese The cheese context.
 * @param classes Style classes for the toast panel.
 * @param stack The stack.
 * @param font The font (null draws panels only).
 */
void cheese_toast_draw(cheese_t *cheese, const cstr *classes,
                       cheese_toast_stack_t *stack, cheese_font_t *font);

#endif // !CHEESE_PATTERNS_TOAST_H
