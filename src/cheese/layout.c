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
  };
}
cheese_layout_t *cheese_current_layout(cheese_t *cheese) {
  if (cheese->layout_stack_depth == 0)
    return &cheese->layout_stack[0];

  return &cheese->layout_stack[cheese->layout_stack_depth - 1];
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
