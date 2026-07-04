#include <cheese/layout.h>
#include <cheese/log.h>
#include <cheese/types.h>

cheese_layout_t cheese_layout_default(void) {
  return (cheese_layout_t){
      .x = 0.0f,
      .y = 0.0f,
      .width = 0.0f,
      .horizontal = true,
      .spacing = 4.0f,
      .align_x = CHEESE_ALIGN_START,
      .align_y = CHEESE_ALIGN_START,
      .container_w = 0.0f,
      .container_h = 0.0f,
  };
}
cheese_layout_t *cheese_current_layout(cheese_t *cheese) {
  if (cheese->layout_stack_depth == 0)
    return &cheese->layout_stack[0];

  return &cheese->layout_stack[cheese->layout_stack_depth - 1];
}

void cheese_layout_get_aligned_pos(const cheese_layout_t *layout, f32 *widget_w,
                                   f32 *widget_h, f32 *out_x, f32 *out_y) {
  if (!layout || !out_x || !out_y)
    return;

  switch (layout->align_x) {
  case CHEESE_ALIGN_START:
    *out_x = layout->x;
    break;
  case CHEESE_ALIGN_CENTER:
    *out_x = layout->x + (layout->container_w - *widget_w) / 2.0f;
    break;
  case CHEESE_ALIGN_END:
    *out_x = layout->x + layout->container_h - *widget_w;
    break;
  case CHEESE_ALIGN_FILL:
    *out_x = layout->x;
    *widget_w = layout->container_w;
    break;
  default:
    *out_x = layout->x;
    break;
  }

  switch (layout->align_y) {
  case CHEESE_ALIGN_START:
    *out_y = layout->y;
    break;
  case CHEESE_ALIGN_CENTER:
    *out_y = layout->y + (layout->container_h - *widget_h) / 2.0f;
    break;
  case CHEESE_ALIGN_END:
    *out_y = layout->y + layout->container_h - *widget_h;
    break;
  case CHEESE_ALIGN_FILL:
    *out_y = layout->y;
    *widget_h = layout->container_h;
    break;
  default:
    *out_y = layout->y;
    break;
  }
}

void cheese_push_layout(cheese_t *cheese, cheese_layout_t layout) {
  if (cheese->layout_stack_depth < CHEESE_STACK_MAX_DEPTH - 1) {
    cheese->layout_stack[cheese->layout_stack_depth++] = layout;
    return;
  }

  cheese_log_error("Layout stack is full, increase max stack depth by defining "
                   "CHEESE_STACK_MAX_DEPTH");
  return;
}

void cheese_pop_layout(cheese_t *cheese) {
  if (cheese->layout_stack_depth > 0)
    cheese->layout_stack_depth--;
}

void cheese_advance_cursor(cheese_t *cheese, f32 w, f32 h) {
  cheese_layout_t *layout = cheese_current_layout(cheese);
  if (!layout) {
    cheese_log_error("Invalid layout");
    return;
  }

  if (layout->horizontal)
    layout->x += w + layout->spacing;
  else
    layout->y += h + layout->spacing;
}
