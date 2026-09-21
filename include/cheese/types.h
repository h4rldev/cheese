#ifndef CHEESE_TYPES_H
#define CHEESE_TYPES_H

/***********************************/

#include <threads.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <harfbuzz/hb.h>

#include <htils/arena.h>
#include <htils/atomic_types.h>
#include <htils/basictypes.h>
#include <htils/string.h>
#include <htils/stringmap.h>

/***********************************/

/** @brief An RGB(A) colour, packed as `0xAARRGGBB`. */
typedef u32 cheese_color_t;

//
//
//

/** @brief Pack an RGBA colour (each channel `0..255`). */
#define cheese_color_rgba(r, g, b, a)                                          \
  ((cheese_color_t)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define cheese_color_rgb(r, g, b) cheese_color_rgba(r, g, b, 255)
#define cheese_color_hex(hex) ((cheese_color_t)(hex))

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

//
//
//

/** @brief A linear (four-corner) colour field for a rectangle fill. */
typedef struct {
  cheese_color_t top_left, top_right, bottom_right, bottom_left;
} cheese_gradient_t;

//
//
//

/** @brief Log severity. */
typedef enum {
  CHEESE_LOG_DEBUG,
  CHEESE_LOG_INFO,
  CHEESE_LOG_WARNING,
  CHEESE_LOG_ERROR,
  CHEESE_LOG_FATAL,
} cheese_log_level_t;

//
//
//

/** @brief FreeType aliases. */
typedef FT_Face ft_face_t;
typedef FT_Library ft_library_t;
typedef FT_Error ft_error_t;
typedef FT_GlyphSlot ft_glyph_slot_t;
typedef FT_Bitmap ft_bitmap_t;
typedef FT_UInt ft_uint_t;

//
//
//

typedef struct cheese cheese_t;
typedef struct cheese_renderer cheese_renderer_t;
typedef struct cheese_font cheese_font_t;
typedef struct cheese_state cheese_state_t;
typedef struct cheese_state_store cheese_state_store_t;

//
//
//

/** @brief Four per-corner radii. */
typedef struct {
  f32 top_left, top_right, bottom_left, bottom_right;
} cheese_corners_t;

//
//
//

/** @brief An integer screen rectangle. */
typedef struct {
  i32 x, y;
  u32 w, h;
} cheese_rect_t;

//
//
//

/** @brief Four edge values (left, right, top, bottom). */
typedef struct {
  f32 left, right, top, bottom;
} cheese_edges_t;

//
//
//

/** @brief Cross-axis alignment. */
typedef enum {
  CHEESE_ALIGN_START,
  CHEESE_ALIGN_CENTER,
  CHEESE_ALIGN_END,
  CHEESE_ALIGN_FILL,
  CHEESE_ALIGN_MAX,
} cheese_alignment_t;

//
//
//

/** @brief How a size spec resolves. */
typedef enum {
  CHEESE_SIZE_FIT,     // intrinsic content size
  CHEESE_SIZE_FILL,    // the container's extent
  CHEESE_SIZE_FIXED,   // exact pixels (value)
  CHEESE_SIZE_PERCENT, // value (0..1) of the container's extent
  CHEESE_SIZE_MAX,
} cheese_size_mode_t;

//
//
//

/** @brief A size relative to the available extent, with optional clamps. */
typedef struct {
  cheese_size_mode_t mode;
  f32 value; // FIXED: pixels; PERCENT: fraction (0..1)
  f32 min;   // 0 = none
  f32 max;   // 0 = none
} cheese_size_t;

//
//
//

/** @brief How an image fits its box. */
typedef enum {
  CHEESE_FIT_STRETCH, // ignore aspect ratio, fill the box
  CHEESE_FIT_CONTAIN, // fit inside, preserving aspect (letterbox)
  CHEESE_FIT_COVER,   // fill, preserving aspect (crop)
} cheese_fit_t;

//
//
//

/**
 * @brief Where a widget sits within its layout's content box.
 * @details @ref CHEESE_ANCHOR_FLOW is the default (normal cursor placement).
 * The rest place the next widget at that position of the content box and do
 * not advance the cursor, so it leaves the flow untouched. Push one with
 * @ref cheese_push_anchor.
 */
typedef enum {
  CHEESE_ANCHOR_FLOW,
  CHEESE_ANCHOR_TOP_LEFT,
  CHEESE_ANCHOR_TOP_CENTER,
  CHEESE_ANCHOR_TOP_RIGHT,
  CHEESE_ANCHOR_CENTER_LEFT,
  CHEESE_ANCHOR_CENTER,
  CHEESE_ANCHOR_CENTER_RIGHT,
  CHEESE_ANCHOR_BOTTOM_LEFT,
  CHEESE_ANCHOR_BOTTOM_CENTER,
  CHEESE_ANCHOR_BOTTOM_RIGHT,
} cheese_anchor_t;

//
//
//

/** @brief The flow/anchor state of a container's children. */
typedef struct {
  f32 x, y;
  f32 width;
  f32 height;
  f32 origin_x, origin_y; // inner origin (wrapping resets to this)
  b32 horizontal;
  f32 spacing;
  cheese_alignment_t align_x;
  cheese_alignment_t align_y;
  f32 container_w;
  f32 container_h;
  b32 wrap;
  u32 columns; // 0 = off; otherwise a grid of this many columns
  cheese_size_mode_t cross_mode; // FIT | FILL | FIXED
  f32 cross_size;
  f32 line_cross;
  cheese_anchor_t anchor;   // FLOW = normal cursor placement
  f32 anchor_dx, anchor_dy; // inset from the anchored edges
} cheese_layout_t;

//
//
//

/** @brief Pixel format of an uploaded texture. */
typedef enum {
  CHEESE_TEXTURE_RGBA8,
  CHEESE_TEXTURE_R8,
} cheese_texture_format_t;

//
//
//

/** @brief A renderer texture handle plus its pixel size (0 id = none). */
typedef struct {
  u32 id;
  u32 width, height;
} cheese_texture_t;

//
//
//

/** @brief Sides of a rounded-rect border ring (a bitmask). */
typedef enum {
  CHEESE_SIDE_NONE = 0,
  CHEESE_SIDE_LEFT = (1 << 0),
  CHEESE_SIDE_TOP = (1 << 1),
  CHEESE_SIDE_RIGHT = (1 << 2),
  CHEESE_SIDE_BOTTOM = (1 << 3),
  CHEESE_SIDE_ALL = CHEESE_SIDE_LEFT | CHEESE_SIDE_TOP | CHEESE_SIDE_RIGHT |
                    CHEESE_SIDE_BOTTOM,
} cheese_side_t;

//
//
//

/**
 * @brief The renderer plugin boundary.
 * @details The only backend-specific seam in cheese: the consumer fills this
 * with the functions its renderer (butter, SDL, Sokol, ...) implements.
 */
struct cheese_renderer {
  void *userdata;
  b32 sdf_text;
  void (*draw_rect)(void *userdata, cheese_corners_t radius, f32 x, f32 y,
                    f32 w, f32 h, cheese_color_t color);
  void (*draw_rect_gradient)(void *userdata, cheese_corners_t radius, f32 x,
                             f32 y, f32 w, f32 h, cheese_gradient_t colors);
  void (*draw_border)(void *userdata, cheese_corners_t radius, f32 x, f32 y,
                      f32 w, f32 h, f32 thickness, u32 sides,
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

  i32 (*create_texture)(void *userdata, u32 width, u32 height,
                        cheese_texture_format_t format, const u8 *data);
  void (*delete_texture)(void *userdata, u32 texture_id);
  void (*update_texture_region)(void *userdata, u32 texture_id, i32 x, i32 y,
                                u32 w, u32 h, const void *data, u64 data_size);

  void (*flush_deferred)(void *userdata);
  void (*flush_draws)(void *userdata);
};

//
//
//

/** @brief The value type a state slot holds. */
typedef enum {
  CHEESE_STATE_BOOL,
  CHEESE_STATE_I32,
  CHEESE_STATE_U32,
  CHEESE_STATE_F32,
  CHEESE_STATE_F64,
  CHEESE_STATE_STR,
  CHEESE_STATE_COUNT,
} cheese_state_type_t;

//
//
//

/**
 * @brief Callback invoked when a subscribed state value changes.
 *
 * @param state The state value that changed.
 * @param userdata The userdata passed at subscription time.
 */
typedef void (*cheese_state_handler_t)(cheese_state_t *state, void *userdata);

//
//
//

/** @brief One subscription to a state value's changes. */
typedef struct {
  u64 id;
  cheese_state_handler_t handler;
  void *userdata;
  u64 last_version;
} cheese_state_subscription_t;

//
//
//

/** @brief A standalone, thread-safe store of typed values. */
struct cheese_state_store {
  arena_t *arena;
  mtx_t mtx;
  cheese_state_t **states; // Dynamic array.
  atomic_u32 batch_depth;
  atomic_b32 frame_needed;
  atomic_b32 batch_pending;
  atomic_u64 generation;
};

//
//
//

/** @brief One typed, named value in a store. */
struct cheese_state {
  cheese_state_store_t *store;
  const cstr *name;
  cheese_state_type_t type;
  atomic_u64 version;
  atomic_b32 bound;
  union {
    atomic_b32 as_b32;
    atomic_i32 as_i32;
    atomic_u32 as_u32;
    atomic_f32 as_f32;
    atomic_f64 as_f64;
    _Atomic(cstr *) as_str;
  } value;

  u64 next_sub_id;
  cheese_state_subscription_t *subs; // Dynamic array.
};

//
//
//

/**
 * @brief A widget data value: either a state binding or a literal.
 * @details When @c state is non-null the value is read from, and written to,
 * that state; otherwise @c literal is used directly.
 */
typedef struct {
  cheese_state_t *state;
  union {
    b32 b32;
    i32 i32;
    u32 u32;
    f32 f32;
    f64 f64;
    const cstr *str;
  } literal;
} cheese_value_t;

/** @brief A literal @ref cheese_value_t. */
#define cheese_val_b32(v) ((cheese_value_t){.literal.b32 = (v)})
#define cheese_val_i32(v) ((cheese_value_t){.literal.i32 = (v)})
#define cheese_val_u32(v) ((cheese_value_t){.literal.u32 = (v)})
#define cheese_val_f32(v) ((cheese_value_t){.literal.f32 = (v)})
#define cheese_val_f64(v) ((cheese_value_t){.literal.f64 = (v)})
#define cheese_val_str(v) ((cheese_value_t){.literal.str = (v)})

/** @brief A state-bound @ref cheese_value_t. */
#define cheese_val_state(s) ((cheese_value_t){.state = (s)})

//
//
//

/** @brief The role a semantics node plays. */
typedef enum {
  CHEESE_ROLE_NONE,
  CHEESE_ROLE_ROOT,
  CHEESE_ROLE_CONTAINER,
  CHEESE_ROLE_BUTTON,
  CHEESE_ROLE_CHECKBOX,
  CHEESE_ROLE_RADIO,
  CHEESE_ROLE_SLIDER,
  CHEESE_ROLE_SCROLLBAR,
  CHEESE_ROLE_PROGRESS_BAR,
  CHEESE_ROLE_TAB,
  CHEESE_ROLE_TEXT_INPUT,
  CHEESE_ROLE_LABEL,
  CHEESE_ROLE_IMAGE,
  CHEESE_ROLE_DROPDOWN,
  CHEESE_ROLE_LIST,
  CHEESE_ROLE_SCROLL,
  CHEESE_ROLE_MAX,
} cheese_role_t;

//
//
//

/** @brief Interaction-state bits reported on a semantics node. */
typedef enum {
  CHEESE_STATE_FOCUSED = 1 << 0,
  CHEESE_STATE_HOVERED = 1 << 1,
  CHEESE_STATE_PRESSED = 1 << 2,
  CHEESE_STATE_DISABLED = 1 << 3,
  CHEESE_STATE_CHECKED = 1 << 4,
  CHEESE_STATE_ACTIVE = 1 << 5,
} cheese_semantics_state_t;

//
//
//

/**
 * @brief A widget's accessibility/inspection descriptor.
 * @details Optional; zero-init for pure defaults. @c key gives a stable
 * identity (wins over the derived id), @c name is the a11y name (falls back to
 * the label binding), and @c role overrides the widget's default role.
 */
typedef struct {
  const cstr *key;
  const cstr *name;
  cheese_role_t role;
  const cstr *description;
} cheese_semantics_t;

//
//
//

/**
 * @brief One node of the per-frame semantics tree.
 * @details A flat array linked by indices (@c parent, @c first_child /
 * @c last_child, @c next_sibling). Rebuilt every frame.
 */
typedef struct {
  u64 id;
  cheese_role_t role;
  u32 state; // cheese_semantics_state_t bits

  i32 parent;
  i32 first_child;
  i32 last_child;
  i32 next_sibling;

  cheese_rect_t bounds;

  const cstr *key;
  const cstr *name;
  const cstr *value;
  const cstr *description;
} cheese_semantics_node_t;

//
//
//

/** @brief A read-only view of a captured semantics tree. */
typedef struct {
  const cheese_semantics_node_t *nodes;
  u64 count;
} cheese_semantics_snapshot_t;

//
//
//

/** @brief One rasterized glyph: atlas UVs and placement metrics. */
typedef struct {
  f32 u0, v0;
  f32 u1, v1;
  f32 advance;
  f32 bearing_x;
  f32 bearing_y;
  f32 width;
  f32 height;
} cheese_glyph_t;

//
//
//

/**
 * @brief One shaped glyph: its destination quad and atlas coordinates.
 * @details Produced by @ref cheese_font_shape_run so a renderer backend only
 * has to draw a textured quad, without linking HarfBuzz itself.
 */
typedef struct {
  f32 x, y;             // destination top-left
  f32 w, h;             // destination size
  f32 u0, v0, u1, v1;   // atlas texture coordinates
  u32 texture_id;       // the glyph atlas texture
  cheese_color_t color; // tint
} cheese_glyph_quad_t;

//
//
//

/**
 * @brief Receives one shaped glyph quad.
 *
 * @param userdata The userdata passed to @ref cheese_font_shape_run.
 * @param quad The glyph's destination quad and atlas coordinates.
 */
typedef void (*cheese_glyph_emit_fn)(void *userdata,
                                     const cheese_glyph_quad_t *quad);

//
//
//

/** @brief A cached glyph plus its temporary bitmap before atlas packing. */
typedef struct {
  cheese_glyph_t glyph;
  u32 glyph_id;
  b32 is_notdef;

  u8 *temp_bitmap_data;
  i32 atlas_x, atlas_y;
} cheese_glyph_entry_t;

//
//
//

/** @brief One size variant of a font: its atlas, glyphs and metrics. */
typedef struct {
  u32 font_size;
  cheese_glyph_entry_t **glyphs; // Dynamic array.
  hb_font_t *hb_font;            // Font at this scale
  b32 sdf;
  b32 atlas_dirty;
  u32 atlas_texture_id;
  u32 atlas_width, atlas_height;

  cheese_rect_t *free_rects; // Dynamic array.
  arena_t *arena;

  i32 ascender, descender, line_height;
} cheese_font_size_variant_t;

//
//
//

/** @brief A loaded font: its face, metrics and size variants. */
struct cheese_font {
  ft_face_t ft_face;
  arena_t *arena;
  arena_t *scratch;
  cheese_renderer_t *renderer;

  u32 notdef_glyph_id;
  u32 default_size;

  b32 sdf;          // renderer->sdf_text: one base SDF atlas, scaled
  u32 base_size;    // pixel size the SDF atlas is generated at
  u32 logical_size; // last requested size
  f32 scale;        // logical_size / base_size (1.0 in bitmap mode)
  i32 base_ascender, base_descender, base_line_height;

  cheese_font_size_variant_t **variants;
  cheese_font_size_variant_t *active_variant;
};

//
//
//

/** @brief Cursor shapes cheese can request from the window layer. */
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

//
//
//

/** @brief Named keyboard keys. Text arrives separately as codepoints. */
typedef enum {
  CHEESE_KEY_UNKNOWN = 0,

  CHEESE_KEY_TAB,
  CHEESE_KEY_ENTER,
  CHEESE_KEY_SPACE,
  CHEESE_KEY_ESCAPE,
  CHEESE_KEY_BACKSPACE,
  CHEESE_KEY_DELETE,

  CHEESE_KEY_LEFT,
  CHEESE_KEY_RIGHT,
  CHEESE_KEY_UP,
  CHEESE_KEY_DOWN,
  CHEESE_KEY_HOME,
  CHEESE_KEY_END,
  CHEESE_KEY_PAGE_UP,
  CHEESE_KEY_PAGE_DOWN,

  CHEESE_KEY_A,
  CHEESE_KEY_B,
  CHEESE_KEY_C,
  CHEESE_KEY_D,
  CHEESE_KEY_E,
  CHEESE_KEY_F,
  CHEESE_KEY_G,
  CHEESE_KEY_H,
  CHEESE_KEY_I,
  CHEESE_KEY_J,
  CHEESE_KEY_K,
  CHEESE_KEY_L,
  CHEESE_KEY_M,
  CHEESE_KEY_N,
  CHEESE_KEY_O,
  CHEESE_KEY_P,
  CHEESE_KEY_Q,
  CHEESE_KEY_R,
  CHEESE_KEY_S,
  CHEESE_KEY_T,
  CHEESE_KEY_U,
  CHEESE_KEY_V,
  CHEESE_KEY_W,
  CHEESE_KEY_X,
  CHEESE_KEY_Y,
  CHEESE_KEY_Z,

  CHEESE_KEY_0,
  CHEESE_KEY_1,
  CHEESE_KEY_2,
  CHEESE_KEY_3,
  CHEESE_KEY_4,
  CHEESE_KEY_5,
  CHEESE_KEY_6,
  CHEESE_KEY_7,
  CHEESE_KEY_8,
  CHEESE_KEY_9,

  CHEESE_KEY_MAX,
} cheese_key_t;

//
//
//

/** @brief One key event for a frame. */
typedef struct {
  cheese_key_t key; // named key, or UNKNOWN for pure text
  u32 codepoint;    // produced text (0 if none)
  u32 mods;         // cheese_mod_key_t bits at event time
  b32 repeat;
} cheese_key_event_t;

//
//
//

/** @brief Maximum key events carried per frame. */
#define CHEESE_MAX_KEY_EVENTS 16

//
//
//

/** @brief Mouse button bits. */
typedef enum {
  CHEESE_MOUSE_LEFT = (1 << 0),
  CHEESE_MOUSE_RIGHT = (1 << 1),
  CHEESE_MOUSE_MIDDLE = (1 << 2),
  CHEESE_MOUSE_BACK = (1 << 3),
  CHEESE_MOUSE_FORWARD = (1 << 4),
} cheese_mouse_button_t;

//
//
//

/** @brief Which mouse buttons a widget responds to on click. */
typedef enum {
  CHEESE_BUTTON_CLICK_LEFT = (1 << 0),
  CHEESE_BUTTON_CLICK_RIGHT = (1 << 1),
  CHEESE_BUTTON_CLICK_MIDDLE = (1 << 2),
  CHEESE_BUTTON_CLICK_BACK = (1 << 3),
  CHEESE_BUTTON_CLICK_FORWARD = (1 << 4),
} cheese_button_click_t;

//
//
//

/** @brief Keyboard modifier bits. */
typedef enum {
  CHEESE_MOD_SHIFT = (1 << 0),
  CHEESE_MOD_CTRL = (1 << 1),
  CHEESE_MOD_ALT = (1 << 2),
  CHEESE_MOD_SUPER = (1 << 3),
} cheese_mod_key_t;

//
//
//

/** @brief A spatial focus direction (see @ref cheese_focus_move_dir). */
typedef enum {
  CHEESE_FOCUS_LEFT = 0,
  CHEESE_FOCUS_RIGHT,
  CHEESE_FOCUS_UP,
  CHEESE_FOCUS_DOWN,
} cheese_focus_dir_t;

//
//
//

/**
 * @brief Windowing input for a frame.
 * @details Filled by whatever owns the window (bread, SDL, ...) and passed to
 * @ref cheese_begin. Read-only from cheese's side; the only output back to the
 * window is the cursor via @ref cheese_set_cursor_callback.
 */
typedef struct {
  f32 mouse_x, mouse_y;
  u32 mouse_buttons;
  f32 scroll_x, scroll_y;
  f32 window_w, window_h; // drawable size
  u32 key_mods;

  u32 key_event_count;
  cheese_key_event_t key_events[CHEESE_MAX_KEY_EVENTS];
} cheese_input_t;

//
//
//

/** @brief The representation a property value uses */
typedef enum {
  CHEESE_PROP_COLOR,
  CHEESE_PROP_F32,
  CHEESE_PROP_U32,
  CHEESE_PROP_I32,
  CHEESE_PROP_PTR,
  CHEESE_PROP_KIND_COUNT,
} cheese_prop_kind_t;

//
//
//

/**
 * @brief One widget-owned style property.
 * @details Ids are interned (@ref cheese_prop_register) and namespaced by
 * convention (`"gauge/needle/angle"`). An entry's presence *is* its value:
 * unlike the body style's inherit sentinels there is no `set` bit, so `0` and
 * transparent are real values. @c kind selects the active union member.
 */
typedef struct {
  u32 id;
  u8 kind;
  union {
    cheese_color_t color;
    f32 f32;
    i32 i32;
    u32 u32;
    const void *ptr;
  };
} cheese_prop_t;

//
//
//

/**
 * @brief The resolved visual style of a widget (see `core/style.h`).
 * @details The named fields are the universal vocabulary every widget shares.
 * Anything component-specific - including the border and focus ring - is an
 * ordinary property in the @ref props bag, addressed by an interned id (@ref
 * cheese_prop_register), so styling can reach any component of any widget
 * without growing this struct. The core's own properties are declared in @ref
 * cheese_core_style_props_t.
 */
typedef struct {
  cheese_color_t bg_color;
  cheese_color_t hover_color;
  cheese_color_t pressed_color;
  cheese_color_t disabled_color;
  cheese_color_t focus_color;
  cheese_color_t text_color;
  cheese_color_t state_layer_color; // 0 = inherit; overlay for interactions

  cheese_corners_t corner_radius;

  cheese_edges_t padding;
  cheese_edges_t margin;

  u32 font_size;

  i32 cursor;     // -1 = inherit (widget default); else a cheese_cursor_t
  i32 selectable; // -1 = inherit; 0 = off; 1 = on (e.g. text selection)

  cheese_prop_t *props;
  u32 prop_count;
} cheese_style_t;

//
//
//

/**
 * @brief One lexical style scope.
 * @details A scope pairs inherited style values with the classes that apply
 * in that scope. Outer scopes are resolved before inner scopes.
 */
typedef struct {
  cheese_style_t style; // inherited values for this scope
  const cstr *classes;  // space-separated class names, or null
} cheese_style_scope_t;

//
//
//

/**
 * @brief Interned ids of the core's own style properties.
 * @details Registered once by @ref cheese_default, so the core draw helpers
 * (@ref cheese_draw_border, @ref cheese_draw_focus_ring) read the property bag
 * without a name lookup per draw. A consumer gets the same ids by registering
 * the same names (the @c CHEESE_PROP_BORDER_* and @c CHEESE_PROP_FOCUS_RING_*
 * macros), so core and third-party styling share one schema. A `0` id (context
 * not created through @ref cheese_default) simply makes the property absent.
 *
 * @param border_color `"border/color"` - ring colour.
 * @param border_width `"border/width"` - ring thickness.
 * @param border_sides `"border/sides"` - `CHEESE_SIDE_*` mask (absent = all).
 * @param focus_ring_color `"focus_ring/color"` - focus indicator colour.
 * @param focus_ring_width `"focus_ring/width"` - indicator thickness.
 * @param focus_ring_offset `"focus_ring/offset"` - pixels outside the bounds.
 * @param bg_gradient `"bg/gradient"` - colour gradient.
 * @param opacity `"opacity"` - opacity of the background.
 */
typedef struct {
  u32 border_color;
  u32 border_width;
  u32 border_sides;
  u32 focus_ring_color;
  u32 focus_ring_width;
  u32 focus_ring_offset;
  u32 bg_gradient;
  u32 opacity;
} cheese_core_style_props_t;

//
//
//

/** @brief The kind of value a theme variable holds. */
typedef enum {
  CHEESE_THEME_COLOR,
  CHEESE_THEME_F32,
  CHEESE_THEME_U32,
  CHEESE_THEME_I32,
  CHEESE_THEME_B32,
} cheese_theme_type_t;

//
//
//

/** @brief A named theme variable's value. */
typedef struct {
  cheese_theme_type_t type;
  union {
    cheese_color_t color;
    f32 f32;
    u32 u32;
    i32 i32;
    b32 b32;
  };
} cheese_theme_value_t;

//
//
//

/**
 * @brief A semantic colour + metric palette.
 * @details Applied for every frame with @ref cheese_theme_apply; widgets then
 * resolve their built-in role classes (see the `CHEESE_CLASS_*` names) from it,
 * so bare widgets are themed without per-widget colour code. Colours are
 * `0xAARRGGBB` (@ref cheese_color_rgba).
 */
typedef struct {
  cheese_color_t background;
  cheese_color_t surface;
  cheese_color_t surface_variant;
  cheese_color_t on_surface;
  cheese_color_t on_surface_variant;
  cheese_color_t outline;

  cheese_color_t primary;
  cheese_color_t on_primary;
  cheese_color_t primary_container;
  cheese_color_t on_primary_container;

  cheese_color_t error;
  cheese_color_t on_error;
  cheese_color_t success;
  cheese_color_t warning;

  f32 radius_sm, radius_md, radius_lg;
  f32 space_xs, space_sm, space_md, space_lg;
} cheese_theme_t;

//
//
//

/** @brief An eased f32 toward a target. */
typedef struct {
  f32 current;
  f32 target;
  f32 speed; // approach rate in 1/s
} cheese_anim_f32_t;

//
//
//

/**
 * @brief The shared interaction scope of a widget (@ref cheese_widget_begin).
 * @details Identity, bounds and the per-frame interaction facts a widget
 * computes before drawing, so hover/focus/press handling lives in one place and
 * leaves read it instead of re-deriving it. @c state holds the base
 * @c CHEESE_STATE_HOVERED / @c CHEESE_STATE_FOCUSED bits; a leaf ORs in the
 * rest (e.g. @c CHEESE_STATE_CHECKED).
 */
typedef struct {
  u64 id;
  f32 x, y, w, h;
  b32 hovered;
  b32 focused;
  b32 pressed; // hovered with the primary button down
  b32 active;  // the pointer is captured by this widget
  u32 state;   // CHEESE_STATE_HOVERED | CHEESE_STATE_FOCUSED
} cheese_widget_t;

//
//
//

/**
 * @brief Shared state of a toggle-style widget.
 * @details Filled by @ref cheese_toggle_begin and consumed by
 * @ref cheese_toggle_emit and @ref cheese_toggle_label.
 */
typedef struct {
  f32 x, y;      // indicator top-left
  f32 indicator; // indicator size
  f32 total_w;
  f32 line_height;
  b32 hovered, focused;
  b32 activated;     // click / Enter / Space on this widget
  u32 state;         // CHEESE_STATE_* mask
  const cstr *label; // label text
  string *label_str; // label wrapped for the font
} cheese_toggle_t;

//
//
//

/** @brief Maximum number of undo snapshots a text input keeps. */
#define CHEESE_TEXT_UNDO_MAX 32

/** @brief Maximum bytes a text input records into its undo arena. */
#ifndef CHEESE_TEXT_UNDO_BYTES
#define CHEESE_TEXT_UNDO_BYTES (64u * 1024u)
#endif

//
//
//

/**
 * @brief One undo snapshot of a text input.
 *
 * @details Stored by @ref cheese_text_edit in the caller-owned
 *          @ref cheese_text_input_t.undo_arena, so the text outlives the frame
 *          it was recorded in. An entry's presence is its value; the ring is
 *          bounded by @ref CHEESE_TEXT_UNDO_MAX and @ref
 * CHEESE_TEXT_UNDO_BYTES.
 *
 * @param text Borrowed pointer to the snapshot text in the undo arena.
 * @param len Snapshot length in bytes.
 * @param caret Caret position at the snapshot, in codepoints.
 * @param anchor Anchor position at the snapshot, in codepoints.
 */
typedef struct {
  const char *text;
  u32 len;
  i32 caret;
  i32 anchor;
} cheese_text_undo_entry_t;

//
//
//

/** @brief Persistent editing state of a text input (caller-owned). */
typedef struct {
  i32 caret;    // caret position, in codepoints
  i32 anchor;   // selection anchor, in codepoints (== caret when none)
  f32 scroll_x; // horizontal scroll offset, in pixels

  f32 content_h;  // out: the wrapped text height, in pixels
  f32 viewport_h; // out: the field's inner (clip) height, in pixels

  u32 click_count;      // consecutive clicks: 2 selects a word, 3 a line
  f32 click_timer;      // seconds left in the multi-click window
  f32 click_x, click_y; // last press position, to detect a moved pointer

  arena_t *undo_arena; // caller-owned snapshot storage (null disables undo)
  u32 undo_count;      // snapshots stored (past and future)
  u32 undo_pos;        // index of the current snapshot
  u32 undo_bytes;      // bytes allocated from undo_arena so far
  f32 undo_timer;      // seconds left in the coalescing window
  cheese_text_undo_entry_t undo[CHEESE_TEXT_UNDO_MAX]; // snapshot ring

  const cstr *placeholder; // hint drawn while the field is empty (null = none)
  b32 readonly;            // selectable and copyable, but not editable
  b32 masked;              // draw one '*' per codepoint (password fields)

  f32 caret_time; // caret blink phase, in seconds
} cheese_text_input_t;

//
//
//

/** @brief Text input variants. */
typedef enum {
  CHEESE_TEXT_INPUT_LINE,      // single line; Enter is ignored
  CHEESE_TEXT_INPUT_MULTILINE, // Enter inserts a newline; Up/Down move lines
  CHEESE_TEXT_INPUT_WRAP,      // multiline + soft-wrap; no horizontal scroll
} cheese_text_input_variant_t;

//
//
//

/** @brief Caret shapes a text input can draw (see @ref
 * CHEESE_PROP_CARET_STYLE). */
typedef enum {
  CHEESE_TEXT_CARET_BAR = 0, // a vertical bar at the caret
  CHEESE_TEXT_CARET_BLOCK,   // a filled cell behind the glyph
  CHEESE_TEXT_CARET_UNDERLINE,
} cheese_text_caret_style_t;

//
//
//

/** @brief Which way a scrollbar (and its motion) runs. */
typedef enum {
  CHEESE_SCROLLBAR_VERTICAL = 0,
  CHEESE_SCROLLBAR_HORIZONTAL,
} cheese_scrollbar_axis_t;

//
//
//

/** @brief Persistent open/close state shared by popovers (caller-owned). */
typedef struct {
  b32 open;
  u64 opened_frame; // internal: the frame the popup opened on
} cheese_popup_t;

//
//
//

/** @brief Deferred draw callback for an overlay (run at @ref cheese_end). */
typedef void (*cheese_overlay_draw_fn)(cheese_t *cheese, void *userdata);

//
//
//

/** @brief One queued overlay draw. */
typedef struct {
  f32 x, y, w, h;
  cheese_overlay_draw_fn draw;
  void *userdata;
} cheese_overlay_entry_t;

//
//
//

/** @brief Callback invoked when the cursor a hovered widget wants changes. */
typedef void (*cheese_cursor_callback_t)(void *userdata,
                                         cheese_cursor_t cursor);

//
//
//

/**
 * @brief Return the clipboard's current text, or null.
 * @details Borrowed by the caller for the frame; the platform owns it.
 */
typedef const cstr *(*cheese_clipboard_get_fn)(void *userdata);

//
//
//

/** @brief Replace the clipboard's text; the platform copies it. */
typedef void (*cheese_clipboard_set_fn)(void *userdata, const cstr *text);

//
//
//

/**
 * @brief Draws one visible row of a virtualized list.
 *
 * @param cheese The cheese context.
 * @param index The item's index.
 * @param row The row's screen rect.
 * @param hovered Whether the pointer is over this row.
 * @param userdata The userdata passed to @ref cheese_list.
 */
typedef void (*cheese_list_item_fn)(cheese_t *cheese, i32 index,
                                    cheese_rect_t row, b32 hovered,
                                    void *userdata);

//
//
//

/**
 * @brief One selectable text run recorded during a frame.
 *
 * @details Appended by @ref cheese_draw_text when the style in effect is
 *          selectable. Runs are frame-scoped: @ref cheese_begin resets the
 *          list, so @ref text only lives for the frame that recorded it.
 *
 * @param text Borrowed pointer to the drawn text.
 * @param font Font the run was drawn with.
 * @param x Pen origin x of the run.
 * @param baseline Baseline y of the run.
 * @param top Top of the run box.
 * @param bottom Bottom of the run box.
 * @param width Measured width of the whole run.
 * @param hash Hash of the text, used to detect a changed run.
 */
typedef struct {
  const string *text;
  cheese_font_t *font;
  f32 x;
  f32 baseline;
  f32 top;
  f32 bottom;
  f32 width;
  u64 hash;
} cheese_text_run_t;

//
//
//

/**
 * @brief The current text selection.
 *
 * @details Identifies the selected run by its draw-order index and text hash;
 *          @ref anchor and @ref focus are UTF-8 byte offsets into that run's
 *          text. The highlight is drawn by the next frame's @ref
 * cheese_draw_text call for the matching run, so it lags the pointer by one
 * frame.
 *
 * @param active Non-zero when a selection exists.
 * @param run Index of the selected run in the frame's run list.
 * @param anchor Selection anchor (UTF-8 byte offset).
 * @param focus Selection focus (UTF-8 byte offset).
 * @param hash Text hash captured when the selection started.
 */
typedef struct {
  b32 active;
  u32 run;
  u32 anchor;
  u32 focus;
  u64 hash;
} cheese_selection_t;

//
//
//

#ifndef CHEESE_STACK_MAX_DEPTH
#define CHEESE_STACK_MAX_DEPTH 64
#endif

//
//
//

/** @brief The per-frame cheese context (see `core/init.h`). */
struct cheese {
  cheese_renderer_t *renderer;
  arena_t *frame_arena;
  arena_t *arena; // persistent arena (property registry lives here)
  cheese_state_store_t *store;

  stringmap_t *props; // persistent name -> cheese_prop_def_t;
  u32 prop_next;      // next property id (ids are 1-based; 0 = none)
  cheese_core_style_props_t core_props; // interned ids of the core properties

  cheese_semantics_node_t *semantics_nodes; // per-frame darray
  u64 semantics_order;
  i32 semantics_current_parent;

  cheese_semantics_node_t *semantics_snapshot; // last captred tree copy
  u64 semantics_snapshot_count;
  arena_t *semantics_snapshot_arena; // caller-provided; null = disabled
  mtx_t semantics_snapshot_mtx;
  b32 semantics_snapshot_init; // mutex initialised (on enable)
  b32 semantics_snapshot_held; // held between begin and end

  b32 arrow_nav; // arrow keys move focus spatially (default on)
  u64 focus_id;  // persistent semantics id of the keyboard-focused widget
  u64 edit_id;   // semantics id currently owning the keyboard (0 = nav mode)
  u64 active_id; // persistent semantics id owning the pointer (drag)
  f32 press_x, press_y; // cursor position when the pointer was captured

  cheese_text_run_t *text_runs; // per-frame selectable runs (frame arena)
  u32 text_run_count;
  u32 text_run_cap;
  cheese_selection_t selection; // current text selection

  u32 key_event_count;
  cheese_key_event_t key_events[CHEESE_MAX_KEY_EVENTS];

  cheese_input_t last_input; // for cheese_input()'s diff (app thread only)
  b32 input_seen;
  b32 input_frame_needed;

  f32 mouse_x, mouse_y;
  f32 scroll_x, scroll_y;

  u32 mouse_buttons;
  u32 mouse_prev_buttons;
  u32 key_mods;

  u64 frame_count;
  f32 delta_time;

  cheese_layout_t layout_stack[CHEESE_STACK_MAX_DEPTH];
  u32 layout_stack_depth;

  cheese_value_t scroll_stack_x[CHEESE_STACK_MAX_DEPTH];
  cheese_value_t scroll_stack_y[CHEESE_STACK_MAX_DEPTH];
  u32 scroll_stack_depth;

  stringmap_t *class_styles;
  stringmap_t *theme; // per-frame name -> cheese_theme_value_t

  cheese_style_scope_t scope_stack[CHEESE_STACK_MAX_DEPTH];
  u32 scope_depth;

  cheese_theme_t theme_palette; // active palette (persists across frames)
  b32 theme_applied;            // cheese_theme_apply has been called

  b32 style_dirty;
  cheese_style_t resolved_style;

  cheese_style_t role_styles[CHEESE_ROLE_MAX]; // typed per-role defaults
  b32 role_set[CHEESE_ROLE_MAX];               // which are registered

  cheese_cursor_callback_t cursor_callback;
  void *cursor_callback_userdata;

  cheese_clipboard_get_fn clipboard_get;
  cheese_clipboard_set_fn clipboard_set;
  void *clipboard_userdata;

  cheese_cursor_t cursor_request; // this frame's requested cursor
  b32 cursor_requested;           // a widget asked this frame
  i32 cursor_last;                // last emitted cursor (-1 = none yet)

  cheese_overlay_entry_t *overlays; // per-frame darray, drawn last (top)
  cheese_rect_t overlay_rect;       // this frame's topmost overlay bounds
  cheese_rect_t prev_overlay_rect;  // last frame's, for input blocking
  b32 overlay_blocked;              // pointer under a popover: mouse is muted
  f32 raw_mouse_x, raw_mouse_y;     // true pointer, for overlay hit-testing
};

//
//
//

/** @brief Outcome of an app callback. */
typedef enum {
  CHEESE_CONTINUE,
  CHEESE_SUCCESS,
  CHEESE_FAILURE,
} cheese_app_result_t;

//
//
//

/**
 * @brief Optional app callbacks.
 * @details A thin, optional convenience over the raw API. The consumer owns
 * `main()`, the window, events and the frame pump; it calls these pumps from
 * its own loop. Skipping the harness and calling the raw API is equally
 * valid.
 */
typedef struct {
  void *userdata;
  cheese_app_result_t (*start)(cheese_t *cheese, void *userdata);
  cheese_app_result_t (*update)(cheese_t *cheese, f32 dt, void *userdata);
  void (*render)(cheese_t *cheese, void *userdata);
  void (*stop)(cheese_t *cheese, void *userdata);
} cheese_app_t;

#endif // !CHEESE_TYPES_H
