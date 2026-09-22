#ifndef CHEESE_STYLE_PROP_H
#define CHEESE_STYLE_PROP_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

#define CHEESE_PROP_BORDER_COLOR "border/color"
#define CHEESE_PROP_BORDER_WIDTH "border/width"
#define CHEESE_PROP_BORDER_SIDES "border/sides"
#define CHEESE_PROP_FOCUS_RING_COLOR "focus_ring/color"
#define CHEESE_PROP_FOCUS_RING_WIDTH "focus_ring/width"
#define CHEESE_PROP_FOCUS_RING_OFFSET "focus_ring/offset"
#define CHEESE_PROP_BG_GRADIENT "bg/gradient"
#define CHEESE_PROP_OPACITY "opacity"

//
//
//

/**
 * @brief Intern a widget-owned property name and return its stable id.
 * @details Ids persist for the context and are 1-based (0 = none); registering
 * the same name twice returns the same id and kind. Names are namespaced by
 * convention (`"gauge/needle/angle"`, `"slider/thumb/color"`), so a third-party
 * widget adds styling fields with no core edit.
 *
 * @param cheese The cheese context.
 * @param name The property name.
 * @param kind The value representation.
 *
 * @return The property id, or `0` on failure.
 */
u32 cheese_style_prop_register(cheese_t *cheese, const cstr *name,
                               cheese_prop_kind_t kind);

//
//
//

/**
 * @brief Intern the core style property ids (@ref cheese_core_style_props_t).
 * @details Called by @ref cheese_default. Idempotent by name, so a consumer
 * that registers `"border/color"` (or any core property) itself gets the same
 * id the core draw helpers use.
 *
 * @param cheese The cheese context.
 */
void cheese_style_prop_register_core(cheese_t *cheese);

//
//
//

/**
 * @brief Write a property into a style.
 * @details Allocates from @c cheese's per-frame arena; a style with no other
 * properties costs no allocation. Replaces any existing value for @c prop.
 *
 * @param cheese The cheese context (for the arena).
 * @param style The style to write.
 * @param prop The property id.
 * @param value The value; its @c id and @c kind are overwritten from @c prop.
 */
void cheese_style_prop_set(cheese_t *cheese, cheese_style_t *style, u32 prop,
                           cheese_prop_t value);

//
//
//

/**
 * @brief Write a colour property by interned id.
 * @details Replaces any existing value; see @ref cheese_style_set_prop for
 * allocation and kind semantics.
 *
 * @param cheese The cheese context (for the arena).
 * @param style The style to write.
 * @param prop The interned property id.
 * @param color The colour to store.
 */
void cheese_style_prop_set_color(cheese_t *cheese, cheese_style_t *style,
                                 u32 prop, cheese_color_t color);

//
//
//

/**
 * @brief Write an f32 property by interned id.
 * @details Replaces any existing value; see @ref cheese_style_set_prop for
 * allocation and kind semantics.
 *
 * @param cheese The cheese context (for the arena).
 * @param style The style to write.
 * @param prop The interned property id.
 * @param value The f32 to store.
 */
void cheese_style_prop_set_f32(cheese_t *cheese, cheese_style_t *style,
                               u32 prop, f32 value);

//
//
//

/**
 * @brief Write a u32 property by interned id.
 * @details Replaces any existing value; see @ref cheese_style_set_prop for
 * allocation and kind semantics.
 *
 * @param cheese The cheese context (for the arena).
 * @param style The style to write.
 * @param prop The interned property id.
 * @param value The u32 to store.
 */
void cheese_style_prop_set_u32(cheese_t *cheese, cheese_style_t *style,
                               u32 prop, u32 value);

//
//
//

/** @brief Write a gradient property by interned id.
 * @details The @ref cheese_gradient_t is copied into the frame arena, so the
 * caller may pass a stack value that goes out of scope; replaces any existing
 * value. Stored as a @c CHEESE_PROP_PTR payload.
 *
 * @param cheese The cheese context (for the arena).
 * @param style The style to write.
 * @param prop The interned property id.
 * @param gradient The four-corner gradient to copy.
 */
void cheese_style_prop_set_gradient(cheese_t *cheese, cheese_style_t *style,
                                    u32 prop, cheese_gradient_t gradient);

//
//
//

/**
 * @brief Read a property from a resolved style.
 *
 * @param style The resolved style.
 * @param prop The property id.
 * @param out Filled with the value when present.
 *
 * @return true when the style carries @c prop.
 */
b32 cheese_style_prop_get(const cheese_style_t *style, u32 prop,
                          cheese_prop_t *out);

//
//
//

/**
 * @brief Read a colour property, or @c fallback when absent.
 *
 * @param style The resolved style.
 * @param prop The interned property id.
 * @param fallback The value to return when @c prop is absent or mistyped.
 *
 * @return The colour, or @c fallback.
 */
cheese_color_t cheese_style_prop_get_color(const cheese_style_t *style,
                                           u32 prop, cheese_color_t fallback);

//
//
//

/**
 * @brief Read an f32 property, or @c fallback when absent.
 *
 * @param style The resolved style.
 * @param prop The interned property id.
 * @param fallback The value to return when @c prop is absent or mistyped.
 *
 * @return The f32, or @c fallback.
 */
f32 cheese_style_prop_get_f32(const cheese_style_t *style, u32 prop,
                              f32 fallback);

//
//
//

/**
 * @brief Read a u32 property, or @c fallback when absent.
 *
 * @param style The resolved style.
 * @param prop The interned property id.
 * @param fallback The value to return when @c prop is absent or mistyped.
 *
 * @return The u32, or @c fallback.
 */
u32 cheese_style_prop_get_u32(const cheese_style_t *style, u32 prop,
                              u32 fallback);

//
//
//

/**
 * @brief Read a gradient property, or @c fallback when absent.
 *
 * @param style The resolved style.
 * @param prop The property id.
 * @param fallback The value to return when @c prop is absent or mistyped.
 *
 * @return The gradient, or @c fallback.
 */
cheese_gradient_t cheese_style_prop_get_gradient(const cheese_style_t *style,
                                                 u32 prop,
                                                 cheese_gradient_t fallback);

#endif // !CHEESE_STYLE_PROP_H
