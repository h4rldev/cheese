/***********************************/

#include <htils/basictypes.h>
#include <htils/string.h>
#include <htils/stringmap.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/style.h>
#include <cheese/core/theme.h>

/***********************************/

static stringmap_t *cheese_theme_map(cheese_t *cheese) {
  if (!cheese->theme && cheese->frame_arena)
    cheese->theme = sm_new(cheese->frame_arena, 8);

  return cheese->theme;
}

//
//
//

static void cheese_theme_set(cheese_t *cheese, const cstr *name,
                             cheese_theme_value_t value) {
  if (!cheese || !name || !*name) {
    cheese_log_error("cheese_theme_set: Invalid parameters");
    return;
  }

  stringmap_t *map = cheese_theme_map(cheese);
  if (!map)
    return;

  string *key = string_from_cstr(cheese->frame_arena, name);
  sm_insert(map, key, &value);
}

//
//
//

static const cheese_theme_value_t *cheese_theme_get(const cheese_t *cheese,
                                                    const cstr *name) {
  if (!cheese || !cheese->theme || !name)
    return null;

  return sm_get(cheese->theme, string_from_cstr(cheese->frame_arena, name));
}

//
//
//

static void cheese_theme_register_classes(cheese_t *cheese,
                                          const cheese_theme_t *t) {
  cheese_style_t base = cheese_style_new();
  cheese_style_set_text_color(&base, t->on_surface);
  cheese_style_set_prop_color(cheese, &base,
                              cheese->core_props.focus_ring_color, t->primary);
  cheese_style_set_prop_f32(cheese, &base, cheese->core_props.focus_ring_width,
                            2.0f);
  cheese_style_set_prop_f32(cheese, &base, cheese->core_props.focus_ring_offset,
                            2.0f);

  cheese->scope_stack[0].style = base;
  cheese->style_dirty = true;

  cheese_style_t container = cheese_style_new();
  cheese_style_set_bg_color(&container, t->surface);
  cheese_style_set_text_color(&container, t->on_surface);
  cheese_style_set_corner_radius_uniform(&container, t->radius_md);
  cheese_style_set_padding_uniform(&container, t->space_md);
  cheese_style_set_state_layer_color(&container, t->on_surface);
  cheese_style_class_register(cheese, CHEESE_CLASS_CONTAINER, container);

  cheese_style_t scroll = cheese_style_new();
  cheese_style_set_bg_color(&scroll, t->surface);
  cheese_style_set_corner_radius_uniform(&scroll, t->radius_md);
  cheese_style_class_register(cheese, CHEESE_CLASS_SCROLL, scroll);

  cheese_style_t button = cheese_style_new();
  cheese_style_set_bg_color(&button, t->primary);
  cheese_style_set_text_color(&button, t->on_primary);
  cheese_style_set_corner_radius_uniform(&button, t->radius_sm);
  cheese_style_set_padding(&button, t->space_md, t->space_xs, t->space_xs,
                           t->space_md);
  cheese_style_set_state_layer_color(&button, t->on_primary);
  cheese_style_set_cursor(&button, CHEESE_CURSOR_POINTER);
  cheese_style_class_register(cheese, CHEESE_CLASS_BUTTON, button);

  cheese_style_t checkbox = cheese_style_new();
  cheese_style_set_bg_color(&checkbox, t->surface_variant);
  cheese_style_set_text_color(&checkbox, t->on_surface);
  cheese_style_set_prop_color(cheese, &checkbox,
                              cheese->core_props.border_color, t->outline);
  cheese_style_set_prop_f32(cheese, &checkbox, cheese->core_props.border_width,
                            1.0f);
  cheese_style_set_corner_radius_uniform(&checkbox, t->radius_sm);
  cheese_style_set_state_layer_color(&checkbox, t->on_surface);
  cheese_style_class_register(cheese, CHEESE_CLASS_CHECKBOX, checkbox);

  cheese_style_t radio = cheese_style_new();
  cheese_style_set_bg_color(&radio, t->surface_variant);
  cheese_style_set_text_color(&radio, t->on_surface);
  cheese_style_set_prop_color(cheese, &radio, cheese->core_props.border_color,
                              t->outline);
  cheese_style_set_prop_f32(cheese, &radio, cheese->core_props.border_width,
                            1.0f);
  cheese_style_set_state_layer_color(&radio, t->on_surface);
  cheese_style_class_register(cheese, CHEESE_CLASS_RADIO, radio);

  cheese_style_t slider = cheese_style_new();
  cheese_style_set_bg_color(&slider, t->surface_variant);
  cheese_style_set_text_color(&slider, t->primary);
  cheese_style_set_corner_radius_uniform(&slider, t->radius_sm);
  cheese_style_set_state_layer_color(&slider, t->primary);
  cheese_style_class_register(cheese, CHEESE_CLASS_SLIDER, slider);

  cheese_style_t scrollbar = cheese_style_new();
  cheese_style_set_bg_color(&scrollbar, t->surface_variant);
  cheese_style_set_text_color(&scrollbar, t->outline);
  cheese_style_class_register(cheese, CHEESE_CLASS_SCROLLBAR, scrollbar);

  cheese_style_t progress = cheese_style_new();
  cheese_style_set_bg_color(&progress, t->surface_variant);
  cheese_style_set_text_color(&progress, t->primary);
  cheese_style_set_corner_radius_uniform(&progress, t->radius_sm);
  cheese_style_class_register(cheese, CHEESE_CLASS_PROGRESS, progress);

  cheese_style_t text_input = cheese_style_new();
  cheese_style_set_bg_color(&text_input, t->surface_variant);
  cheese_style_set_text_color(&text_input, t->on_surface);
  cheese_style_set_prop_color(cheese, &text_input,
                              cheese->core_props.border_color, t->outline);
  cheese_style_set_prop_f32(cheese, &text_input,
                            cheese->core_props.border_width, 1.0f);
  cheese_style_set_corner_radius_uniform(&text_input, t->radius_sm);
  cheese_style_set_padding(&text_input, t->space_sm, t->space_xs, t->space_xs,
                           t->space_sm);
  cheese_style_set_state_layer_color(&text_input, t->on_surface);
  cheese_style_set_cursor(&text_input, CHEESE_CURSOR_TEXT);
  cheese_style_class_register(cheese, CHEESE_CLASS_TEXT_INPUT, text_input);

  cheese_style_t dropdown = cheese_style_new();
  cheese_style_set_bg_color(&dropdown, t->surface_variant);
  cheese_style_set_text_color(&dropdown, t->on_surface);
  cheese_style_set_prop_color(cheese, &dropdown,
                              cheese->core_props.border_color, t->outline);
  cheese_style_set_prop_f32(cheese, &dropdown, cheese->core_props.border_width,
                            1.0f);
  cheese_style_set_corner_radius_uniform(&dropdown, t->radius_sm);
  cheese_style_set_padding(&dropdown, t->space_md, t->space_xs, t->space_xs,
                           t->space_md);
  cheese_style_set_hover_color(&dropdown, t->primary_container);
  cheese_style_set_cursor(&dropdown, CHEESE_CURSOR_POINTER);
  cheese_style_class_register(cheese, CHEESE_CLASS_DROPDOWN, dropdown);

  cheese_style_t tabs = cheese_style_new();
  cheese_style_set_bg_color(&tabs, t->surface);
  cheese_style_set_text_color(&tabs, t->on_surface_variant);
  cheese_style_set_hover_color(&tabs, t->primary_container);
  cheese_style_set_focus_color(&tabs, t->primary);
  cheese_style_class_register(cheese, CHEESE_CLASS_TABS, tabs);

  cheese_style_t list = cheese_style_new();
  cheese_style_set_bg_color(&list, t->surface);
  cheese_style_set_text_color(&list, t->on_surface);
  cheese_style_class_register(cheese, CHEESE_CLASS_LIST, list);

  cheese_style_t image = cheese_style_new();
  cheese_style_set_bg_color(&image, t->surface_variant);
  cheese_style_set_corner_radius_uniform(&image, t->radius_md);
  cheese_style_class_register(cheese, CHEESE_CLASS_IMAGE, image);

  cheese_style_role_register(cheese, CHEESE_ROLE_CONTAINER, container);
  cheese_style_role_register(cheese, CHEESE_ROLE_BUTTON, button);
  cheese_style_role_register(cheese, CHEESE_ROLE_CHECKBOX, checkbox);
  cheese_style_role_register(cheese, CHEESE_ROLE_RADIO, radio);
  cheese_style_role_register(cheese, CHEESE_ROLE_SLIDER, slider);
  cheese_style_role_register(cheese, CHEESE_ROLE_SCROLLBAR, scrollbar);
  cheese_style_role_register(cheese, CHEESE_ROLE_PROGRESS_BAR, progress);
  cheese_style_role_register(cheese, CHEESE_ROLE_TEXT_INPUT, text_input);
  cheese_style_role_register(cheese, CHEESE_ROLE_IMAGE, image);
  cheese_style_role_register(cheese, CHEESE_ROLE_TAB, tabs);
  cheese_style_role_register(cheese, CHEESE_ROLE_SCROLL, scroll);
  cheese_style_role_register(cheese, CHEESE_ROLE_DROPDOWN, dropdown);
  cheese_style_role_register(cheese, CHEESE_ROLE_LIST, list);
}

