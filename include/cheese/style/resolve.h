#ifndef CHEESE_STYLE_RESOLVE_H
#define CHEESE_STYLE_RESOLVE_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

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

#endif // !CHEESE_STYLE_RESOLVE_H
