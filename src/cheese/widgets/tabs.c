/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/widgets/tabs.h>
#include <cheese/widgets/widget.h>

/***********************************/

i32 cheese_tab_bar(cheese_t *cheese, const cstr *classes,
                   cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                   cheese_value_t selected, const cstr *const *labels,
                   u32 count, cheese_font_t *font) {
  if (!cheese || !cheese->renderer || count == 0)
    return 0;

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_TAB, classes, null);

  if (font) {
    u32 target_size = style.font_size ? style.font_size : font->default_size;
    cheese_font_set_size(font, target_size);
  }

  i32 sel = cheese_value_i32(selected);
  if (sel < 0)
    sel = 0;
  if ((u32)sel >= count)
    sel = (i32)count - 1;

  f32 pad_x = cheese_style_get_pad_left(&style) >= 0.0f
                  ? cheese_style_get_pad_left(&style)
                  : 12.0f;

  f32 *tab_w = arena_alloc(cheese->frame_arena, f32, count);
  f32 total = 0.0f;
  for (u32 i = 0; i < count; i++) {
    const cstr *label = labels[i] ? labels[i] : "";
    string *label_str = string_from_cstr(cheese->frame_arena, label);
    f32 text_w =
        (font && label[0]) ? cheese_font_measure_text(font, label_str) : 0.0f;

    tab_w[i] = text_w + pad_x * 2.0f;
    if (tab_w[i] < 24.0f)
      tab_w[i] = 24.0f;
    total += tab_w[i];
  }
  if (w > 0.0f && total < w) {
    f32 extra = (w - total) / (f32)count;
    for (u32 i = 0; i < count; i++)
      tab_w[i] += extra;
  }

  cheese_color_t bg = style.bg_color;
  cheese_color_t hover = style.hover_color ? style.hover_color : 0x3B82F640;
  cheese_color_t accent = style.focus_color ? style.focus_color : 0xFFFFFFFF;
  cheese_color_t text_color = style.text_color ? style.text_color : 0x000000FF;

  f32 tx = x;
  for (u32 i = 0; i < count; i++) {
    const cstr *label = labels[i] ? labels[i] : "";
    b32 is_sel = (i32)i == sel;

    cheese_semantics_t tab_sem = {.key = label};

    cheese_widget_t scope;
    cheese_widget_begin(cheese, tab_sem, CHEESE_ROLE_TAB, tx, y, tab_w[i], h,
                        &scope);

    if (is_sel)
      cheese_draw_rect(cheese, style.corner_radius, tx, y, tab_w[i], h, hover);
    else if (scope.hovered && bg)
      cheese_draw_rect(cheese, style.corner_radius, tx, y, tab_w[i], h, bg);

    if (font && label[0]) {
      string *label_str = string_from_cstr(cheese->frame_arena, label);
      f32 text_w = cheese_font_measure_text(font, label_str);
      f32 ty = y + h * 0.5f + font->active_variant->line_height * 0.25f;
      cheese_draw_text(cheese, tx + (tab_w[i] - text_w) * 0.5f, ty, label_str,
                       font, text_color, 1.0f);
    }

    if (is_sel)
      cheese_draw_rect(cheese, (cheese_corners_t){0, 0, 0, 0}, tx, y + h - 2.0f,
                       tab_w[i], 2.0f, accent);

    cheese_widget_emit(cheese, tab_sem, CHEESE_ROLE_TAB, label, null,
                       is_sel ? CHEESE_STATE_CHECKED : 0, &scope);

    if (cheese_widget_activated(cheese, &scope)) {
      sel = (i32)i;
      cheese_value_set_i32(selected, sel);
    }

    if (scope.focused)
      cheese_draw_focus_ring(cheese, &style, tx, y, tab_w[i], h);

    tx += tab_w[i];
  }

  return sel;
}

i32 cheese_tab_bar_auto(cheese_t *cheese, const cstr *classes,
                        cheese_semantics_t semantics, cheese_value_t selected,
                        const cstr *const *labels, u32 count,
                        cheese_font_t *font) {

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_TAB, classes, null);

  f32 line_height = 16.0f;
  if (font) {
    u32 target_size = style.font_size ? style.font_size : font->default_size;
    cheese_font_set_size(font, target_size);
    line_height = font->active_variant->line_height;
  }

  f32 pad_y = cheese_style_get_pad_top(&style) >= 0.0f
                  ? cheese_style_get_pad_top(&style)
                  : 6.0f;

  cheese_layout_t *layout = cheese_current_layout(cheese);
  f32 w = layout->width > 0.0f ? layout->width : 200.0f;
  f32 h = line_height + pad_y * 2.0f;

  f32 x, y;
  cheese_layout_place(cheese, &w, &h, &x, &y);

  return cheese_tab_bar(cheese, classes, semantics, x, y, w, h, selected,
                        labels, count, font);
}
