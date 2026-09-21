#ifndef CHEESE_CORE_STYLE_H
#define CHEESE_CORE_STYLE_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Built-in style-class names.
 * @details @ref cheese_theme_apply registers a class style and, for most
 * roles, a matching typed role style. Bare widgets are themed through their
 * role defaults; callers may still pass extra classes alongside a role.
 */
#define CHEESE_CLASS_CONTAINER "container"
#define CHEESE_CLASS_SCROLL "scroll"
#define CHEESE_CLASS_BUTTON "button"
#define CHEESE_CLASS_CHECKBOX "checkbox"
#define CHEESE_CLASS_RADIO "radio"
#define CHEESE_CLASS_SLIDER "slider"
#define CHEESE_CLASS_SCROLLBAR "scrollbar"
#define CHEESE_CLASS_PROGRESS "progress"
#define CHEESE_CLASS_TEXT_INPUT "text-input"
#define CHEESE_CLASS_DROPDOWN "dropdown"
#define CHEESE_CLASS_TABS "tabs"
#define CHEESE_CLASS_LIST "list"
#define CHEESE_CLASS_IMAGE "image"

//
//
//

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
 * @brief Get the default style / begin a new style.
 * @details Every value starts as "inherit":
 * - colours (`bg`, `hover`, `pressed`, `disabled`, `focus`, `text`,
 *   `state_layer`): `0`;
 * - `corner_radius`, `padding`, `margin`: inherit (`-1.0f`);
 * - `font_size`: `0`.
 *
 * The property bag starts empty; see @ref cheese_style_set_prop. The core's
 * own properties (border, focus ring) are documented in
 * @ref cheese_core_style_props_t.
 *
 * @return The default style.
 */
cheese_style_t cheese_style_new(void);

//
//
//

/** @brief Set the background colour. `0` = inherit. */
void cheese_style_set_bg_color(cheese_style_t *style, cheese_color_t color);

//
//
//

/** @brief Set the hover background colour. `0` = inherit. */
void cheese_style_set_hover_color(cheese_style_t *style, cheese_color_t color);

//
//
//

/** @brief Set the pressed background colour. `0` = inherit. */
void cheese_style_set_pressed_color(cheese_style_t *style,
                                    cheese_color_t color);

//
//
//

/** @brief Set the disabled background colour. `0` = inherit. */
void cheese_style_set_disabled_color(cheese_style_t *style,
                                     cheese_color_t color);

//
//
//

/** @brief Set the focused background colour. `0` = inherit. */
void cheese_style_set_focus_color(cheese_style_t *style, cheese_color_t color);

//
//
//

/** @brief Set the text colour. `0` = inherit. */
void cheese_style_set_text_color(cheese_style_t *style, cheese_color_t color);

//
//
//

/**
 * @brief Set the interaction-state overlay colour.
 * @details When the explicit `hover`/`pressed`/`focus`/`disabled` colours are
 * unset, @ref cheese_style_apply_state overlays this colour on @c bg_color at
 * the M3 state-layer opacity (hover 8%, focus 10%, pressed/disabled 12%).
 * `0` = inherit.
 */
void cheese_style_set_state_layer_color(cheese_style_t *style,
                                        cheese_color_t color);

//
//
//

/** @brief Inherit the corner radius from the style stack. */
void cheese_style_set_corner_radius_inherit(cheese_style_t *style);

//
//
//

/** @brief Set a uniform corner radius on all four corners. */
void cheese_style_set_corner_radius_uniform(cheese_style_t *style, f32 radius);

//
//
//

/** @brief Set each corner radius individually. */
void cheese_style_set_corner_radius(cheese_style_t *style, f32 top_left,
                                    f32 top_right, f32 bottom_left,
                                    f32 bottom_right);

//
//
//

/** @brief Inherit the padding from the style stack. */
void cheese_style_set_padding_inherit(cheese_style_t *style);

//
//
//

/** @brief Set uniform padding on all four edges. */
void cheese_style_set_padding_uniform(cheese_style_t *style, f32 padding);

//
//
//

/** @brief Set each padding edge individually (left, bottom, top, right). */
void cheese_style_set_padding(cheese_style_t *style, f32 padding_left,
                              f32 padding_bottom, f32 padding_top,
                              f32 padding_right);

//
//
//

/** @brief Inherit the margin from the style stack. */
void cheese_style_set_margin_inherit(cheese_style_t *style);

