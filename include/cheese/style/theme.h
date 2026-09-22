#ifndef CHEESE_STYLE_THEME_H
#define CHEESE_STYLE_THEME_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief The built-in dark palette.
 *
 * @return A dark @ref cheese_theme_t.
 */
cheese_theme_t cheese_theme_dark(void);

//
//
//

/**
 * @brief The built-in light palette.
 *
 * @return A light @ref cheese_theme_t.
 */
cheese_theme_t cheese_theme_light(void);

//
//
//

/**
 * @brief Set the active palette for every following frame.
 * @details Call once after @ref cheese_default; the palette persists, and
 * @ref cheese_begin re-registers its built-in role-class styles each frame.
 * @ref cheese_theme_palette returns what is active.
 *
 * @param cheese The cheese context.
 * @param theme The palette to apply; copied, so it need not outlive the call.
 *
 * @pre @c cheese and @c theme must be valid and cannot be `null`.
 */
void cheese_theme_apply(cheese_t *cheese, const cheese_theme_t *theme);

//
//
//

/**
 * @brief The active palette, or null when none was applied.
 *
 * @param cheese The cheese context.
 *
 * @return The palette set by @ref cheese_theme_apply, or null.
 */
const cheese_theme_t *cheese_theme_palette(const cheese_t *cheese);

//
//
//

/**
 * @brief Re-register the active palette's built-in classes for a new frame.
 * @internal Called from @ref cheese_begin; not for consumer use.
 *
 * @param cheese The cheese context.
 */
void cheese_theme_frame_begin(cheese_t *cheese);

//
//
//

/**
 * @brief Set a theme variable, overwriting any existing value.
 * @details The theme is per-frame: declare it after @ref cheese_begin (ideally
 * from a bound state so it reacts) and read it while building styles. A lookup
 * with no matching variable returns the caller's fallback.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param value The value to store.
 *
 * @pre @c cheese and @c name must be valid and cannot be `null`.
 */
void cheese_theme_set_color(cheese_t *cheese, const cstr *name,
                            cheese_color_t value);

//
//
//

/**
 * @brief Set an f32 theme variable, overwriting any existing value.
 * @details The theme is per-frame: declare it after @ref cheese_begin (ideally
 * from a bound state so it reacts) and read it while building styles. A lookup
 * with no matching variable returns the caller's fallback.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param value The f32 to store.
 *
 * @pre @c cheese and @c name must be valid and cannot be `null`.
 */
void cheese_theme_set_f32(cheese_t *cheese, const cstr *name, f32 value);

//
//
//

/**
 * @brief Set a u32 theme variable, overwriting any existing value.
 * @details The theme is per-frame: declare it after @ref cheese_begin (ideally
 * from a bound state so it reacts) and read it while building styles. A lookup
 * with no matching variable returns the caller's fallback.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param value The u32 to store.
 *
 * @pre @c cheese and @c name must be valid and cannot be `null`.
 */
void cheese_theme_set_u32(cheese_t *cheese, const cstr *name, u32 value);

//
//
//

/**
 * @brief Set an i32 theme variable, overwriting any existing value.
 * @details The theme is per-frame: declare it after @ref cheese_begin (ideally
 * from a bound state so it reacts) and read it while building styles. A lookup
 * with no matching variable returns the caller's fallback.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param value The i32 to store.
 *
 * @pre @c cheese and @c name must be valid and cannot be `null`.
 */
void cheese_theme_set_i32(cheese_t *cheese, const cstr *name, i32 value);

//
//
//

/**
 * @brief Set a b32 theme variable, overwriting any existing value.
 * @details The theme is per-frame: declare it after @ref cheese_begin (ideally
 * from a bound state so it reacts) and read it while building styles. A lookup
 * with no matching variable returns the caller's fallback.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param value The b32 to store.
 *
 * @pre @c cheese and @c name must be valid and cannot be `null`.
 */
void cheese_theme_set_b32(cheese_t *cheese, const cstr *name, b32 value);

//
//
//

/**
 * @brief Read a theme variable, or @c fallback when absent or mistyped.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param fallback The value to return when @c name is absent or mistyped.
 *
 * @return The variable's value, or @c fallback.
 */
cheese_color_t cheese_theme_color(const cheese_t *cheese, const cstr *name,
                                  cheese_color_t fallback);

//
//
//

/**
 * @brief Read an f32 theme variable, or @c fallback when absent or mistyped.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param fallback The value to return when @c name is absent or mistyped.
 *
 * @return The variable's f32, or @c fallback.
 */
f32 cheese_theme_f32(const cheese_t *cheese, const cstr *name, f32 fallback);

//
//
//

/**
 * @brief Read a u32 theme variable, or @c fallback when absent or mistyped.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param fallback The value to return when @c name is absent or mistyped.
 *
 * @return The variable's u32, or @c fallback.
 */
u32 cheese_theme_u32(const cheese_t *cheese, const cstr *name, u32 fallback);

//
//
//

/**
 * @brief Read an i32 theme variable, or @c fallback when absent or mistyped.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param fallback The value to return when @c name is absent or mistyped.
 *
 * @return The variable's i32, or @c fallback.
 */
i32 cheese_theme_i32(const cheese_t *cheese, const cstr *name, i32 fallback);

//
//
//
/**
 * @brief Read a b32 theme variable, or @c fallback when absent or mistyped.
 *
 * @param cheese The cheese context.
 * @param name The variable name.
 * @param fallback The value to return when @c name is absent or mistyped.
 *
 * @return The variable's b32, or @c fallback.
 */
b32 cheese_theme_b32(const cheese_t *cheese, const cstr *name, b32 fallback);

#endif // !CHEESE_STYLE_THEME_H
