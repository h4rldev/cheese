#ifndef CHEESE_STYLE_VALUE_H
#define CHEESE_STYLE_VALUE_H

/***********************************/

#include <htils/arena.h>
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

/**
 * @note Needs documentation.
 */
void cheese_style_merge(arena_t *arena, cheese_style_t *dest,
                        const cheese_style_t *src);

//
//
//

/**
 * @not Needs documentation.
 */
void cheese_style_override(arena_t *arena, cheese_style_t *des,
                           const cheese_style_t *src);

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

#endif // !CHEESE_STYLE_VALUE_H
