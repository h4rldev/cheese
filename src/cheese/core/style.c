/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/darray.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/anim.h>
#include <cheese/core/style.h>

/***********************************/

typedef struct {
  u32 id;
  u8 kind;
} cheese_prop_def_t;

//
//
//

static cheese_prop_t *style_prop_find(cheese_style_t *style, u32 prop) {
  if (!style)
    return null;

  for (u32 i = 0; i < style->prop_count; i++)
    if (style->props[i].id == prop)
      return &style->props[i];

  return null;
}

//
//
//

static void style_prop_put(arena_t *arena, cheese_style_t *style,
                           cheese_prop_t value) {
  if (!arena || !style || value.id == 0)
    return;

  u32 n = style->prop_count;
  u32 out = 0;

  cheese_prop_t *grown = arena_alloc(arena, cheese_prop_t, n + 1);
  for (u32 i = 0; i < n; i++) {
    if (style->props[i].id == value.id)
      continue;

    grown[out++] = style->props[i];
  }

  grown[out++] = value;

  style->props = grown;
  style->prop_count = out;
}

//
//
//

static void cheese_style_merge(arena_t *arena, cheese_style_t *dest,
                               const cheese_style_t *src) {
  if (dest->bg_color == 0)
    dest->bg_color = src->bg_color;
  if (dest->hover_color == 0)
    dest->hover_color = src->hover_color;
  if (dest->pressed_color == 0)
    dest->pressed_color = src->pressed_color;
  if (dest->disabled_color == 0)
    dest->disabled_color = src->disabled_color;
  if (dest->focus_color == 0)
    dest->focus_color = src->focus_color;
  if (dest->text_color == 0)
    dest->text_color = src->text_color;
  if (dest->state_layer_color == 0)
    dest->state_layer_color = src->state_layer_color;

  if (dest->corner_radius.top_left < 0.0f)
    dest->corner_radius.top_left = src->corner_radius.top_left;
  if (dest->corner_radius.top_right < 0.0f)
    dest->corner_radius.top_right = src->corner_radius.top_right;
  if (dest->corner_radius.bottom_right < 0.0f)
    dest->corner_radius.bottom_right = src->corner_radius.bottom_right;
  if (dest->corner_radius.bottom_left < 0.0f)
    dest->corner_radius.bottom_left = src->corner_radius.bottom_left;

  if (dest->padding.left < 0.0f)
    dest->padding.left = src->padding.left;
  if (dest->padding.bottom < 0.0f)
    dest->padding.bottom = src->padding.bottom;
  if (dest->padding.top < 0.0f)
    dest->padding.top = src->padding.top;
  if (dest->padding.right < 0.0f)
    dest->padding.right = src->padding.right;

  if (dest->margin.left < 0.0f)
    dest->margin.left = src->margin.left;
  if (dest->margin.right < 0.0f)
    dest->margin.right = src->margin.right;
  if (dest->margin.top < 0.0f)
    dest->margin.top = src->margin.top;
  if (dest->margin.bottom < 0.0f)
    dest->margin.bottom = src->margin.bottom;

  if (dest->font_size == 0)
    dest->font_size = src->font_size;
  if (dest->cursor < 0)
    dest->cursor = src->cursor;
  if (dest->selectable < 0)
    dest->selectable = src->selectable;

  for (u32 i = 0; i < src->prop_count; i++) {
    const cheese_prop_t *prop = &src->props[i];
    if (style_prop_find(dest, prop->id))
      continue;

    style_prop_put(arena, dest, *prop);
  }
}

//
//
//