//
//
//

void cheese_theme_set_color(cheese_t *cheese, const cstr *name,
                            cheese_color_t value) {
  cheese_theme_value_t v = {.type = CHEESE_THEME_COLOR, .color = value};
  cheese_theme_set(cheese, name, v);
}

void cheese_theme_set_f32(cheese_t *cheese, const cstr *name, f32 value) {
  cheese_theme_value_t v = {.type = CHEESE_THEME_F32, .f32 = value};
  cheese_theme_set(cheese, name, v);
}

void cheese_theme_set_u32(cheese_t *cheese, const cstr *name, u32 value) {
  cheese_theme_value_t v = {.type = CHEESE_THEME_U32, .u32 = value};
  cheese_theme_set(cheese, name, v);
}

void cheese_theme_set_i32(cheese_t *cheese, const cstr *name, i32 value) {
  cheese_theme_value_t v = {.type = CHEESE_THEME_I32, .i32 = value};
  cheese_theme_set(cheese, name, v);
}

void cheese_theme_set_b32(cheese_t *cheese, const cstr *name, b32 value) {
  cheese_theme_value_t v = {.type = CHEESE_THEME_B32, .b32 = value};
  cheese_theme_set(cheese, name, v);
}

cheese_color_t cheese_theme_color(const cheese_t *cheese, const cstr *name,
                                  cheese_color_t fallback) {
  const cheese_theme_value_t *v = cheese_theme_get(cheese, name);
  return (v && v->type == CHEESE_THEME_COLOR) ? v->color : fallback;
}

