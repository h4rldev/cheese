#include <cheese/log.h>
#include <cheese/style.h>
#include <cheese/types.h>

cheese_style_t cheese_default_style(void) {
  return (cheese_style_t){
      .bg_color = cheese_color_rgba(0, 0, 0, 255),
      .border_color = cheese_color_rgba(0, 0, 0, 255),
      .hover_color = cheese_color_rgba(0, 0, 0, 255),
      .text_color = cheese_color_rgba(255, 255, 255, 255),
      .border_width = 0.0f,
      .corner_radius = 0.0f,
      .uniform_padding = true,
      .padding.uniform = 0.0f,
  };
}

cheese_style_t *cheese_current_style(cheese_t *cheese) {
  return &cheese->style_stack[cheese->style_stack_depth - 1];
}

void cheese_push_style(cheese_t *cheese, cheese_style_t style) {
  if (cheese->style_stack_depth < CHEESE_STACK_MAX_DEPTH - 1) {
    cheese->style_stack[cheese->style_stack_depth++] = style;
    return;
  }

  cheese_log_error("Style stack is full, increase max stack depth by defining "
                   "CHEESE_STACK_MAX_DEPTH");
  return;
}

void cheese_pop_style(cheese_t *cheese) {
  if (cheese->style_stack_depth > 0)
    cheese->style_stack_depth--;
}

void cheese_style_set_padding_uniform(cheese_style_t *style, f32 padding) {
  if (!style) {
    cheese_log_error("Invalid style");
    return;
  }

  style->uniform_padding = true;
  style->padding.uniform = padding;
}
void cheese_style_set_padding(cheese_style_t *style, f32 padding_left,
                              f32 padding_down, f32 padding_up,
                              f32 padding_right) {
  if (!style) {
    cheese_log_error("Invalid style");
    return;
  }

  if (padding_left < 0.0f || padding_down < 0.0f || padding_up < 0.0f ||
      padding_right < 0.0f) {
    cheese_log_error("Invalid padding values");
    return;
  }

  style->uniform_padding = false;
  style->padding.directions.padding_left = padding_left;
  style->padding.directions.padding_down = padding_down;
  style->padding.directions.padding_up = padding_up;
  style->padding.directions.padding_right = padding_right;
}

void cheese_style_set_margin_uniform(cheese_style_t *style, f32 margin) {
  if (!style) {
    cheese_log_error("Invalid style");
    return;
  }

  style->uniform_margin = true;
  style->margin.uniform = margin;
}

void cheese_style_set_margin(cheese_style_t *style, f32 margin_left,
                             f32 margin_down, f32 margin_up, f32 margin_right) {
  if (!style) {
    cheese_log_error("Invalid style");
    return;
  }

  if (margin_left < 0.0f || margin_down < 0.0f || margin_up < 0.0f ||
      margin_right < 0.0f) {
    cheese_log_error("Invalid padding values");
    return;
  }

  style->uniform_margin = false;
  style->margin.directions.margin_left = margin_left;
  style->margin.directions.margin_down = margin_down;
  style->margin.directions.margin_up = margin_up;
  style->margin.directions.margin_right = margin_right;
}