static void cheese_style_override(arena_t *arena, cheese_style_t *dest,
                                  const cheese_style_t *src) {
  if (src->bg_color)
    dest->bg_color = src->bg_color;
  if (src->hover_color)
    dest->hover_color = src->hover_color;
  if (src->pressed_color)
    dest->pressed_color = src->pressed_color;
  if (src->disabled_color)
    dest->disabled_color = src->disabled_color;
  if (src->focus_color)
    dest->focus_color = src->focus_color;
  if (src->text_color)
    dest->text_color = src->text_color;
  if (src->state_layer_color)
    dest->state_layer_color = src->state_layer_color;

  if (src->corner_radius.top_left >= 0.0f)
    dest->corner_radius.top_left = src->corner_radius.top_left;
  if (src->corner_radius.top_right >= 0.0f)
    dest->corner_radius.top_right = src->corner_radius.top_right;
  if (src->corner_radius.bottom_right >= 0.0f)
    dest->corner_radius.bottom_right = src->corner_radius.bottom_right;
  if (src->corner_radius.bottom_left >= 0.0f)
    dest->corner_radius.bottom_left = src->corner_radius.bottom_left;

  if (src->padding.left >= 0.0f)
    dest->padding.left = src->padding.left;
  if (src->padding.right >= 0.0f)
    dest->padding.right = src->padding.right;
  if (src->padding.top >= 0.0f)
    dest->padding.top = src->padding.top;
  if (src->padding.bottom >= 0.0f)
    dest->padding.bottom = src->padding.bottom;

  if (src->margin.left >= 0.0f)
    dest->margin.left = src->margin.left;
  if (src->margin.right >= 0.0f)
    dest->margin.right = src->margin.right;
  if (src->margin.top >= 0.0f)
    dest->margin.top = src->margin.top;
  if (src->margin.bottom >= 0.0f)
    dest->margin.bottom = src->margin.bottom;

  if (src->font_size)
    dest->font_size = src->font_size;

  if (src->cursor >= 0)
    dest->cursor = src->cursor;

  if (src->selectable >= 0)
    dest->selectable = src->selectable;

  for (u32 i = 0; i < src->prop_count; i++)
    style_prop_put(arena, dest, src->props[i]);
}

//
//
//

static stringmap_t *cheese_style_map(cheese_t *cheese) {
  stringmap_t **map = &cheese->class_styles;
  if (!*map && cheese->frame_arena)
    *map = sm_new(cheese->frame_arena, 8);
  return *map;
}

//
//
//

static inline void warn_if_color_will_get_inherited(const cstr *scope) {
  cheese_log_warning("%s: Color set to 0 will be overwritten by inheritance",
                     scope);
}

//
//
//