f32 cheese_theme_f32(const cheese_t *cheese, const cstr *name, f32 fallback) {
  const cheese_theme_value_t *v = cheese_theme_get(cheese, name);
  return (v && v->type == CHEESE_THEME_F32) ? v->f32 : fallback;
}

u32 cheese_theme_u32(const cheese_t *cheese, const cstr *name, u32 fallback) {
  const cheese_theme_value_t *v = cheese_theme_get(cheese, name);
  return (v && v->type == CHEESE_THEME_U32) ? v->u32 : fallback;
}

i32 cheese_theme_i32(const cheese_t *cheese, const cstr *name, i32 fallback) {
  const cheese_theme_value_t *v = cheese_theme_get(cheese, name);
  return (v && v->type == CHEESE_THEME_I32) ? v->i32 : fallback;
}

b32 cheese_theme_b32(const cheese_t *cheese, const cstr *name, b32 fallback) {
  const cheese_theme_value_t *v = cheese_theme_get(cheese, name);
  return (v && v->type == CHEESE_THEME_B32) ? v->b32 : fallback;
}

cheese_theme_t cheese_theme_dark(void) {
  return (cheese_theme_t){
      .background = cheese_color_rgba(20, 18, 24, 255),
      .surface = cheese_color_rgba(28, 27, 31, 255),
      .surface_variant = cheese_color_rgba(43, 41, 48, 255),
      .on_surface = cheese_color_rgba(230, 224, 233, 255),
      .on_surface_variant = cheese_color_rgba(202, 196, 208, 255),
      .outline = cheese_color_rgba(147, 143, 153, 255),

      .primary = cheese_color_rgba(208, 188, 255, 255),
      .on_primary = cheese_color_rgba(56, 30, 114, 255),
      .primary_container = cheese_color_rgba(79, 55, 139, 255),
      .on_primary_container = cheese_color_rgba(234, 221, 255, 255),

      .error = cheese_color_rgba(242, 184, 181, 255),
      .on_error = cheese_color_rgba(96, 20, 16, 255),
      .success = cheese_color_rgba(122, 219, 143, 255),
      .warning = cheese_color_rgba(255, 180, 171, 255),

      .radius_sm = 4.0f,
      .radius_md = 8.0f,
      .radius_lg = 16.0f,
      .space_xs = 4.0f,
      .space_sm = 8.0f,
      .space_md = 12.0f,
      .space_lg = 16.0f,
  };
}

