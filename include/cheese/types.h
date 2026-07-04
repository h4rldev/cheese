#ifndef CHEESE_TYPES_H
#define CHEESE_TYPES_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <harfbuzz/hb.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

typedef u32 cheese_color_t;

#define cheese_color_rgba(r, g, b, a)                                          \
  (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define cheese_color_rgb(r, g, b) cheese_color_rgba(r, g, b, 255)
#define cheese_color_hex(hex) (hex)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

/** Aliases */
typedef FT_Face ft_face_t;
typedef FT_Library ft_library_t;
typedef FT_Error ft_error_t;
typedef FT_GlyphSlot ft_glyph_slot_t;
typedef FT_Bitmap ft_bitmap_t;

typedef enum {
  CHEESE_LOG_DEBUG,
  CHEESE_LOG_INFO,
  CHEESE_LOG_WARNING,
  CHEESE_LOG_ERROR,
  CHEESE_LOG_FATAL,
} cheese_log_level_t;

typedef struct {
  f32 u0, v0;
  f32 u1, v1;
  f32 advance;
  f32 bearing_x;
  f32 bearing_y;
  f32 width;
  f32 height;
} cheese_glyph_t;

typedef struct {
  cheese_glyph_t glyph;
  u32 glyph_id;
  b32 is_notdef;

  u8 *temp_bitmap_data;
  i32 atlas_x, atlas_y;
} cheese_glyph_entry_t;

typedef struct cheese_font cheese_font_t;
typedef struct cheese_renderer cheese_renderer_t;

struct cheese_renderer {
  void *userdata;
  void (*draw_rect)(void *userdata, f32 x, f32 y, f32 w, f32 h,
                    cheese_color_t color);
  void (*draw_texture)(void *userdata, f32 x, f32 y, f32 w, f32 h,
                       u32 texture_id, cheese_color_t color);
  void (*draw_line)(void *userdata, f32 x1, f32 y1, f32 x2, f32 y2,
                    f32 thickness, cheese_color_t color);
  void (*draw_arc)(void *userdata, f32 cx, f32 cy, f32 radius, f32 start_angle,
                   f32 end_angle, f32 thickness, cheese_color_t color);
  void (*draw_text)(void *userdata, f32 x, f32 y, const string *text,
                    cheese_font_t *font, cheese_color_t color, f32 scale);

  void (*push_clip)(void *userdata, f32 x, f32 y, f32 w, f32 h);
  void (*pop_clip)(void *userdata);

  i32 (*create_texture)(void *userdata, u32 width, u32 height, const u8 *data);
  void (*delete_texture)(void *userdata, u32 texture_id);
};

typedef enum {
  CHEESE_ALIGN_START,
  CHEESE_ALIGN_CENTER,
  CHEESE_ALIGN_END,
  CHEESE_ALIGN_FILL,
  CHEESE_ALIGN_MAX,
} cheese_alignment_t;

typedef enum {
  CHEESE_SIZE_FIT,
  CHEESE_SIZE_FILL,
  CHEESE_SIZE_FIXED,
  CHEESE_SIZE_MAX,
} cheese_size_mode_t;

typedef struct {
  u32 font_size;
  cheese_glyph_entry_t **glyphs; // Dynamic array.
  hb_font_t *hb_font;            // Font at this scale
  b32 atlas_dirty;
  u32 atlas_texture_id;
  u32 atlas_width, atlas_height;
  i32 ascender, descender, line_height;
} cheese_font_size_variant_t;

struct cheese_font {
  ft_face_t ft_face;
  arena_t *arena;
  cheese_renderer_t *renderer;

  u32 notdef_glyph_id;
  u32 default_size;

  cheese_font_size_variant_t **variants;
  cheese_font_size_variant_t *active_variant;
};

typedef enum {
  CHEESE_CURSOR_DEFAULT = 0,
  CHEESE_CURSOR_POINTER,
  CHEESE_CURSOR_HAND,
  CHEESE_CURSOR_TEXT,
  CHEESE_CURSOR_MOVE,
  CHEESE_CURSOR_RESIZE_EW,
  CHEESE_CURSOR_RESIZE_NS,
  CHEESE_CURSOR_RESIZE_NESW,
  CHEESE_CURSOR_RESIZE_NWSE,
  CHEESE_CURSOR_NOT_ALLOWED,
  CHEESE_CURSOR_WAIT,
  CHEESE_CURSOR_MAX,
} cheese_cursor_t;

typedef enum {
  CHEESE_MOUSE_LEFT = (1 << 0),
  CHEESE_MOUSE_RIGHT = (1 << 1),
  CHEESE_MOUSE_MIDDLE = (1 << 2),
  CHEESE_MOUSE_BACK = (1 << 3),
  CHEESE_MOUSE_FORWARD = (1 << 4),
} cheese_mouse_button_t;

typedef enum {
  CHEESE_BUTTON_CLICK_LEFT = (1 << 0),
  CHEESE_BUTTON_CLICK_RIGHT = (1 << 1),
  CHEESE_BUTTON_CLICK_MIDDLE = (1 << 2),
  CHEESE_BUTTON_CLICK_BACK = (1 << 3),
  CHEESE_BUTTON_CLICK_FORWARD = (1 << 4),
} cheese_button_click_t;

typedef enum {
  CHEESE_MOD_SHIFT = (1 << 0),
  CHEESE_MOD_CTRL = (1 << 1),
  CHEESE_MOD_ALT = (1 << 2),
  CHEESE_MOD_SUPER = (1 << 3),
} cheese_mod_key_t;

typedef struct {
  f32 x, y;
  f32 width;
  b32 horizontal;
  f32 spacing;
  cheese_alignment_t align_x;
  cheese_alignment_t align_y;
  f32 container_w;
  f32 container_h;
} cheese_layout_t;

typedef struct {
  cheese_color_t bg_color;
  cheese_color_t border_color;
  cheese_color_t hover_color;
  cheese_color_t text_color;

  f32 border_width;
  f32 corner_radius;

  b32 uniform_padding;
  union {
    f32 uniform;
    struct {
      f32 padding_left;
      f32 padding_down;
      f32 padding_up;
      f32 padding_right;
    } directions;
  } padding;

  b32 uniform_margin;
  union {
    f32 uniform;
    struct {
      f32 margin_left;
      f32 margin_down;
      f32 margin_up;
      f32 margin_right;
    } directions;
  } margin;

  u32 font_size;
  b32 layout_horizontal;
  f32 layout_spacing;
  cheese_alignment_t align_x;
  cheese_alignment_t align_y;
} cheese_style_t;

#ifndef CHEESE_STACK_MAX_DEPTH
#define CHEESE_STACK_MAX_DEPTH 64
#endif

typedef void (*cheese_cursor_callback_t)(void *userdata,
                                         cheese_cursor_t cursor);

typedef struct {
  cheese_renderer_t *renderer;
  f32 mouse_x, mouse_y;
  f32 scroll_x, scroll_y;

  u32 mouse_buttons;
  u32 mouse_prev_buttons;
  u32 key_mods;

  u64 frame_count;
  f32 delta_time;

  cheese_layout_t layout_stack[CHEESE_STACK_MAX_DEPTH];
  u32 layout_stack_depth;

  cheese_style_t style_stack[CHEESE_STACK_MAX_DEPTH];
  u32 style_stack_depth;

  cheese_cursor_callback_t cursor_callback;
  void *cursor_callback_userdata;

  b32 text_input_active;
  u32 text_input_cursor_pos;
  u32 text_input_selection_start;

  b32 is_dragging;
  f32 drag_start_x, drag_start_y;
  void *drag_payload;

  u32 focused_widget_id;
  u32 active_widget_id;
  u32 hovered_widget_id;
} cheese_t;

#endif // !CHEESE_TYPES_H
