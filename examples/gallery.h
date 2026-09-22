#ifndef EXAMPLES_GALLERY_H
#define EXAMPLES_GALLERY_H

/**
 * @brief Shared widget sequence for every renderer example.
 * @details Backend-neutral: owns persistent widget state (stable state names),
 * a backend-neutral tick, and the draw calls in a fixed order. Each example
 * keeps its own window, input, renderer, cursor, theme, and main loop, and
 * calls gallery_init once, gallery_tick per frame, and gallery_draw between
 * cheese_begin/cheese_end. For butter, the gallery struct lives on the render
 * thread: init/tick/draw only inside app_render; the app thread touches only
 * the thread-safe state store.
 *
 * Coverage: button, checkbox + radio (toggle core), slider, scrollbar,
 * progress, LINE text, dropdown (overlay core), tabs + image content
 * (composition), virtualized list, procedural image (upload seam), and the
 * toast pattern. The meta plus is composition, not new core: button -> toast,
 * tabs -> per-tab image fit.
 */

#include <htils/string.h>

#include <cheese.h>

/***********************************/

/** @brief Persistent gallery state (caller-owned, one per example). */
typedef struct {
  cheese_font_t *font;
  cheese_state_t *checkbox;
  cheese_state_t *slider;
  cheese_state_t *progress;
  cheese_state_t *text;
  cheese_text_input_t text_state;
  cheese_state_t *choice;
  cheese_state_t *dropdown;
  cheese_popup_t dropdown_state;
  cheese_state_t *tab;
  cheese_state_t *list_scroll;
  cheese_state_t *bar_scroll;
  cheese_texture_t image;
  cheese_toast_stack_t toasts;
  cheese_state_t *text_wrap;
  cheese_text_input_t text_wrap_state;
  cheese_state_t *text_scroll;
  cheese_state_t *password;
  cheese_text_input_t password_state;
  cheese_text_input_t readonly_state;
} example_gallery_t;

/***********************************/

static const cstr *gallery_list_labels[] = {
    "alpha",  "bravo",   "charlie", "delta",  "echo",   "foxtrot", "golf",
    "hotel",  "india",   "juliet",  "kilo",   "lima",   "mike",    "november",
    "oscar",  "papa",    "quebec",  "romeo",  "sierra", "tango",   "uniform",
    "victor", "whiskey", "xray",    "yankee", "zulu",
};
#define GALLERY_LIST_COUNT 26
#define GALLERY_ROW_H 20.0f

static const cstr *gallery_options[] = {"One", "Two", "Three"};

/***********************************/

/** @brief Draw one list row: hover highlight + deterministic label. */
static void gallery_list_item(cheese_t *cheese, i32 index, cheese_rect_t row,
                              b32 hovered, void *userdata) {
  example_gallery_t *g = (example_gallery_t *)userdata;
  if (index < 0 || index >= GALLERY_LIST_COUNT)
    return;
  if (hovered)
    cheese_draw_rect(cheese, (cheese_corners_t){0}, (f32)row.x, (f32)row.y,
                     (f32)row.w, (f32)row.h,
                     cheese_color_rgba(88, 91, 112, 255));
  if (g->font) {
    const string *s =
        string_from_cstr(cheese->frame_arena, gallery_list_labels[index]);
    cheese_draw_text(cheese, (f32)row.x + 4.0f, (f32)row.y + (f32)row.h - 5.0f,
                     s, g->font, cheese_color_rgba(255, 255, 255, 255), 1.0f);
  }
}

/***********************************/

/** @brief Upload the 2x2 checker once; later frames reuse the handle. */
static void gallery_ensure_image(cheese_t *cheese, example_gallery_t *g) {
  if (g->image.id != 0)
    return;
  static const u8 pixels[4 * 4] = {
      255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
  };
  g->image = cheese_texture_upload(cheese, 2, 2, pixels);
}

/***********************************/

/**
 * @brief Create the gallery's states with stable names.
 * @param g The gallery to initialise.
 * @param store The cheese store (persistent arena).
 * @param arena Persistent arena for the toast stack.
 * @param font The loaded font the widgets draw with.
 */
