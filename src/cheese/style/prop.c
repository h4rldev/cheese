/***********************************/

#include <htils/basictypes.h>
#include <htils/string.h>
#include <htils/stringmap.h>

#include <cheese/types.h>

#include <cheese/style/internal.h>
#include <cheese/style/prop.h>

/***********************************/

u32 cheese_style_prop_register(cheese_t *cheese, const cstr *name,
                               cheese_prop_kind_t kind) {
  if (!cheese || !name || !*name)
    return 0;

  if (!cheese->props) {
    if (!cheese->arena)
      return 0;

    cheese->props = sm_new(cheese->arena, 16);
  }

  string *key = string_from_cstr(cheese->arena, name);
  cheese_style_prop_def_t *existing = sm_get(cheese->props, key);
  if (existing)
    return existing->id;

  cheese_style_prop_def_t def = {.id = cheese->prop_next++, .kind = kind};
  sm_insert(cheese->props, key, &def);
  return def.id;
}

void cheese_style_prop_register_core(cheese_t *cheese) {
  if (!cheese)
    return;

  cheese->core_props.border_color = cheese_style_prop_register(
      cheese, CHEESE_PROP_BORDER_COLOR, CHEESE_PROP_COLOR);
  cheese->core_props.border_width = cheese_style_prop_register(
      cheese, CHEESE_PROP_BORDER_WIDTH, CHEESE_PROP_F32);
  cheese->core_props.border_sides = cheese_style_prop_register(
      cheese, CHEESE_PROP_BORDER_SIDES, CHEESE_PROP_U32);
  cheese->core_props.focus_ring_color = cheese_style_prop_register(
      cheese, CHEESE_PROP_FOCUS_RING_COLOR, CHEESE_PROP_COLOR);
  cheese->core_props.focus_ring_width = cheese_style_prop_register(
      cheese, CHEESE_PROP_FOCUS_RING_WIDTH, CHEESE_PROP_F32);
  cheese->core_props.focus_ring_offset = cheese_style_prop_register(
      cheese, CHEESE_PROP_FOCUS_RING_OFFSET, CHEESE_PROP_F32);
  cheese->core_props.bg_gradient = cheese_style_prop_register(
      cheese, CHEESE_PROP_BG_GRADIENT, CHEESE_PROP_PTR);
  cheese->core_props.opacity =
      cheese_style_prop_register(cheese, CHEESE_PROP_OPACITY, CHEESE_PROP_F32);
}

void cheese_style_prop_set(cheese_t *cheese, cheese_style_t *style, u32 prop,
                           cheese_prop_t value) {
  if (!cheese || !style || prop == 0)
    return;

  value.id = prop;
  cheese_style_prop_put(cheese->frame_arena, style, value);
}

void cheese_style_prop_set_color(cheese_t *cheese, cheese_style_t *style,
                                 u32 prop, cheese_color_t color) {
  cheese_style_prop_set(
      cheese, style, prop,
      (cheese_prop_t){.kind = CHEESE_PROP_COLOR, .color = color});
}

void cheese_style_prop_set_f32(cheese_t *cheese, cheese_style_t *style,
                               u32 prop, f32 value) {
  cheese_style_prop_set(cheese, style, prop,
                        (cheese_prop_t){.kind = CHEESE_PROP_F32, .f32 = value});
}

void cheese_style_prop_set_u32(cheese_t *cheese, cheese_style_t *style,
                               u32 prop, u32 value) {
  cheese_style_prop_set(cheese, style, prop,
                        (cheese_prop_t){.kind = CHEESE_PROP_U32, .u32 = value});
}

void cheese_style_prop_set_gradient(cheese_t *cheese, cheese_style_t *style,
                                    u32 prop, cheese_gradient_t gradient) {
  if (!cheese || !style || prop == 0 || !cheese->frame_arena)
    return;

  cheese_gradient_t *copy =
      arena_alloc(cheese->frame_arena, cheese_gradient_t, 1);
  *copy = gradient;

  cheese_style_prop_set(cheese, style, prop,
                        (cheese_prop_t){.kind = CHEESE_PROP_PTR, .ptr = copy});
}

b32 cheese_style_prop_get(const cheese_style_t *style, u32 prop,
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

cheese_color_t cheese_style_prop_get_color(const cheese_style_t *style,
                                           u32 prop, cheese_color_t fallback) {
  cheese_prop_t value;
  if (cheese_style_prop_get(style, prop, &value) &&
      value.kind == CHEESE_PROP_COLOR)
    return value.color;

  return fallback;
}

f32 cheese_style_prop_get_f32(const cheese_style_t *style, u32 prop,
                              f32 fallback) {
  cheese_prop_t value;
  if (cheese_style_prop_get(style, prop, &value) &&
      value.kind == CHEESE_PROP_F32)
    return value.f32;

  return fallback;
}

u32 cheese_style_prop_get_u32(const cheese_style_t *style, u32 prop,
                              u32 fallback) {
  cheese_prop_t value;
  if (cheese_style_prop_get(style, prop, &value) &&
      value.kind == CHEESE_PROP_U32)
    return value.u32;

  return fallback;
}

cheese_gradient_t cheese_style_prop_get_gradient(const cheese_style_t *style,
                                                 u32 prop,
                                                 cheese_gradient_t fallback) {
  cheese_prop_t value;
  if (cheese_style_prop_get(style, prop, &value) &&
      value.kind == CHEESE_PROP_PTR) {
    return *(cheese_gradient_t *)value.ptr;
  }

  return fallback;
}
