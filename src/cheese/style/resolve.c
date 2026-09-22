/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/anim.h>

#include <cheese/style/internal.h>
#include <cheese/style/resolve.h>
#include <cheese/style/value.h>

/***********************************/

static stringmap_t *cheese_style_map(cheese_t *cheese) {
  stringmap_t **map = &cheese->class_styles;
  if (!*map && cheese->frame_arena)
    *map = sm_new(cheese->frame_arena, 8);
  return *map;
}

//
//
//

/**
 * @brief Apply one lexical scope to @c style.
 * @details Copies set style values, then merges the scope's class names.
 * Used by both resolvers so the style and class belonging to one scope stay
 * together.
 *
 * @param cheese The cheese context.
 * @param style The style to modify in place.
 * @param scope The scope to apply.
 */
static void cheese_style_apply_scope(cheese_t *cheese, cheese_style_t *style,
                                     const cheese_style_scope_t *scope) {
  cheese_style_override(cheese->frame_arena, style, &scope->style);
  cheese_style_apply_classes(cheese, style, scope->classes);
}

//
//
//

void cheese_push_scope(cheese_t *cheese, cheese_style_t style,
                       const cstr *classes) {
  if (cheese->scope_depth < CHEESE_STACK_MAX_DEPTH - 1) {
    cheese->scope_stack[cheese->scope_depth++] =
        (cheese_style_scope_t){style, classes};
    cheese->style_dirty = true;
    return;
  }

  cheese_log_error("cheese_push_scope: Style scope is full, increase max "
                   "stack depth by defining CHEESE_STACK_MAX_DEPTH");
  return;
}

void cheese_pop_scope(cheese_t *cheese) {
  if (cheese->scope_depth > 0) {
    cheese->scope_depth--;
    cheese->style_dirty = true;
  }
}

void cheese_push_style(cheese_t *cheese, cheese_style_t style) {
  cheese_push_scope(cheese, style, null);
}

void cheese_pop_style(cheese_t *cheese) { cheese_pop_scope(cheese); }

void cheese_style_class_register(cheese_t *cheese, const cstr *name,
                                 cheese_style_t style) {
  if (!cheese || !name) {
    cheese_log_error("cheese_style_class_register: Invalid parameters");
    return;
  }

  stringmap_t *map = cheese_style_map(cheese);
  if (!map) {
    cheese_log_error("cheese_style_class_register: Failed to create map");
    return;
  }

  sm_insert(map, string_from_cstr(cheese->frame_arena, name), &style);
}

void cheese_style_apply_classes(cheese_t *cheese, cheese_style_t *style,
                                const cstr *classes) {
  if (!cheese || !style) {
    cheese_log_error("cheese_style_apply_classes: Invalid parameters");
    return;
  }

  if (!classes || !*classes)
    return;

  const cstr *p = classes;
  while (*p) {
    while (*p == ' ')
      p++;
    if (!*p)
      break;

    const cstr *start = p;
    while (*p && *p != ' ')
      p++;

    u32 len = (u32)(p - start);
    char buf[128];

    if (len >= sizeof(buf))
      len = sizeof(buf) - 1;

    memcpy(buf, start, len);
    buf[len] = '\0';

    cheese_style_t *cls = null;
    if (cheese->class_styles)
      cls = sm_get(cheese->class_styles,
                   string_from_cstr(cheese->frame_arena, buf));

    if (cls)
      cheese_style_override(cheese->frame_arena, style, cls);
    else
      cheese_log_debug("cheese_style_apply_classes: Class '%s' not found", buf);
  }
}

void cheese_style_apply_state(cheese_style_t *style, u32 state) {
  if (!style)
    return;

  cheese_color_t state_color = 0;
  if (state & CHEESE_STATE_DISABLED)
    state_color = style->disabled_color;
  if (!state_color && (state & CHEESE_STATE_PRESSED))
    state_color = style->pressed_color;
  if (!state_color && (state & CHEESE_STATE_HOVERED))
    state_color = style->hover_color;
  if (!state_color && (state & CHEESE_STATE_FOCUSED))
    state_color = style->focus_color;

  if (state_color) {
    style->bg_color = state_color;
    return;
  }

  f32 alpha = cheese_style_state_layer_alpha(state);
  if (alpha > 0.0f && style->state_layer_color && style->bg_color)
    style->bg_color =
        cheese_color_lerp(style->bg_color, style->state_layer_color, alpha);
}

void cheese_style_resolve(cheese_t *cheese, cheese_style_t *out,
                          const cstr *classes) {
  cheese_style_resolve_scoped(cheese, out, CHEESE_ROLE_NONE, classes, null);
}

void cheese_style_role_register(cheese_t *cheese, cheese_role_t role,
                                cheese_style_t style) {
  if (!cheese || role <= CHEESE_ROLE_NONE || role >= CHEESE_ROLE_MAX)
    return;

  cheese->role_styles[role] = style;
  cheese->role_set[role] = true;
}

void cheese_style_resolve_scoped(cheese_t *cheese, cheese_style_t *out,
                                 cheese_role_t role, const cstr *classes,
                                 const cheese_style_t *explicit) {
  if (!cheese || !out)
    return;

  *out = cheese_style_new();

  if (cheese->scope_depth > 0)
    cheese_style_apply_scope(cheese, out, &cheese->scope_stack[0]);

  if (role > CHEESE_ROLE_NONE && role < CHEESE_ROLE_MAX &&
      cheese->role_set[role])
    cheese_style_override(cheese->frame_arena, out, &cheese->role_styles[role]);

  for (u32 i = 1; i < cheese->scope_depth; i++)
    cheese_style_apply_scope(cheese, out, &cheese->scope_stack[i]);

  cheese_style_apply_classes(cheese, out, classes);

  if (explicit)
    cheese_style_override(cheese->frame_arena, out, explicit);

  cheese->resolved_style = *out;
  cheese->style_dirty = false;
}