cheese_theme_t cheese_theme_light(void) {
  return (cheese_theme_t){
      .background = cheese_color_rgba(254, 247, 255, 255),
      .surface = cheese_color_rgba(254, 247, 255, 255),
      .surface_variant = cheese_color_rgba(231, 224, 236, 255),
      .on_surface = cheese_color_rgba(29, 27, 32, 255),
      .on_surface_variant = cheese_color_rgba(73, 69, 79, 255),
      .outline = cheese_color_rgba(121, 116, 126, 255),

      .primary = cheese_color_rgba(103, 80, 164, 255),
      .on_primary = cheese_color_rgba(255, 255, 255, 255),
      .primary_container = cheese_color_rgba(234, 221, 255, 255),
      .on_primary_container = cheese_color_rgba(33, 0, 93, 255),

      .error = cheese_color_rgba(179, 38, 30, 255),
      .on_error = cheese_color_rgba(255, 255, 255, 255),
      .success = cheese_color_rgba(46, 125, 50, 255),
      .warning = cheese_color_rgba(237, 108, 2, 255),

      .radius_sm = 4.0f,
      .radius_md = 8.0f,
      .radius_lg = 16.0f,
      .space_xs = 4.0f,
      .space_sm = 8.0f,
      .space_md = 12.0f,
      .space_lg = 16.0f,
  };
}

void cheese_theme_apply(cheese_t *cheese, const cheese_theme_t *theme) {
  if (!cheese || !theme) {
    cheese_log_error("cheese_theme_apply: Invalid parameters");
    return;
  }

  cheese->theme_palette = *theme;
  cheese->theme_applied = true;
}

const cheese_theme_t *cheese_theme_palette(const cheese_t *cheese) {
  return (cheese && cheese->theme_applied) ? &cheese->theme_palette : null;
}

void cheese_theme_frame_begin(cheese_t *cheese) {
  if (!cheese || !cheese->theme_applied || !cheese->frame_arena)
    return;

  cheese_theme_register_classes(cheese, &cheese->theme_palette);
}
