# Getting started

This is the how-to for building an app with cheese. For what cheese is and why
it works the way it does, see the README. You will go from an empty file to a
small interactive UI, then look at the recipes for common widgets.

The shape of every cheese app is the same:

1. Link cheese.
2. Provide a renderer (the `cheese_renderer_t` vtable).
3. Create the context, theme and font.
4. Write a frame function that draws the UI.
5. Run a loop that feeds input and requests redraws.

## 1. Link cheese

Install or build cheese (release), then link it with its dependencies:
`htils-threadsafe`, `harfbuzz`, `freetype`, `m`. If you use pkg-config, the
installed `.pc` file provides the flags. Include the umbrella header:

```c
#include <cheese.h>
```

## 2. Provide a renderer

cheese ships no backend. You supply one by filling `cheese_renderer_t`. For a
first compile-and-run you can use a no-op renderer. It draws nothing, but the
program runs and the semantics tree is populated:

```c
static void noop(void *userdata) { (void)userdata; }

static cheese_renderer_t make_offscreen_renderer(void) {
  cheese_renderer_t r = {
    .flush_deferred = noop,
    .flush_draws = noop,
  };
  return r;
}
```

`flush_deferred` is called on every `cheese_begin`, so it must be non-null. The
other callbacks are optional; a missing one logs and no-ops. For real pixels,
fill `draw_rect`, `draw_border`, `draw_line`, `draw_arc`, `draw_text`,
`draw_texture`, `push_clip`/`pop_clip` and the texture functions, or use a ready
backend. See "Renderer plugins" in the README for the full contract.

## 3. Create the context

```c
arena_t *perm       = arena_new(GiB(1), MiB(1));  // lives for the app
arena_t *frame      = arena_new(GiB(1), MiB(1));  // cleared per frame
arena_t *font_arena = arena_new(GiB(1), MiB(1));  // font and glyph atlas

cheese_t cheese = cheese_default(perm);
cheese_renderer_t renderer = make_offscreen_renderer();

cheese_theme_t theme = cheese_theme_dark();       // or cheese_theme_light()
cheese_theme_apply(&cheese, &theme);              // persists across frames
```

Load a font if you want text:

```c
cheese_font_system_init();
cheese_font_t *font =
  cheese_load_font(&renderer, font_arena,
                   string_from_cstr(font_arena, "/path/to/font.ttf"), 16);
```

## 4. Write the frame function

Everything you draw goes between `cheese_begin` and `cheese_end`:

```c
static void draw_ui(cheese_t *c, cheese_font_t *font,
                    cheese_renderer_t *renderer, arena_t *frame,
                    cheese_input_t input, f32 dt) {
  cheese_begin(c, frame, font, renderer, input, dt);
  { // Scopes are optional but give structure.
    cheese_begin_container_auto(c, "panel", (cheese_semantics_t){0},
                                cheese_fill(), cheese_fill());
    {
      // Draws within the panel's bounds.
      if (cheese_button_auto(c, null, (cheese_semantics_t){0},
                             cheese_val_str("Play"), font))
        play();
    }
    cheese_end_container(c);
  }
  cheese_end(c);
}
```

`cheese_begin` resets per-frame state and processes input; `cheese_end` flushes
overlays and the draw queue.

## 5. Run the loop

On your main thread you poll the window and feed input. `cheese_input` diffs the
frame's input and sets a "frame needed" flag for you:

```c
while (!window_should_close()) {
  poll_events();                             // fills mouse and keys

  cheese_input_t in = build_input();
  in.window_w = drawable_w;                  // the draw target size
  in.window_h = drawable_h;

  cheese_input(&cheese, in);                 // app thread
  if (cheese_needs_frame(&cheese)) {         // input or bound state changed
    cheese_clear_frame_needed(&cheese);
    request_a_frame();                       // wake your renderer/window pump
  }
}
```

The draw pump eventually calls your frame function on the thread that owns the
draw target, then clears the frame arena once that frame has completed. Do not
clear the frame arena while a frame is still in flight.

## 6. Layout and containers

Place widgets yourself (x, y, w, h) or use the `_auto` variants, which place at
the current layout cursor. A container re-anchors the cursor for its children.
To change the flow inside a container, push a copy of the current layout:

```c
cheese_layout_t col = *cheese_current_layout(&cheese);
cheese_layout_set_horizontal(&col, false);
cheese_layout_set_spacing(&col, 8.0f);
cheese_push_layout(&cheese, col);
// ... children ...
cheese_pop_layout(&cheese);
```

Sizes are specs: `cheese_px(n)`, `cheese_pct(f)`, `cheese_fill()`,
`cheese_fit()`, each with optional min and max clamps.

## 7. State and interactivity

Create typed values in the context's store, then bind widgets to them:

```c
cheese_state_t *playing  = cheese_state_b32(cheese.store, "playing", false);
cheese_state_t *progress = cheese_state_f32(cheese.store, "progress", 0.f);

cheese_checkbox_auto(&cheese, null, (cheese_semantics_t){.key = "play"},
                     cheese_val_state(playing),
                     cheese_val_str("Playing"), font);

cheese_progress_bar_auto(&cheese, null, (cheese_semantics_t){0},
                         cheese_val_state(progress));
```

