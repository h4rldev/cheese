# cheese

An immediate-mode GUI library for C. You rebuild the UI every frame with plain
function calls, and cheese draws it through a renderer plugin.

cheese is a standalone library. Its only dependency is
[htils](https://github.com/anomalyco/htils), a general C utility library (arenas,
strings, threading). cheese contains no windowing code and no graphics backend.
You give it a `cheese_renderer_t` (a small vtable your chosen backend fills) and
a `cheese_input_t` each frame; the cursor is the only thing it gives back.

A windowing layer and a renderer are the consumer's choice. bread (windowing) and
butter (Vulkan) are one example pair, but cheese has no dependency on either and
works with anything that fills the vtable.

## Philosophy

- Immediate mode: the UI is code that runs every frame, not a retained tree.
- Arena allocation: no per-widget heap churn; state is reclaimed in bulk.
- Renderer agnostic: `cheese_renderer_t` is the only backend seam.
- Windowing and input agnostic: `cheese_input_t` in, cursor out via callback.
- Accessibility intrinsic: widgets emit a per-frame semantics tree as they draw.

## Build

cheese builds with [conjure](https://github.com/EddLabs/conjure). Profiles live
in `conjure.kdl`:

```sh
conjure as release build
conjure as release-static build
conjure as debug build
conjure as debug test          # builds the headless test binaries
./bin/theme/debug/theme        # tests are plain executables, run them yourself
```

The release installs the library, headers and a pkg-config file. Link cheese
together with `htils-threadsafe`, `harfbuzz`, `freetype` and `m`. The public API
is `cheese_`-prefixed. conjure tracks header changes, so a clean rebuild is never
required after editing a header.

On NixOS there is also a flake (`nix build .#cheese`, `.#cheese-static`,
`.#cheese-debug`), but that is one convenient way to build, not a requirement.
cheese does not depend on Nix.

## Platform support

cheese core contains no OS code and no windowing code, so it is not tied to any
platform. It compiles wherever C, FreeType and HarfBuzz do, and it renders
wherever a backend implements the vtable.

What is Unix-like only today is the surrounding setup: the bundled example stack
and the ready backends currently target Linux (Wayland and X11). macOS and
Windows backends are planned, and will land once the first real consumer is in a
working state. Because the backend is the only platform seam, adding them does
not change cheese's core or the UI code you write.

## Quickstart

```c
#include <cheese.h>

arena_t *perm  = arena_new(GiB(1), MiB(1));   // once
arena_t *frame = arena_new(GiB(1), MiB(1));

cheese_t cheese = cheese_default(perm);       // once
cheese_theme_t theme = cheese_theme_dark();
cheese_theme_apply(&cheese, &theme);          // once, persists

// each frame, from the thread that owns your draw target:
cheese_begin(&cheese, frame, font, &renderer, input, dt);
{ // Scopes are optional but give structure.
  cheese_begin_container_auto(&cheese, "panel", (cheese_semantics_t){0}, cheese_fill(), cheese_fill());
  {
    // Draws within the panel's bounds.
    if (cheese_button_auto(&cheese, null, (cheese_semantics_t){0}, cheese_val_str("Play"), font))
      play();
  }
  cheese_end_container(&cheese);
}
cheese_end(&cheese);
```

## Lifecycle

1. Arenas. One persistent arena for the context and its state store, one frame
   arena for transient data. Clear the frame arena only after the in-flight
   frame has finished.

```c
arena_t *perm  = arena_new(GiB(1), MiB(1));
arena_t *frame = arena_new(GiB(1), MiB(1));
```

2. Renderer. Fill a `cheese_renderer_t` from your backend (see Renderer
   plugins). cheese holds only the vtable.

```c
cheese_renderer_t renderer = my_backend_renderer(&my_backend);
```

3. Font. Load once; the glyph atlas lives in its own arena. Text uses FreeType
   and HarfBuzz inside cheese.

```c
cheese_font_system_init();
cheese_font_t *font = cheese_load_font(&renderer, perm,
  string_from_cstr(perm, "/path/to/font.ttf"), 16);
```

4. Context and theme.

```c
cheese_t cheese = cheese_default(perm);
cheese_theme_t theme = cheese_theme_dark();  // or _light()
cheese_theme_apply(&cheese, &theme);
```

5. Frame loop. Call `cheese_begin` and `cheese_end` once per rendered frame,
   from the thread that owns the draw target. Pass this frame's input by value.

## Input

`cheese_input_t` carries mouse position and buttons, wheel, drawable size, key
mods, and this frame's key events:

```c
cheese_input_t input = {
  .mouse_x = mx, .mouse_y = my, .mouse_buttons = buttons,
  .scroll_x = sx, .scroll_y = sy,
  .window_w = drawable_w,   // size of the image being drawn this frame
  .window_h = drawable_h,
  .key_mods = mods,
};
```

`cheese_begin` takes it by value. On your app thread you can also call
`cheese_input(&cheese, input)`, which diffs against the previous call and marks
a frame needed for you, so you do not track `needs_redraw` for input:

```c
cheese_input(&cheese, input);        // app thread
if (cheese_needs_frame(&cheese)) {   // worker and state changes included
  cheese_clear_frame_needed(&cheese);
  request_a_frame();                 // wake your renderer/window pump
}
```

The only output back out is the cursor:

```c
static void on_cursor(void *ud, cheese_cursor_t c) { /* set OS cursor */ }
cheese_set_cursor_callback(&cheese, on_cursor, &state);
```

`input.window_w` and `input.window_h` must be the size of the image being drawn,
not a window that may be mid-resize; the two disagree for a frame otherwise.

## Widgets

Every widget takes style `classes` (a space-separated list; `null` means its
built-in role class), a `cheese_semantics_t` for accessibility identity (`{0}`
is fine), and a `cheese_value_t` data binding. The `_auto` variants place
themselves in the current layout.

    cheese_button[_auto]        label, returns b32 activated
    cheese_checkbox[_auto]      b32 binding
    cheese_radio[_auto]         i32 index binding
    cheese_slider[_auto]        f32 binding (0..1)
    cheese_progress_bar[_auto]  f32 binding (0..1)
    cheese_text_input[_auto]    str binding, caret and scroll state
    cheese_dropdown[_auto]      i32 index and items
    cheese_tab_bar[_auto]       i32 index and labels
    cheese_list[_auto]          virtualized rows, f32 scroll
    cheese_image[_auto]         cheese_texture_t
    cheese_scrollbar[_auto]     f32 pixel scroll, viewport and content
    cheese_begin_container[_auto] / cheese_end_container
    cheese_begin_scroll / cheese_end_scroll

The leaves are thin wrappers over shared behaviour cores, which a third party
can reuse to build a new widget without touching cheese:

    cheese_toggle_begin / _emit / _label   checkbox, radio, switch
    cheese_text_edit                       text-editing core
    cheese_slider_begin                    slider drag/key behaviour
    cheese_scrollbar_begin                 scrollbar press/drag/key behaviour
    cheese_popup_open / _close / _dismissed  open-state and dismissal
    cheese_overlay / cheese_overlay_modal  deferred unclipped draw + occlusion

Text fields come in three variants: `CHEESE_TEXT_INPUT_LINE`,
`CHEESE_TEXT_INPUT_MULTILINE`, and `CHEESE_TEXT_INPUT_WRAP` (soft wrap plus
vertical scroll). Wrapped fields publish `state->content_h` and
`state->viewport_h`, so you can place a `cheese_scrollbar` anywhere and share
the same bound scroll value.

Text editing is built on the shared `cheese_text_edit` core: caret and selection
placement (click, drag, double and triple click, shift / word / line
selection), Ctrl+A/C/X/V, Ctrl+Z and Ctrl+Shift+Z / Ctrl+Y undo-redo, and
Ctrl+arrows with Ctrl+Backspace/Delete for word operations. The per-field
`cheese_text_input_t` is caller-owned and carries caret, anchor, scroll, the
undo ring, and the `placeholder` / `readonly` / `masked` flags. Caret look is
styled by properties: `CHEESE_PROP_CARET_COLOR`, `_WIDTH`, `_STYLE` (bar,
block, underline) and `_BLINK` (seconds; `0` is steady).

Selectable text outside a text field is handled by a global selection layer:
`cheese_draw_text` records a run whenever the resolved style is selectable
(`cheese_style_set_selectable`), and press or drag over it selects. Copy is
Ctrl+C, and the highlight is drawn by the next frame.

## Layout

Layout is separate from style. The current layout is a cursor plus extents, and
containers re-anchor it. Push a copy of the current layout when you want a
different flow inside a container:

```c
cheese_layout_t v = *cheese_current_layout(&cheese);
cheese_layout_set_horizontal(&v, false);
cheese_push_layout(&cheese, v);
```

Sizes are specs: `cheese_px(n)`, `cheese_pct(f)`, `cheese_fill()`,
`cheese_fit()`. `cheese_layout_place` measures and places a widget at the
cursor; `cheese_push_anchor` places the next widget at a corner without
advancing the cursor.

## State and bindings

State lives in a standalone, thread-safe store created with the context:

```c
cheese_state_t *running  = cheese_state_b32(cheese.store, "running", false);
cheese_state_t *progress = cheese_state_f32(cheese.store, "progress", 0.f);
```

A value becomes bound when a widget reads it, and only bound values wake a
redraw on change. Widgets read and write through `cheese_value_t`:

```
cheese_val_str("Play")        // literal
cheese_val_state(running)     // binding: read binds, write sets
cheese_val_f32(0.5f)
```

Subscriptions run on the app thread when a value changes:

```c
cheese_state_subscribe(progress, on_progress, state);
cheese_state_dispatch(cheese.store);   // app thread, once per tick
```

## Styling

`cheese_style_t` is visual only and has two layers: a small universal
vocabulary, and an open typed property bag for widget-owned fields.

The vocabulary is what every widget needs: colours (`bg`, `hover`, `pressed`,
`disabled`, `focus`, `text`, `state_layer`), geometry (`corner_radius`,
`padding`, `margin`), and the per-widget `font_size`, `cursor` and `selectable`.
An unset field inherits: colours use `0`, floats use `-1.0f`, `font_size` uses
`0`, `cursor` and `selectable` use `-1`. Build with `cheese_style_new()` and
setters, never a bare struct literal - a literal's zeros would wipe the inherit
sentinels.

```c
cheese_style_t s = cheese_style_new();
cheese_style_set_bg_color(&s, cheese_color_rgb(40, 40, 60));
cheese_style_set_corner_radius_uniform(&s, 8.0f);
```

Everything a *widget* owns lives in the property bag, keyed by an interned id.
A widget registers its own ids, so a third-party widget adds styling fields with
no core edit. Presence is the value, so `0` and transparent are real values:

```c
u32 thumb = cheese_prop_register(&cheese, "slider/thumb/color", CHEESE_PROP_COLOR);
cheese_style_set_prop_color(&s, thumb, cheese_color_rgb(200, 120, 40));
```

Resolution walks scopes lowest first: the inherited root scope, the widget's
typed role default, the lexical scope stack outer to inner, the widget's
`classes`, then its explicit style. Later wins by presence - there is no
selector engine, no specificity, no rule list. The app's code *is* the selector:
register a class and any widget that names it picks it up.

```c
cheese_style_t panel = cheese_style_new();
cheese_style_set_bg_color(&panel, cheese_color_rgb(30, 30, 40));
cheese_style_set_padding_uniform(&panel, 12.0f);
cheese_style_class_register(&cheese, "panel", panel);   // per frame, see below
```

Classes are per-frame: the class map and the role map are reset in
`cheese_begin`, so register them after it. The same holds for
`cheese_push_style` / `cheese_pop_style`, which push a subtree scope, and
`cheese_push_scope`, which pushes values and classes together. A widget with
`classes = null` resolves its built-in role class (`CHEESE_CLASS_*`), which is
how bare widgets get themed.

## Theming

`cheese_theme_t` is a semantic palette (surface, on_surface, outline, primary,
primary_container, error, and so on) plus metric tokens. Apply a preset once:

```c
cheese_theme_t t = cheese_theme_dark();
cheese_theme_apply(&cheese, &t);
```

A widget with `classes = null` resolves its built-in role class
(`CHEESE_CLASS_*`), so a bare widget is themed. Interactive states use M3 state
layers (hover 8%, focus 10%, pressed and disabled 12%) unless you set explicit
per-state colours. Focused widgets draw a focus ring. Presets and
`cheese_theme_palette()` live in `cheese/core/theme.h`.

## Focus and edit mode

Keyboard focus is a persistent semantics id on the context. Tab and the arrow
keys move it (`cheese_focus_move`, `cheese_focus_move_dir`). Arrow navigation is
on by default and can be turned off with `cheese_set_arrow_nav`.

Focus and editing are separate. A focused widget does not take the keyboard
until it is activated with a click or Enter/Space. Once activated,
`cheese->edit_id` owns the keys and Escape returns to navigation. This applies
to text fields, sliders, and scrollbars.

## Accessibility and tooling

Widgets emit a per-frame semantics tree as they draw. Inspect it in tests, or
draw `cheese_debug_overlay(&cheese, font)` after the UI for a labelled outline
(`cheese_debug_monitor` draws a resource panel). For a consumer on another
thread, such as an OS accessibility bridge, opt into the race-free snapshot:

```c
cheese_semantics_snapshot_enable(&cheese, snapshot_arena);
cheese_semantics_snapshot_begin(&cheese, &view);
// read view.nodes on another thread
cheese_semantics_snapshot_end(&cheese);
```

## Renderer plugins

The renderer is the only backend boundary. Implement this vtable
(`cheese_renderer_t` in `cheese/types.h`) and pass it to `cheese_begin`:

| field | purpose |
|---|---|
| `draw_rect` | rounded-rect fill (per-corner radius) |
| `draw_border` | rounded-rect ring (thickness and side mask) |
| `draw_line` / `draw_arc` | lines and arc bands or sectors |
| `draw_texture` | textured quad (tint) |
| `draw_text` | glyph quads; `sdf_text` selects the SDF path |
| `push_clip` / `pop_clip` | scissor stack |
| `create_texture` / `delete_texture` / `update_texture_region` | texture lifecycle |
| `flush_deferred` / `flush_draws` | per-frame batch flush |
| `sdf_text` | set when `draw_text` supports screen-space SDF |

A backend is a standalone file that fills that vtable; no core change is needed
to add one, and cheese ships with no backend of its own. The target set of ready
backends:

    GLES3, SDL2, SDL3, raylib, sokol, cairo, web, win32_gdi

A minimal SDL2 backend, for instance, maps `draw_rect` to
`SDL_RenderFillRect`, `draw_line` to `SDL_RenderLine`, and uploads the glyph
atlas with `SDL_CreateTexture`. Because cheese only holds the vtable, a consumer
can swap windowing or rendering without touching UI code.

## App harness

`cheese_app_t { userdata, start, update, render, stop }` plus
`cheese_app_start`, `cheese_app_update`, `cheese_app_render` and
`cheese_app_stop` is a thin convenience for structuring an app. It never
overrides your `main`, and skipping it for the raw API is equally valid.

## Memory model

Arena allocation throughout. Glyph bitmaps live in the font's persistent arena,
transient atlas pixels in `font->scratch` (cleared on each atlas rebuild), and
the context owns its state store. There are no per-widget heap allocations;
widget persistent state such as text caret, popup open flags and scroll offsets
is caller-owned. Reclaim with `arena_clear` and `arena_free`.

Measured release budget (butter Wayland gallery, 800x600, SDF on, MSAA and
validation off):

| figure             | value   | limit   |
|--------------------|---------|---------|
| Rss (resident)     | ~48 MiB | 100 MiB |
| Pss (proportional) | ~15 MiB | 100 MiB |
| CPU per frame      | ~0.3 ms | -       |

Most of the Rss is the AMD Vulkan driver (radv/LLVM) that `butter_init` dlopens
and shares with other processes, which is why Pss is the meaningful figure. The
demo prints a staged breakdown at startup (arena -> window -> butter -> font).

Check it on demand after building the release demo:

    conjure as butter-wayland-release test
    scripts/budget.sh

`scripts/budget.sh` runs the demo, samples `/proc/<pid>/smaps_rollup` once
settled, and exits non-zero if Rss or Pss exceeds `CHEESE_RSS_LIMIT_MIB` or
`CHEESE_PSS_LIMIT_MIB` (default 100 each).

## Gotchas

- Size the frame from the draw target, not a window that may be resizing; they
  disagree for a frame otherwise.
- Any pushed layout must be a copy of the current one, never
  `cheese_layout_default()` (it zeroes the cursor and collapses children).
- `cheese_draw_text` takes the glyph baseline, not the top. Place the first line
  at `top + ascender`.
- `da_append` on by-value structs needs a temp variable; a compound literal as a
  macro argument breaks parsing.


## License

This project is licensed under The BSD 3-Clause License - See [LICENSE](LICENSE) for details.