//
//
//

/** @brief Set a uniform margin on all four edges. */
void cheese_style_set_margin_uniform(cheese_style_t *style, f32 margin);

//
//
//

/** @brief Set each margin edge individually (left, bottom, top, right). */
void cheese_style_set_margin(cheese_style_t *style, f32 margin_left,
                             f32 margin_bottom, f32 margin_top,
                             f32 margin_right);

//
//
//

/**
 * @brief Set the cursor shown while hovering the widget.
 * @param cursor A @ref cheese_cursor_t; inherit (`-1`) when not a valid value.
 */
void cheese_style_set_cursor(cheese_style_t *style, cheese_cursor_t cursor);

//
//
//

/** @brief Inherit the cursor from the style stack. */
void cheese_style_set_cursor_inherit(cheese_style_t *style);

//
//
//

/**
 * @brief Set whether the style's text is selectable.
 * @details A selectable style makes @ref cheese_draw_text record its run in
 * the global selection layer. Pass @c false to opt out.
 *
 * @param style The style to write.
 * @param selectable Whether the text can be selected.
 */
void cheese_style_set_selectable(cheese_style_t *style, b32 selectable);

//
//
//

/** @brief Inherit the selectable state from the style stack. */
void cheese_style_set_selectable_inherit(cheese_style_t *style);

//
//
//

/** @brief Set the font size. `0` = inherit. */
void cheese_style_set_font_size(cheese_style_t *style, u32 font_size);

//
//
//

/**
 * @brief The current top of the style stack.
 * @param cheese The cheese context.
 *
 * @return The style widgets resolve from by default.
 */
