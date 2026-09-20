/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/layout.h>

/***********************************/

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

f32 cheese_size_resolve(cheese_size_t size, f32 extent, f32 intrinsic) {
  f32 out;

  switch (size.mode) {
  case CHEESE_SIZE_FILL:
    out = extent;
    break;
  case CHEESE_SIZE_FIXED:
    out = size.value;
    break;
  case CHEESE_SIZE_PERCENT:
    out = size.value * extent;
    break;
  case CHEESE_SIZE_FIT:
  default:
    out = intrinsic;
    break;
  }

  if (size.min > 0.0f && out < size.min)
    out = size.min;
  if (size.max > 0.0f && out > size.max)
    out = size.max;

  return out;
}

void cheese_layout_place(cheese_t *cheese, f32 *w, f32 *h, f32 *out_x,
                         f32 *out_y) {
  cheese_layout_t *layout = cheese_current_layout(cheese);
  if (!layout || !w || !h || !out_x || !out_y)
    return;

  if (layout->anchor != CHEESE_ANCHOR_FLOW) {
    f32 bx = layout->origin_x;
    f32 by = layout->origin_y;
    f32 bw = layout->width;
    f32 bh = layout->height;

    switch (layout->anchor) {
    case CHEESE_ANCHOR_TOP_LEFT:
      *out_x = bx + layout->anchor_dx;
      *out_y = by + layout->anchor_dy;
      break;
    case CHEESE_ANCHOR_TOP_CENTER:
      *out_x = bx + (bw - *w) * 0.5f + layout->anchor_dx;
      *out_y = by + layout->anchor_dy;
      break;
    case CHEESE_ANCHOR_TOP_RIGHT:
      *out_x = bx + bw - *w - layout->anchor_dx;
      *out_y = by + layout->anchor_dy;
      break;
    case CHEESE_ANCHOR_CENTER_LEFT:
      *out_x = bx + layout->anchor_dx;
      *out_y = by + (bh - *h) * 0.5f + layout->anchor_dy;
      break;
    case CHEESE_ANCHOR_CENTER:
      *out_x = bx + (bw - *w) * 0.5f + layout->anchor_dx;
      *out_y = by + (bh - *h) * 0.5f + layout->anchor_dy;
      break;
    case CHEESE_ANCHOR_CENTER_RIGHT:
      *out_x = bx + bw - *w - layout->anchor_dx;
      *out_y = by + (bh - *h) * 0.5f + layout->anchor_dy;
      break;
    case CHEESE_ANCHOR_BOTTOM_LEFT:
      *out_x = bx + layout->anchor_dx;
      *out_y = by + bh - *h - layout->anchor_dy;
      break;
    case CHEESE_ANCHOR_BOTTOM_CENTER:
      *out_x = bx + (bw - *w) * 0.5f + layout->anchor_dx;
      *out_y = by + bh - *h - layout->anchor_dy;
      break;
    case CHEESE_ANCHOR_BOTTOM_RIGHT:
      *out_x = bx + bw - *w - layout->anchor_dx;
      *out_y = by + bh - *h - layout->anchor_dy;
      break;
    default:
      break;
    }
    return;
  }

  f32 main = layout->horizontal ? *w : *h;
  f32 cross = layout->horizontal ? *h : *w;

  if (layout->columns > 0 && layout->width > 0.0f) {
    f32 gaps = (f32)(layout->columns - 1) * layout->spacing;
    main = (layout->width - gaps) / (f32)layout->columns;

    if (layout->horizontal)
      *w = main;
    else
      *h = main;
  }

  switch (layout->cross_mode) {
  case CHEESE_SIZE_FILL:
    cross = layout->horizontal ? layout->height : layout->width;
    break;
  case CHEESE_SIZE_FIXED:
    cross = layout->cross_size;
    break;
  case CHEESE_SIZE_FIT:
  default:
    break;
  }

  f32 main_pos = layout->horizontal ? layout->x : layout->y;
  f32 main_origin = layout->horizontal ? layout->origin_x : layout->origin_y;
  f32 main_limit = layout->horizontal ? layout->width : layout->height;

  if (layout->wrap && main_limit > 0.0f && main_pos > main_origin &&
      (main_pos + main) > (main_origin + main_limit)) {
    if (layout->horizontal)
      layout->y += layout->line_cross + layout->spacing;
    else
      layout->x += layout->line_cross + layout->spacing;

    main_pos = main_origin;
    layout->line_cross = 0.0f;
  }

  if (layout->horizontal)
    *h = cross;
  else
    *w = cross;

  *out_x = layout->horizontal ? main_pos : layout->x;
  *out_y = layout->horizontal ? layout->y : main_pos;

  if (layout->horizontal)
    layout->x = main_pos + main + layout->spacing;
  else
    layout->y = main_pos + main + layout->spacing;

  if (cross > layout->line_cross)
    layout->line_cross = cross;
}

void cheese_layout_set_columns(cheese_layout_t *layout, u32 columns) {
  if (!layout)
    return;

  layout->columns = columns;
  if (columns > 1)
    layout->wrap = true;
}

void cheese_layout_set_wrap(cheese_layout_t *layout, b32 wrap) {
  if (!layout)
    return;

  layout->wrap = wrap;
}

void cheese_layout_set_cross(cheese_layout_t *layout, cheese_size_mode_t mode,
                             f32 size) {
  if (!layout)
    return;

  layout->cross_mode = mode;
  layout->cross_size = size;
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
    *out_x = layout->x + layout->container_w - *widget_w;
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

void cheese_push_anchor(cheese_t *cheese, cheese_anchor_t anchor, f32 dx,
                        f32 dy) {
  cheese_layout_t child = *cheese_current_layout(cheese);
  child.anchor = anchor;
  child.anchor_dx = dx;
  child.anchor_dy = dy;
  cheese_push_layout(cheese, child);
}

void cheese_pop_anchor(cheese_t *cheese) { cheese_pop_layout(cheese); }

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

void cheese_layout_set_horizontal(cheese_layout_t *layout, b32 horizontal) {
  if (!layout) {
    cheese_log_error("cheese_layout_set_horizontal: Invalid layout");
    return;
  }
  layout->horizontal = horizontal;
}

void cheese_layout_set_spacing(cheese_layout_t *layout, f32 spacing) {
  if (!layout) {
    cheese_log_error("cheese_layout_set_spacing: Invalid layout");
    return;
  }

  if (spacing < 0.0f) {
    cheese_log_error(
        "cheese_layout_set_spacing: Invalid spacing value, must be >= 0.0f");
    return;
  }

  layout->spacing = spacing;
}

void cheese_layout_set_align_x(cheese_layout_t *layout,
                               cheese_alignment_t align_x) {
  if (!layout) {
    cheese_log_error("cheese_layout_set_align_x: Invalid layout");
    return;
  }

  layout->align_x = align_x;
}

void cheese_layout_set_align_y(cheese_layout_t *layout,
                               cheese_alignment_t align_y) {
  if (!layout) {
    cheese_log_error("cheese_layout_set_align_y: Invalid layout");
    return;
  }

  layout->align_y = align_y;
}
