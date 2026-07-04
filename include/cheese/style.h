#ifndef CHEESE_STYLE_H
#define CHEESE_STYLE_H

#include <cheese/types.h>

cheese_style_t cheese_default_style(void);
cheese_style_t *cheese_current_style(cheese_t *cheese);

void cheese_push_style(cheese_t *cheese, cheese_style_t style);
void cheese_pop_style(cheese_t *cheese);

void cheese_style_set_font_size(cheese_style_t *style, u32 font_size);
void cheese_style_set_layout_horizontal(cheese_style_t *style, b32 horizontal);
void cheese_style_set_layout_spacing(cheese_style_t *style, f32 spacing);
void cheese_style_set_align_x(cheese_style_t *style,
                              cheese_alignment_t align_x);
void cheese_style_set_align_y(cheese_style_t *style,
                              cheese_alignment_t align_y);

void cheese_style_set_padding_uniform(cheese_style_t *style, f32 padding);
void cheese_style_set_padding(cheese_style_t *style, f32 padding_left,
                              f32 padding_down, f32 padding_up,
                              f32 padding_right);

void cheese_style_set_margin_uniform(cheese_style_t *style, f32 margin);
void cheese_style_set_margin(cheese_style_t *style, f32 margin_left,
                             f32 margin_down, f32 margin_up, f32 margin_right);

static inline f32 cheese_style_get_pad_left(const cheese_style_t *style) {
  return style->uniform_padding ? style->padding.uniform
                                : style->padding.directions.padding_left;
}
static inline f32 cheese_style_get_pad_down(const cheese_style_t *style) {
  return style->uniform_padding ? style->padding.uniform
                                : style->padding.directions.padding_down;
}
static inline f32 cheese_style_get_pad_up(const cheese_style_t *style) {
  return style->uniform_padding ? style->padding.uniform
                                : style->padding.directions.padding_up;
}
static inline f32 cheese_style_get_pad_right(const cheese_style_t *style) {
  return style->uniform_padding ? style->padding.uniform
                                : style->padding.directions.padding_right;
}

static inline f32 cheese_style_get_margin_left(const cheese_style_t *style) {
  return style->uniform_margin ? style->margin.uniform
                               : style->margin.directions.margin_left;
}
static inline f32 cheese_style_get_margin_down(const cheese_style_t *style) {
  return style->uniform_margin ? style->margin.uniform
                               : style->margin.directions.margin_down;
}
static inline f32 cheese_style_get_margin_up(const cheese_style_t *style) {
  return style->uniform_margin ? style->margin.uniform
                               : style->margin.directions.margin_up;
}
static inline f32 cheese_style_get_margin_right(const cheese_style_t *style) {
  return style->uniform_margin ? style->margin.uniform
                               : style->margin.directions.margin_right;
}

static inline f32 cheese_style_get_margin_height(const cheese_style_t *style) {
  return cheese_style_get_margin_up(style) +
         cheese_style_get_margin_down(style);
}

static inline f32 cheese_style_get_margin_width(const cheese_style_t *style) {
  return cheese_style_get_margin_left(style) +
         cheese_style_get_margin_right(style);
}

#endif // !CHEESE_STYLE_H
