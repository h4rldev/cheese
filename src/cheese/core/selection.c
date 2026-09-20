/***********************************/

#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

#include <cheese/types.h>

#include <cheese/core/selection.h>
#include <cheese/core/style.h>
#include <cheese/core/utf8.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>
#include <cheese/render/text.h>

/***********************************/

static u64 cheese_selection_hash(const string *text) {
  u64 hash = 14695981039346656037ull;

  for (u64 i = 0; i < text->len; i++) {
    hash ^= text->base[i];
    hash *= 1099511628211ull;
  }

  return hash;
}

//
//
//

static b32 cheese_selection_copy_pressed(const cheese_t *cheese) {
  for (u32 i = 0; i < cheese->key_event_count; i++) {
    const cheese_key_event_t *e = &cheese->key_events[i];
    if (e->key == CHEESE_KEY_C || e->codepoint == 'c' || e->codepoint == 'C')
      return true;
  }

  return false;
}

//
//
//

void cheese_selection_record(cheese_t *cheese, const string *text,
                             cheese_font_t *font, f32 x, f32 baseline) {
  if (!cheese || !text || !font || text->len == 0)
    return;

  if (cheese->text_run_count == cheese->text_run_cap) {
    u32 cap = cheese->text_run_cap ? cheese->text_run_cap * 2 : 16;
    cheese_text_run_t *runs =
        arena_alloc_zeroed(cheese->frame_arena, cheese_text_run_t, cap);

    if (cheese->text_runs && cheese->text_run_count)
      memcpy(runs, cheese->text_runs,
             sizeof(cheese_text_run_t) * cheese->text_run_count);

    cheese->text_runs = runs;
    cheese->text_run_cap = cap;
  }

  f32 ascender = (f32)font->base_ascender * font->scale;
  f32 descender = (f32)font->base_descender * font->scale;

  u64 hash = cheese_selection_hash(text);
  u32 index = cheese->text_run_count;

  cheese->text_runs[index] = (cheese_text_run_t){
      .text = text,
      .font = font,
      .x = x,
      .baseline = baseline,
      .top = baseline - ascender,
      .bottom = baseline + descender,
      .width = cheese_font_measure_text(font, text),
      .hash = hash,
  };

  cheese->text_run_count++;

  cheese_selection_t *selection = &cheese->selection;
  if (!selection->active || selection->run != index || selection->hash != hash)
    return;

  u32 start = min(selection->anchor, selection->focus);
  u32 end = max(selection->anchor, selection->focus);

  if (start == end)
    return;

  cheese_style_t *style = cheese_current_style(cheese);
  cheese_color_t color =
      style && style->focus_color ? style->focus_color : 0x3B82F680;
  f32 sx = x + cheese_text_prefix_w(font, (const cstr *)text->base, start);
  f32 ex = x + cheese_text_prefix_w(font, (const cstr *)text->base, end);

  cheese_draw_rect(cheese, (cheese_corners_t){0, 0, 0, 0}, sx,
                   baseline - ascender, ex - sx, ascender + descender, color);
}

void cheese_selection_update(cheese_t *cheese) {
  if (!cheese)
    return;

  u32 pressed = cheese->mouse_buttons & ~cheese->mouse_prev_buttons;
  b32 press = (pressed & CHEESE_MOUSE_LEFT) && cheese->active_id == 0;
  b32 dragging = cheese->active_id == CHEESE_SELECTION_ID;

  if (press) {
    const cheese_text_run_t *hit = null;
    u32 hit_index = 0;

    for (u32 i = 0; i < cheese->text_run_count; i++) {
      const cheese_text_run_t *run = &cheese->text_runs[i];

      if (cheese->mouse_x >= run->x && cheese->mouse_x <= run->x + run->width &&
          cheese->mouse_y >= run->top && cheese->mouse_y <= run->bottom) {
        hit = run;
        hit_index = i;
        break;
      }
    }

    if (hit) {
      u32 at =
          cheese_text_caret_from_x(hit->font, (const cstr *)hit->text->base, 0,
                                   hit->text->len, cheese->mouse_x - hit->x);

      cheese->selection.active = true;
      cheese->selection.run = hit_index;
      cheese->selection.anchor = at;
      cheese->selection.focus = at;
      cheese->selection.hash = hit->hash;
      cheese->active_id = CHEESE_SELECTION_ID;
      cheese->press_x = cheese->mouse_x;
      cheese->press_y = cheese->mouse_y;
    } else
      cheese_selection_clear(cheese);
  } else if (dragging && cheese->selection.run < cheese->text_run_count) {
    const cheese_text_run_t *run = &cheese->text_runs[cheese->selection.run];
    if (run->hash == cheese->selection.hash)
      cheese->selection.focus =
          cheese_text_caret_from_x(run->font, (const cstr *)run->text->base, 0,
                                   run->text->len, cheese->mouse_x - run->x);
  }

  if (cheese->selection.active && cheese->edit_id == 0 &&
      (cheese->key_mods & CHEESE_MOD_CTRL) &&
      cheese_selection_copy_pressed(cheese))
    cheese_selection_copy(cheese);
}

void cheese_selection_clear(cheese_t *cheese) {
  if (!cheese)
    return;

  cheese->selection.active = false;
  cheese->selection.anchor = 0;
  cheese->selection.focus = 0;

  if (cheese->active_id == CHEESE_SELECTION_ID)
    cheese->active_id = 0;
}

b32 cheese_selection_active(const cheese_t *cheese) {
  return cheese && cheese->selection.active &&
         cheese->selection.anchor != cheese->selection.focus;
}

b32 cheese_selection_copy(cheese_t *cheese) {
  if (!cheese || !cheese->selection.active || !cheese->clipboard_set)
    return false;

  if (cheese->selection.run >= cheese->text_run_count)
    return false;

  const cheese_text_run_t *run = &cheese->text_runs[cheese->selection.run];
  if (run->hash != cheese->selection.hash)
    return false;

  u32 start = min(cheese->selection.anchor, cheese->selection.focus);
  u32 end = max(cheese->selection.anchor, cheese->selection.focus);
  if (start == end)
    return false;

  u8 *text = arena_alloc_zeroed(cheese->frame_arena, u8, end - start + 1);
  memcpy(text, run->text->base + start, end - start);
  cheese->clipboard_set(cheese->clipboard_userdata, (const cstr *)text);
  return true;
}