static inline void gallery_init(example_gallery_t *g,
                                cheese_state_store_t *store, arena_t *arena,
                                cheese_font_t *font) {
  g->font = font;
  g->checkbox = cheese_state_b32(store, "checkbox", false);
  g->slider = cheese_state_f32(store, "slider", 0.4f);
  g->progress = cheese_state_f32(store, "progress", 0.0f);
  g->text = cheese_state_str(store, "text", "Edit me!");
  g->text_state = (cheese_text_input_t){0};
  g->text_state.undo_arena = arena;
  g->text_wrap = cheese_state_str(
      store, "text_wrap",
      "Cheese soft-wraps at the field edge. Type a long line and watch it "
      "fold onto the next row; the caret stays in view and the field scrolls "
      "when the text outgrows the box.");
  g->text_wrap_state = (cheese_text_input_t){0};
  g->text_wrap_state.undo_arena = arena;
  g->text_scroll = cheese_state_f32(store, "text_scroll", 0.0f);
  g->password_state = (cheese_text_input_t){0};
  g->password_state.undo_arena = arena;
  g->password_state.masked = true;
  g->password = cheese_state_str(store, "password", "hunter2");
  g->password_state.placeholder = "password";
  g->readonly_state = (cheese_text_input_t){0};
  g->readonly_state.readonly = true;
  g->choice = cheese_state_i32(store, "choice", 0);
  g->dropdown = cheese_state_i32(store, "dropdown", 0);
  g->dropdown_state = (cheese_popup_t){0};
  g->tab = cheese_state_i32(store, "tab", 0);
  g->list_scroll = cheese_state_f32(store, "list_scroll", 0.0f);
  g->bar_scroll = cheese_state_f32(store, "bar_scroll", 0.0f);
  g->image = (cheese_texture_t){0};
  cheese_toast_stack_init(&g->toasts, arena, 4);
}

/**
 * @brief Advance the progress bar and age the toasts.
 * @details Backend-neutral; no window/renderer access. Wraps at 1.0.
 * @param g The gallery.
 * @param dt Seconds since the previous frame.
 */
static inline void gallery_tick(example_gallery_t *g, f32 dt) {
  if (cheese_state_get_b32(g->checkbox)) {
    f32 progress = cheese_state_get_f32(g->progress) + dt * 0.25f;
    if (progress > 1.0f)
      progress = 0.0f;
    cheese_state_set_f32(g->progress, progress);
  }
  cheese_toast_tick(&g->toasts, dt);
}

/**
 * @brief Draw the shared sequence between cheese_begin and cheese_end.
 * @details Same public calls, semantics, classes, and order on every backend:
 * button, checkbox, slider, bar, text, radios, dropdown, tabs + image,
 * list, scrollbar, toasts.
 * @param cheese The cheese context.
 * @param g The gallery.
 */