cheese_style_t *cheese_current_style(cheese_t *cheese);

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
u32 cheese_prop_register(cheese_t *cheese, const cstr *name,
                         cheese_prop_kind_t kind);

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
void cheese_style_set_prop(cheese_t *cheese, cheese_style_t *style, u32 prop,
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
void cheese_style_set_prop_color(cheese_t *cheese, cheese_style_t *style,
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
void cheese_style_set_prop_f32(cheese_t *cheese, cheese_style_t *style,
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
void cheese_style_set_prop_u32(cheese_t *cheese, cheese_style_t *style,
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
void cheese_style_set_prop_gradient(cheese_t *cheese, cheese_style_t *style,
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
b32 cheese_style_get_prop(const cheese_style_t *style, u32 prop,
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
cheese_color_t cheese_style_get_prop_color(const cheese_style_t *style,
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
f32 cheese_style_get_prop_f32(const cheese_style_t *style, u32 prop,
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
u32 cheese_style_get_prop_u32(const cheese_style_t *style, u32 prop,
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
cheese_gradient_t cheese_style_get_prop_gradient(const cheese_style_t *style,
                                                 u32 prop,
                                                 cheese_gradient_t fallback);

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
void cheese_style_register_core_props(cheese_t *cheese);

//
//
//

/** @brief Push @c style as the current style for the following widgets. */
void cheese_push_style(cheese_t *cheese, cheese_style_t style);

//
//
//

/** @brief Pop the style pushed by @ref cheese_push_style. */
void cheese_pop_style(cheese_t *cheese);

//
//
//

/**
 * @brief Push inherited values and their classes as one lexical scope.
 * @details Equivalent to @ref cheese_push_style plus the scope's class names.
 * Later scopes win over earlier scopes during resolution.
 *
 * @param cheese The cheese context.
 * @param style The inherited values for the scope.
 * @param classes Space-separated class names, or null.
 */
void cheese_push_scope(cheese_t *cheese, cheese_style_t style,
                       const cstr *classes);

//
//
//

/** @brief Pop the scope pushed by @ref cheese_push_scope. */
void cheese_pop_scope(cheese_t *cheese);

//
//
//

/**
 * @brief Register a named style class for this frame.
 * @details Containers push a scope carrying classes that cascade to children;
 * widgets can also name a class. Re-registering a name replaces it.
 *
 * @param cheese The cheese context.
 * @param name The class name.
 * @param style The style to register.
 */
void cheese_style_class_register(cheese_t *cheese, const cstr *name,
                                 cheese_style_t style);

//
//
//

/**
 * @brief Overlay the registered classes in @c classes onto @c style.
 *
 * @param cheese The cheese context.
 * @param style The style to modify in place.
 * @param classes Space-separated class names, or null.
 */
void cheese_style_apply_classes(cheese_t *cheese, cheese_style_t *style,
                                const cstr *classes);

//
//
//

/**
 * @brief The overlay alpha @ref cheese_style_apply_state uses for a state.
 * @details 0.12 for disabled/pressed, 0.08 for hovered, 0.10 for focused, else
 * `0`.
 *
 * @param state The interaction-state bits.
 *
 * @return The alpha to overlay `state_layer_color` onto the background.
 */
f32 cheese_style_state_layer_alpha(u32 state);

//
//
//

/**
 * @brief Overlay the interaction-state background onto a resolved style.
 * @details Priority disabled > pressed > hover > focus; a `0` state colour is
 * skipped.
 */
void cheese_style_apply_state(cheese_style_t *style, u32 state);

//
//
//

/** @brief Resolve a widget's style from the stack and its classes. */
void cheese_style_resolve(cheese_t *cheese, cheese_style_t *out,
                          const cstr *classes);

//
//
//

/**
 * @brief Register a typed default style for a role (this frame).
 * @details The role default, parallel to the string role classes. Re-
 * registering replaces it; cleared each frame like the class map.
 *
 * @param cheese The cheese context.
 * @param role The role (@c CHEESE_ROLE_NONE and out-of-range are ignored).
 * @param style The default style.
 */
void cheese_style_role_register(cheese_t *cheese, cheese_role_t role,
                                cheese_style_t style);

//
//
//

/**
 * @brief Resolve a style by scope walk.
 * @details Cascade, lowest first: the inherited root/base scope, then the typed
 * @c role default, then the remaining lexical scopes (outer → inner), then the
 * widget's @c classes, and finally @c explicit. Later wins by presence. This
 * prevents a broad inherited value from hiding a role's more specific default.
 * No specificity and no rule matcher - a widget that wants an interaction state
 * applies @ref cheese_style_apply_state to the result, and one that wants an
 * override passes @c explicit.
 *
 * @param cheese The cheese context.
 * @param out The resolved style.
 * @param role The widget's role, or @c CHEESE_ROLE_NONE for none.
 * @param classes Extra own classes, or null.
 * @param explicit A final explicit style, or null.
 */
void cheese_style_resolve_scoped(cheese_t *cheese, cheese_style_t *out,
                                 cheese_role_t role, const cstr *classes,
                                 const cheese_style_t *explicit);

//
//
//

/** @brief Whether the style opts into text selection (off by default). */
static inline b32 cheese_style_get_selectable(const cheese_style_t *style) {
  return style->selectable > 0;
}

//
//
//

/** @brief Get the left padding. */
static inline f32 cheese_style_get_pad_left(const cheese_style_t *style) {
  return style->padding.left;
}

//
//
//

/** @brief Get the bottom padding. */
static inline f32 cheese_style_get_pad_bottom(const cheese_style_t *style) {
  return style->padding.bottom;
}

//
//
//

/** @brief Get the top padding. */
static inline f32 cheese_style_get_pad_top(const cheese_style_t *style) {
  return style->padding.top;
}

//
//
//

/** @brief Get the right padding. */
static inline f32 cheese_style_get_pad_right(const cheese_style_t *style) {
  return style->padding.right;
}

//
//
//

/** @brief Get the left margin. */
static inline f32 cheese_style_get_margin_left(const cheese_style_t *style) {
  return style->margin.left;
}

//
//
//

/** @brief Get the bottom margin. */
static inline f32 cheese_style_get_margin_bottom(const cheese_style_t *style) {
  return style->margin.bottom;
}

//
//
//

/** @brief Get the top margin. */
static inline f32 cheese_style_get_margin_top(const cheese_style_t *style) {
  return style->margin.top;
}

//
//
//

/** @brief Get the right margin. */
static inline f32 cheese_style_get_margin_right(const cheese_style_t *style) {
  return style->margin.right;
}

//
//
//

/** @brief Get the combined top + bottom margin. */
static inline f32 cheese_style_get_margin_height(const cheese_style_t *style) {
  return cheese_style_get_margin_top(style) +
         cheese_style_get_margin_bottom(style);
}

//
//
//

/** @brief Get the combined left + right margin. */
static inline f32 cheese_style_get_margin_width(const cheese_style_t *style) {
  return cheese_style_get_margin_left(style) +
         cheese_style_get_margin_right(style);
}

#endif // !CHEESE_CORE_STYLE_H