Reading a bound value during a frame marks it draw-relevant, so changing it wakes
a frame. React on the app thread with a subscription:

```c
static void on_playing(cheese_state_t *s, void *ud) {
  // start or stop playback
}

cheese_state_subscribe(playing, on_playing, &app);
// each tick, on the app thread:
cheese_state_dispatch(cheese.store);
```

## 8. Styling and theming

A widget with `classes = null` uses its built-in role class and is themed by the
applied palette. Register a named class for your own look:

```c
cheese_style_t panel = cheese_style_new();
cheese_style_set_bg_color(&panel, cheese_color_rgb(30, 30, 40));
cheese_style_set_padding_uniform(&panel, 12.0f);
cheese_style_set_corner_radius_uniform(&panel, 8.0f);
cheese_style_class_register(&cheese, "panel", panel);
```

Class registration is per-frame - the map is cleared in `cheese_begin` - so put
it at the top of your frame function. Then any widget that names the class picks
it up, at any depth, without touching its draw code:

```c
cheese_begin_container_auto(&cheese, "panel", (cheese_semantics_t){0},
                            cheese_fill(), cheese_fill());
```

For a field no standard setter covers, a widget uses the typed property bag. It
registers its own property names (ids are stable and idempotent) and reads them
back with a fallback:

```c
u32 thumb = cheese_prop_register(&cheese, "slider/thumb/color", CHEESE_PROP_COLOR);
```

To push values for a whole subtree, use `cheese_push_style` / `cheese_pop_style`
(or `cheese_push_scope`, which pushes values and classes together) - again after
`cheese_begin`.

Switching theme is one call, and it persists:

```c
cheese_theme_t light = cheese_theme_light();
cheese_theme_apply(&cheese, &light);
```

## 9. Recipes

A labelled checkbox that drives a worker. The binding is written for you, so
there is no need to branch on the return value:

```c
cheese_checkbox_auto(&cheese, null, (cheese_semantics_t){.key = "w"},
                     cheese_val_state(worker_on),
                     cheese_val_str("Run worker"), font);
```

A slider, bound to an f32 in `0..1`:

```c
cheese_slider_auto(&cheese, null, (cheese_semantics_t){.key = "vol"},
                   cheese_val_state(volume));
```

A single-line text field. The caret and scroll state is caller-owned, so it
persists across frames:

```c
static cheese_text_input_t sst = {0};
cheese_text_input_auto(&cheese, null, (cheese_semantics_t){.key = "q"},
                       cheese_val_state(query), &sst,
                       cheese_val_f32(0.0f), CHEESE_TEXT_INPUT_LINE, font);
```

The field supports the usual editing keys (selection, Ctrl+A/C/X/V, undo).
A wrapped, scrollable text area with a scrollbar next to it. The field and the
scrollbar share the same bound scroll value, and the field publishes its
extents:

```c
static cheese_text_input_t mst = {0};
cheese_text_input_auto(&cheese, null, (cheese_semantics_t){.key = "notes"},
                       cheese_val_state(notes), &mst,
                       cheese_val_state(notes_scroll),
                       CHEESE_TEXT_INPUT_WRAP, font);

// Elsewhere in the layout, sized from the field's published extents. Draw the
// field first so content_h and viewport_h are up to date this frame.
cheese_scrollbar_auto(&cheese, null, (cheese_semantics_t){0},
                      cheese_val_state(notes_scroll),
                      mst.viewport_h, mst.content_h,
                      CHEESE_SCROLLBAR_VERTICAL);
```

A virtualized list. Only rows intersecting the viewport are drawn:

```c
static void row(cheese_t *c, i32 i, cheese_rect_t r, b32 hovered, void *ud) {
  // draw the row yourself
}

cheese_list_auto(&cheese, null, (cheese_semantics_t){.key = "tracks"},
                 cheese_val_state(list_scroll), track_count, 28.0f,
                 row, &app);
```

A dropdown. The open state is caller-owned, so it persists across frames:

```c
static cheese_popup_t dd = {0};
cheese_dropdown_auto(&cheese, null, (cheese_semantics_t){.key = "album"},
                     cheese_val_state(album_index), items, item_count,
                     &dd, font);
```

## 10. Keyboard, focus and edit mode

Tab and the arrow keys move focus. A focused widget does not take the keyboard
until it is activated with a click or Enter/Space, and then Escape returns to
navigation. You usually get this for free. Call `cheese_focus_move_dir` or
`cheese_set_arrow_nav` only if you want to customize it.

## Next steps

- README: concepts (state, styling, semantics, renderer contract, memory).
- Tests under `src/tests/` are small, self-contained examples of every widget.
- For real pixels, pick a backend. The ready set: GLES3, SDL2, SDL3, raylib,
  sokol, cairo, web, win32_gdi; Want to use something else? The vtable is small enough that anyone can add
  one.