static inline void gallery_draw(cheese_t *cheese, example_gallery_t *g) {
  gallery_ensure_image(cheese, g);

  cheese_layout_t vertical = *cheese_current_layout(cheese);
  cheese_layout_set_horizontal(&vertical, false);
  cheese_push_layout(cheese, vertical);

  cheese_begin_container_auto(cheese, null, (cheese_semantics_t){.key = "root"},
                              cheese_fill(), cheese_fill());
  {
    if (cheese_button_auto(cheese, null,
                           (cheese_semantics_t){.name = "say hello"},
                           cheese_val_str("Meow!"), g->font)) {
      cheese_log_info("Meow :3");
      cheese_toast_push(&g->toasts, "Meow :3", 2.0f);
    }

    cheese_checkbox_auto(cheese, null,
                         (cheese_semantics_t){.name = "advance progress"},
                         cheese_val_state(g->checkbox),
                         cheese_val_str("Advance progress"), g->font);

    cheese_slider_auto(cheese, null, (cheese_semantics_t){.name = "slider"},
                       cheese_val_state(g->slider));

    {
      cheese_style_t bar = cheese_style_new();
      cheese_style_prop_set_f32(cheese, &bar, cheese->core_props.opacity,
                                0.75f);
      cheese_progress_set_fill_gradient(
          cheese, &bar,
          (cheese_gradient_t){.top_left = cheese_color_hex(0xFF66D9EF),
                              .top_right = cheese_color_hex(0xFFA6E22E),
                              .bottom_right = cheese_color_hex(0xFFF92672),
                              .bottom_left = cheese_color_hex(0xFFFD971F)});
      cheese_push_style(cheese, bar);
      cheese_progress_bar_auto(cheese, null, (cheese_semantics_t){0},
                               cheese_val_state(g->progress));
      cheese_pop_style(cheese);
    }

    cheese_text_input_auto(
        cheese, null, (cheese_semantics_t){.name = "text field"},
        cheese_val_state(g->text), &g->text_state, cheese_val_f32(0.0f),
        CHEESE_TEXT_INPUT_LINE, g->font);

    {
      f32 x, y, w = 0.0f, h = 0.0f;
      cheese_layout_place(cheese, &w, &h, &x, &y);
      f32 bar_w = 12.0f;
      cheese_text_input_auto(
          cheese, null, (cheese_semantics_t){.name = "wrapped field"},
          cheese_val_state(g->text_wrap), &g->text_wrap_state,
          cheese_val_state(g->text_scroll), CHEESE_TEXT_INPUT_WRAP, g->font);
      cheese_scrollbar(cheese, null, (cheese_semantics_t){.name = "wrap bar"},
                       x + w - bar_w, y, bar_w, h,
                       cheese_val_state(g->text_scroll),
                       g->text_wrap_state.viewport_h,
                       g->text_wrap_state.content_h, CHEESE_SCROLLBAR_VERTICAL);
    }

    cheese_text_input_auto(
        cheese, null, (cheese_semantics_t){.name = "password"},
        cheese_val_state(g->password), &g->password_state, cheese_val_f32(0.0f),
        CHEESE_TEXT_INPUT_LINE, g->font);

    cheese_text_input_auto(
        cheese, null, (cheese_semantics_t){.name = "read only"},
        cheese_val_str("read only, selectable"), &g->readonly_state,
        cheese_val_f32(0.0f), CHEESE_TEXT_INPUT_LINE, g->font);

    cheese_radio_auto(cheese, null, (cheese_semantics_t){.name = "choice nyan"},
                      cheese_val_state(g->choice), 0, cheese_val_str("Nyan"),
                      g->font);
    cheese_radio_auto(cheese, null, (cheese_semantics_t){.name = "choice wan"},
                      cheese_val_state(g->choice), 1, cheese_val_str("Wan"),
                      g->font);
    cheese_radio_auto(cheese, null, (cheese_semantics_t){.name = "choice moo"},
                      cheese_val_state(g->choice), 2, cheese_val_str("Moo"),
                      g->font);

    cheese_dropdown_auto(cheese, null, (cheese_semantics_t){.name = "pick one"},
                         cheese_val_state(g->dropdown), gallery_options, 3,
                         &g->dropdown_state, g->font);

    cheese_tab_bar_auto(cheese, null, (cheese_semantics_t){.name = "tabs"},
                        cheese_val_state(g->tab), gallery_options, 3, g->font);

    {
      f32 w = 160.0f, h = 100.0f, x = 0.0f, y = 0.0f;
      cheese_layout_place(cheese, &w, &h, &x, &y);
      cheese_fit_t fit = CHEESE_FIT_STRETCH;
      if (cheese_state_get_i32(g->tab) == 1)
        fit = CHEESE_FIT_CONTAIN;
      else if (cheese_state_get_i32(g->tab) == 2)
        fit = CHEESE_FIT_COVER;
      cheese_image(cheese, null, (cheese_semantics_t){.name = "checker"}, x, y,
                   w, h, g->image, fit, 0);
    }

    cheese_begin_container_auto(cheese, null,
                                (cheese_semantics_t){.key = "list-box"},
                                cheese_fill(), cheese_px(150.0f));
    {
      cheese_layout_t *box = cheese_current_layout(cheese);
      f32 bx = box->x, by = box->y, bw = box->width, bh = box->height;
      f32 bar_w = 12.0f;

      cheese_list(cheese, null, (cheese_semantics_t){.key = "list"}, bx, by,
                  bw - bar_w, bh, cheese_val_state(g->list_scroll),
                  GALLERY_LIST_COUNT, GALLERY_ROW_H, gallery_list_item, g);

      cheese_scrollbar(
          cheese, null, (cheese_semantics_t){.name = "list bar"},
          bx + bw - bar_w, by, bar_w, bh, cheese_val_state(g->list_scroll), bh,
          (f32)GALLERY_LIST_COUNT * GALLERY_ROW_H, CHEESE_SCROLLBAR_VERTICAL);
    }
    cheese_end_container(cheese);
    cheese_scrollbar_auto(cheese, null,
                          (cheese_semantics_t){.name = "demo bar"},
                          cheese_val_state(g->bar_scroll), 120.0f, 400.0f,
                          CHEESE_SCROLLBAR_HORIZONTAL);
  }
  cheese_end_container(cheese);

  cheese_toast_draw(cheese, null, &g->toasts, g->font);

  cheese_pop_layout(cheese);
}

#endif // !EXAMPLES_GALLERY_H