static inline void warn_if_float_will_get_inherited(const cstr *scope) {
  cheese_log_warning("%s: Negative floats will be overwritten by inheritance",
                     scope);
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

cheese_style_t cheese_style_new(void) {
  return (cheese_style_t){
      .bg_color = 0,
      .hover_color = 0,
      .pressed_color = 0,
      .disabled_color = 0,
      .focus_color = 0,
      .text_color = 0,
      .state_layer_color = 0,

      .corner_radius = {-1.0f, -1.0f, -1.0f, -1.0f},
      .padding = {-1.0f, -1.0f, -1.0f, -1.0f},
      .margin = {-1.0f, -1.0f, -1.0f, -1.0f},

      .font_size = 0,

      .cursor = -1,
      .selectable = -1,

      .props = null,
      .prop_count = 0,
  };
}

void cheese_style_set_bg_color(cheese_style_t *style, cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_bg_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_bg_color");

  style->bg_color = color;
}

void cheese_style_set_hover_color(cheese_style_t *style, cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_hover_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_hover_color");

  style->hover_color = color;
}

void cheese_style_set_pressed_color(cheese_style_t *style,
                                    cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_pressed_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_pressed_color");

  style->pressed_color = color;
}

void cheese_style_set_disabled_color(cheese_style_t *style,
                                     cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_disabled_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_disabled_color");

  style->disabled_color = color;
}

void cheese_style_set_focus_color(cheese_style_t *style, cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_focus_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_focus_color");

  style->focus_color = color;
}

void cheese_style_set_text_color(cheese_style_t *style, cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_text_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_text_color");

  style->text_color = color;
}

void cheese_style_set_state_layer_color(cheese_style_t *style,
                                        cheese_color_t color) {
  if (!style) {
    cheese_log_error("cheese_style_set_state_layer_color: Invalid style");
    return;
  }

  if (color == 0)
    warn_if_color_will_get_inherited("cheese_style_set_state_layer_color");

  style->state_layer_color = color;
}

void cheese_style_set_corner_radius_inherit(cheese_style_t *style) {
  if (!style) {
    cheese_log_error("cheese_style_set_corner_radius_inherit: Invalid style");
    return;
  }

  style->corner_radius = (cheese_corners_t){-1.0f, -1.0f, -1.0f, -1.0f};
}

void cheese_style_set_corner_radius_uniform(cheese_style_t *style, f32 radius) {
  if (!style) {
    cheese_log_error("cheese_style_set_corner_radius_uniform: Invalid style");
    return;
  }

  if (radius < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_corner_radius_uniform");

  style->corner_radius.top_left = radius;
  style->corner_radius.top_right = radius;
  style->corner_radius.bottom_left = radius;
  style->corner_radius.bottom_right = radius;
}

void cheese_style_set_corner_radius(cheese_style_t *style, f32 top_left,
                                    f32 top_right, f32 bottom_left,
                                    f32 bottom_right) {
  if (!style) {
    cheese_log_error("cheese_style_set_corner_radius: Invalid style");
    return;
  }

  if (top_left < 0.0f || top_right < 0.0f || bottom_left < 0.0f ||
      bottom_right < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_corner_radius");

  style->corner_radius.top_left = top_left;
  style->corner_radius.top_right = top_right;
  style->corner_radius.bottom_left = bottom_left;
  style->corner_radius.bottom_right = bottom_right;
}

void cheese_style_set_padding_inherit(cheese_style_t *style) {
  if (!style) {
    cheese_log_error("cheese_style_set_padding_inherit: Invalid style");
    return;
  }

  style->padding = (cheese_edges_t){-1.0f, -1.0f, -1.0f, -1.0f};
}

void cheese_style_set_padding_uniform(cheese_style_t *style, f32 padding) {
  if (!style) {
    cheese_log_error("cheese_style_set_padding_uniform: Invalid style");
    return;
  }

  if (padding < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_padding_uniform");

  style->padding.left = padding;
  style->padding.right = padding;
  style->padding.top = padding;
  style->padding.bottom = padding;
}

void cheese_style_set_padding(cheese_style_t *style, f32 padding_left,
                              f32 padding_bottom, f32 padding_top,
                              f32 padding_right) {
  if (!style) {
    cheese_log_error("cheese_style_set_padding: Invalid style");
    return;
  }

  if (padding_left < 0.0f || padding_bottom < 0.0f || padding_top < 0.0f ||
      padding_right < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_padding");

  style->padding.left = padding_left;
  style->padding.right = padding_right;
  style->padding.top = padding_top;
  style->padding.bottom = padding_bottom;
}

void cheese_style_set_margin_inherit(cheese_style_t *style) {
  if (!style) {
    cheese_log_error("cheese_style_set_margin_inherit: Invalid style");
    return;
  }

  style->margin = (cheese_edges_t){-1.0f, -1.0f, -1.0f, -1.0f};
}

void cheese_style_set_margin_uniform(cheese_style_t *style, f32 margin) {
  if (!style) {
    cheese_log_error("cheese_style_set_margin_uniform: Invalid style");
    return;
  }

  if (margin < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_margin_uniform");

  style->margin.left = margin;
  style->margin.right = margin;
  style->margin.top = margin;
  style->margin.bottom = margin;
}

void cheese_style_set_margin(cheese_style_t *style, f32 margin_left,
                             f32 margin_bottom, f32 margin_top,
                             f32 margin_right) {
  if (!style) {
    cheese_log_error("cheese_style_set_margin: Invalid style");
    return;
  }

  if (margin_left < 0.0f || margin_bottom < 0.0f || margin_top < 0.0f ||
      margin_right < 0.0f)
    warn_if_float_will_get_inherited("cheese_style_set_margin");

  style->margin.left = margin_left;
  style->margin.bottom = margin_bottom;
  style->margin.top = margin_top;
  style->margin.right = margin_right;
}

void cheese_style_set_cursor(cheese_style_t *style, cheese_cursor_t cursor) {
  if (!style)
    return;

  if (cursor < CHEESE_CURSOR_DEFAULT || cursor >= CHEESE_CURSOR_MAX)
    cheese_log_warning("cheese_style_set_cursor: Invalid cursor");

  style->cursor = (i32)cursor;
}

void cheese_style_set_cursor_inherit(cheese_style_t *style) {
  if (!style)
    return;

  style->cursor = -1;
}

void cheese_style_set_selectable(cheese_style_t *style, b32 selectable) {
  if (!style)
    return;

  style->selectable = selectable ? 1 : 0;
}

void cheese_style_set_selectable_inherit(cheese_style_t *style) {
  if (!style)
    return;

  style->selectable = -1;
}

void cheese_style_set_font_size(cheese_style_t *style, u32 font_size) {
  if (!style) {
    cheese_log_error("cheese_style_set_font_size: Invalid style");
    return;
  }

  if (font_size == 0)
    cheese_log_warning("cheese_style_set_font_size: A font-size of 0 will be "
                       "overwritten by inheritance");

  style->font_size = font_size;
}

u32 cheese_prop_register(cheese_t *cheese, const cstr *name,
                         cheese_prop_kind_t kind) {
  if (!cheese || !name || !*name)
    return 0;

  if (!cheese->props) {
    if (!cheese->arena)
      return 0;

    cheese->props = sm_new(cheese->arena, 16);
  }

  string *key = string_from_cstr(cheese->arena, name);
  cheese_prop_def_t *existing = sm_get(cheese->props, key);
  if (existing)
    return existing->id;

  cheese_prop_def_t def = {.id = cheese->prop_next++, .kind = kind};
  sm_insert(cheese->props, key, &def);
  return def.id;
}

void cheese_style_set_prop(cheese_t *cheese, cheese_style_t *style, u32 prop,
                           cheese_prop_t value) {
  if (!cheese || !style || prop == 0)
    return;

  value.id = prop;
  style_prop_put(cheese->frame_arena, style, value);
}

void cheese_style_set_prop_color(cheese_t *cheese, cheese_style_t *style,
                                 u32 prop, cheese_color_t color) {
  cheese_style_set_prop(
      cheese, style, prop,
      (cheese_prop_t){.kind = CHEESE_PROP_COLOR, .color = color});
}

void cheese_style_set_prop_f32(cheese_t *cheese, cheese_style_t *style,
                               u32 prop, f32 value) {
  cheese_style_set_prop(cheese, style, prop,
                        (cheese_prop_t){.kind = CHEESE_PROP_F32, .f32 = value});
}

void cheese_style_set_prop_u32(cheese_t *cheese, cheese_style_t *style,
                               u32 prop, u32 value) {
  cheese_style_set_prop(cheese, style, prop,
                        (cheese_prop_t){.kind = CHEESE_PROP_U32, .u32 = value});
}

void cheese_style_set_prop_gradient(cheese_t *cheese, cheese_style_t *style,
                                    u32 prop, cheese_gradient_t gradient) {
  if (!cheese || !style || prop == 0 || !cheese->frame_arena)
    return;

  cheese_gradient_t *copy =
      arena_alloc(cheese->frame_arena, cheese_gradient_t, 1);
  *copy = gradient;

  cheese_style_set_prop(cheese, style, prop,
                        (cheese_prop_t){.kind = CHEESE_PROP_PTR, .ptr = copy});
}

b32 cheese_style_get_prop(const cheese_style_t *style, u32 prop,
                          cheese_prop_t *out) {
  if (!style || prop == 0)
    return false;

  for (u32 i = 0; i < style->prop_count; i++) {
    if (style->props[i].id != prop)
      continue;

    if (out)
      *out = style->props[i];

    return true;
  }

  return false;
}

cheese_color_t cheese_style_get_prop_color(const cheese_style_t *style,
                                           u32 prop, cheese_color_t fallback) {
  cheese_prop_t value;
  if (cheese_style_get_prop(style, prop, &value) &&
      value.kind == CHEESE_PROP_COLOR)
    return value.color;

  return fallback;
}

f32 cheese_style_get_prop_f32(const cheese_style_t *style, u32 prop,
                              f32 fallback) {
  cheese_prop_t value;
  if (cheese_style_get_prop(style, prop, &value) &&
      value.kind == CHEESE_PROP_F32)
    return value.f32;

  return fallback;
}

u32 cheese_style_get_prop_u32(const cheese_style_t *style, u32 prop,
                              u32 fallback) {
  cheese_prop_t value;
  if (cheese_style_get_prop(style, prop, &value) &&
      value.kind == CHEESE_PROP_U32)
    return value.u32;

  return fallback;
}

cheese_gradient_t cheese_style_get_prop_gradient(const cheese_style_t *style,
                                                 u32 prop,
                                                 cheese_gradient_t fallback) {
  cheese_prop_t value;
  if (cheese_style_get_prop(style, prop, &value) &&
      value.kind == CHEESE_PROP_PTR) {
    return *(cheese_gradient_t *)value.ptr;
  }

  return fallback;
}

void cheese_style_register_core_props(cheese_t *cheese) {
  if (!cheese)
    return;

  cheese->core_props.border_color =
      cheese_prop_register(cheese, CHEESE_PROP_BORDER_COLOR, CHEESE_PROP_COLOR);
  cheese->core_props.border_width =
      cheese_prop_register(cheese, CHEESE_PROP_BORDER_WIDTH, CHEESE_PROP_F32);
  cheese->core_props.border_sides =
      cheese_prop_register(cheese, CHEESE_PROP_BORDER_SIDES, CHEESE_PROP_U32);
  cheese->core_props.focus_ring_color = cheese_prop_register(
      cheese, CHEESE_PROP_FOCUS_RING_COLOR, CHEESE_PROP_COLOR);
  cheese->core_props.focus_ring_width = cheese_prop_register(
      cheese, CHEESE_PROP_FOCUS_RING_WIDTH, CHEESE_PROP_F32);
  cheese->core_props.focus_ring_offset = cheese_prop_register(
      cheese, CHEESE_PROP_FOCUS_RING_OFFSET, CHEESE_PROP_F32);
  cheese->core_props.bg_gradient =
      cheese_prop_register(cheese, CHEESE_PROP_BG_GRADIENT, CHEESE_PROP_PTR);
  cheese->core_props.opacity =
      cheese_prop_register(cheese, CHEESE_PROP_OPACITY, CHEESE_PROP_F32);
}

cheese_style_t *cheese_current_style(cheese_t *cheese) {
  if (cheese->style_dirty) {
    cheese_style_t resolved = cheese_style_new();
    for (u32 i = cheese->scope_depth; i-- > 0;)
      cheese_style_merge(cheese->frame_arena, &resolved,
                         &cheese->scope_stack[i].style);

    cheese->resolved_style = resolved;
    cheese->style_dirty = false;
  }

  return &cheese->resolved_style;
}

void cheese_push_style(cheese_t *cheese, cheese_style_t style) {
  cheese_push_scope(cheese, style, null);
}

void cheese_pop_style(cheese_t *cheese) { cheese_pop_scope(cheese); }

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

f32 cheese_style_state_layer_alpha(u32 state) {
  if (state & CHEESE_STATE_DISABLED)
    return 0.12f;
  if (state & CHEESE_STATE_PRESSED)
    return 0.12f;
  if (state & CHEESE_STATE_HOVERED)
    return 0.08f;
  if (state & CHEESE_STATE_FOCUSED)
    return 0.10f;

  return 0.0f;
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
