#include <cheese/init.h>
#include <cheese/layout.h>
#include <cheese/style.h>
#include <cheese/types.h>

cheese_t cheese_default(void) {
  cheese_t cheese = {0};

  cheese.style_stack[0] = cheese_default_style();
  cheese.layout_stack[0] = cheese_layout_default();

  return cheese;
}

void cheese_begin(cheese_t *cheese, cheese_renderer_t *renderer, f32 mouse_x,
                  f32 mouse_y, u32 mouse_buttons, f32 scroll_x, f32 scroll_y,
                  u32 key_mods, f32 delta_time) {

  cheese->renderer = renderer;
  cheese->mouse_x = mouse_x;
  cheese->mouse_y = mouse_y;
  cheese->scroll_x = scroll_x;
  cheese->scroll_y = scroll_y;
  cheese->mouse_prev_buttons = cheese->mouse_buttons;
  cheese->mouse_buttons = mouse_buttons;
  cheese->key_mods = key_mods;

  cheese->frame_count++;
  cheese->delta_time = delta_time;

  cheese->style_stack[0] = cheese_default_style();
  cheese->layout_stack[0] = cheese_layout_default();
  cheese->layout_stack_depth = 1;
  cheese->style_stack_depth = 1;

  if (cheese->cursor_callback)
    cheese->cursor_callback(cheese->cursor_callback_userdata,
                            CHEESE_CURSOR_DEFAULT);
}

void cheese_end(cheese_t *cheese) {
  cheese->scroll_x = 0;
  cheese->scroll_y = 0;
}
