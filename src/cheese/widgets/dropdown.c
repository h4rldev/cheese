/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/overlay.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/widgets/button.h>
#include <cheese/widgets/dropdown.h>
#include <cheese/widgets/widget.h>

/***********************************/

typedef struct {
  const cstr *const *items;
  u32 count;
  i32 selected;
  f32 x, y, w, row_h, pad;
  cheese_style_t style;
  cheese_font_t *font;
  cheese_popup_t *state;
  cheese_value_t selected_value;
} cheese_dropdown_menu_t;

//
//
//

static void cheese_dropdown_draw(cheese_t *cheese, void *userdata) {
  cheese_dropdown_menu_t *menu = userdata;

  f32 panel_h = menu->pad * 2.0f + (f32)menu->count * menu->row_h;

  cheese_color_t bg = menu->style.bg_color ? menu->style.bg_color : 0xFFFFFFFF;
  cheese_color_t edge = cheese_style_get_prop_color(
      &menu->style, cheese->core_props.border_color, 0x000000FF);
  cheese_color_t text =
      menu->style.text_color ? menu->style.text_color : 0x000000FF;
  cheese_color_t hover =
      menu->style.hover_color ? menu->style.hover_color : 0x3B82F640;

  cheese_draw_rect(cheese, menu->style.corner_radius, menu->x, menu->y, menu->w,
                   panel_h, bg);

  cheese_draw_line(cheese, menu->x, menu->y, menu->x + menu->w, menu->y, 1.0f,
                   edge);
  cheese_draw_line(cheese, menu->x, menu->y + panel_h, menu->x + menu->w,
                   menu->y + panel_h, 1.0f, edge);
  cheese_draw_line(cheese, menu->x, menu->y, menu->x, menu->y + panel_h, 1.0f,
                   edge);
  cheese_draw_line(cheese, menu->x + menu->w, menu->y, menu->x + menu->w,
                   menu->y + panel_h, 1.0f, edge);

  f32 mx = cheese->mouse_x;
  f32 my = cheese->mouse_y;

  b32 clicked =
      (cheese->mouse_buttons & ~cheese->mouse_prev_buttons) & CHEESE_MOUSE_LEFT;
  b32 inside = mx >= menu->x && mx <= menu->x + menu->w && my >= menu->y &&
               my <= menu->y + panel_h;

  if (inside)
    cheese_cursor_request(cheese, CHEESE_CURSOR_POINTER);

  for (u32 i = 0; i < menu->count; i++) {
    f32 ry = menu->y + menu->pad + (f32)i * menu->row_h;
    b32 row_hovered = mx >= menu->x && mx <= menu->x + menu->w && my >= ry &&
                      my <= ry + menu->row_h;

    if (row_hovered)
      cheese_draw_rect(cheese, menu->style.corner_radius, menu->x + 1.0f, ry,
                       menu->w - 2.0f, menu->row_h, hover);

    cheese_semantics_emit(
        cheese, (cheese_semantics_t){.name = menu->items[i]}, CHEESE_ROLE_LABEL,
        menu->items[i], null, 0,
        (cheese_rect_t){(i32)menu->x, (i32)ry, (u32)menu->w, (u32)menu->row_h});

    if (menu->font && menu->items[i][0]) {
      string *label = string_from_cstr(cheese->frame_arena, menu->items[i]);
      f32 ty = ry + menu->row_h * 0.5f +
               menu->font->active_variant->line_height * 0.25f;
      cheese_draw_text(cheese, menu->x + menu->pad, ty, label, menu->font, text,
                       1.0f);
    }

    if (row_hovered && clicked) {
      menu->selected = (i32)i;
      cheese_popup_close(menu->state);
      cheese_value_set_i32(menu->selected_value, menu->selected);
    }
  }
  cheese_popup_dismissed(
      cheese, menu->state,
      (cheese_rect_t){(i32)menu->x, (i32)menu->y, (u32)menu->w, (u32)panel_h});
}

//
//
//

i32 cheese_dropdown(cheese_t *cheese, const cstr *classes,
                    cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                    cheese_value_t selected, const cstr *const *items,
                    u32 count, cheese_popup_t *state, cheese_font_t *font) {
  if (!cheese || !cheese->renderer || !state)
    return 0;

  i32 sel = cheese_value_i32(selected);
  if (count == 0)
    sel = 0;
  else if (sel < 0)
    sel = 0;
  else if ((u32)sel >= count)
    sel = (i32)count - 1;

  const cstr *label = (items && count > 0 && items[sel]) ? items[sel] : "";

  cheese_push_scope(cheese, cheese_style_new(), "dropdown");
  u32 clicks = cheese_button_ex(cheese, classes, semantics, x, y, w, h,
                                cheese_val_str(label), font, 0, 0, 0);
  cheese_pop_scope(cheese);

  if (clicks & CHEESE_BUTTON_CLICK_LEFT) {
    if (state->open)
      cheese_popup_close(state);
    else
      cheese_popup_open(cheese, state);
  }

  if (state->open) {
    cheese_style_t style;
    cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_DROPDOWN, classes,
                                null);

    f32 line_height = 16.0f;
    if (font) {
      u32 target_size = style.font_size ? style.font_size : font->default_size;
      cheese_font_set_size(font, target_size);
      line_height = font->active_variant->line_height;
    }

    f32 pad = 4.0f;
    f32 row_h = line_height + 8.0f;

    cheese_dropdown_menu_t *menu =
        arena_alloc_zeroed(cheese->frame_arena, cheese_dropdown_menu_t, 1);
    menu->items = items;
    menu->count = count;
    menu->selected = sel;
    menu->x = x;
    menu->y = y + h;
    menu->w = w;
    menu->row_h = row_h;
    menu->pad = pad;
    menu->style = style;
    menu->font = font;
    menu->state = state;
    menu->selected_value = selected;

    cheese_overlay(cheese, menu->x, menu->y, menu->w,
                   pad * 2.0f + (f32)count * row_h, cheese_dropdown_draw, menu);
  }

  return sel;
}

i32 cheese_dropdown_auto(cheese_t *cheese, const cstr *classes,
                         cheese_semantics_t semantics, cheese_value_t selected,
                         const cstr *const *items, u32 count,
                         cheese_popup_t *state, cheese_font_t *font) {

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_DROPDOWN, classes,
                              null);

  f32 line_height = 16.0f;
  if (font) {
    u32 target_size = style.font_size ? style.font_size : font->default_size;
    cheese_font_set_size(font, target_size);
    line_height = font->active_variant->line_height;
  }

  f32 pad_y = cheese_style_get_pad_top(&style) >= 0.0f
                  ? cheese_style_get_pad_top(&style)
                  : 4.0f;

  cheese_layout_t *layout = cheese_current_layout(cheese);
  f32 w = layout->width > 0.0f ? layout->width : 200.0f;
  f32 h = line_height + pad_y * 2.0f;

  f32 x, y;
  cheese_layout_place(cheese, &w, &h, &x, &y);

  return cheese_dropdown(cheese, classes, semantics, x, y, w, h, selected,
                         items, count, state, font);
}
