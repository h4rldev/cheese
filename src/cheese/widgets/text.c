/***********************************/

#include <string.h>

#include <htils/basictypes.h>
#include <htils/darray.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/anim.h>
#include <cheese/core/init.h>
#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>
#include <cheese/core/state.h>
#include <cheese/core/style.h>
#include <cheese/core/utf8.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>
#include <cheese/render/text.h>

#include <cheese/widgets/text.h>
#include <cheese/widgets/widget.h>

/***********************************/

#define CHEESE_TEXT_INPUT_PAD_X 6.0f
#define CHEESE_TEXT_INPUT_PAD_Y 4.0f

#define CHEESE_TEXT_UNDO_COALESCE 0.5f

typedef struct {
  cheese_t *cheese;
  cheese_text_input_t *state;
  cheese_font_t *font;

  const cheese_style_t *style;

  cheese_value_t scroll;
  const cstr *cur;
  u32 len;

  cheese_text_line_t *lines;
  u32 line_count;

  u32 caret_b;
  u32 anchor_b;
  u32 caret_line;

  f32 x, y;
  f32 pad_left, pad_top;
  f32 inner_w, inner_h;
  f32 line_height;

  f32 scroll_x, scroll_y;

  b32 multiline;
  b32 wrap;
  b32 editing;

  cheese_color_t bg;
  cheese_color_t text_color;
  cheese_color_t sel_color;
} cheese_text_draw_t;

//
//
//

typedef struct {
  cheese_t *cheese;
  cheese_text_input_t *state;
  cheese_font_t *font;

  const cstr *cur;
  u32 len;

  b32 wrap;

  f32 x, y;
  f32 pad_left, pad_top;
  f32 inner_w;

  f32 line_height;
  f32 scroll_y;
} cheese_text_mouse_t;

//
//
//

static u32 text_prop_caret_color;
static u32 text_prop_caret_width;
static u32 text_prop_caret_style;
static u32 text_prop_caret_blink;

static void cheese_text_props(cheese_t *cheese) {
  if (text_prop_caret_color)
    return;

  text_prop_caret_color =
      cheese_prop_register(cheese, CHEESE_PROP_CARET_COLOR, CHEESE_PROP_COLOR);
  text_prop_caret_width =
      cheese_prop_register(cheese, CHEESE_PROP_CARET_WIDTH, CHEESE_PROP_F32);
  text_prop_caret_style =
      cheese_prop_register(cheese, CHEESE_PROP_CARET_STYLE, CHEESE_PROP_U32);
  text_prop_caret_blink =
      cheese_prop_register(cheese, CHEESE_PROP_CARET_BLINK, CHEESE_PROP_F32);
}

//
//
//

static b32 cheese_text_is_word(u8 c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_' || c >= 0x80;
}
//
//
//

static u64 cheese_text_word_prev(const cstr *s, u64 i) {
  while (i > 0 && !cheese_text_is_word(s[cheese_utf8_prev(s, i)]))
    i = cheese_utf8_prev(s, i);

  while (i > 0 && cheese_text_is_word(s[cheese_utf8_prev(s, i)]))
    i = cheese_utf8_prev(s, i);

  return i;
}

//
//
//

static u64 cheese_text_word_next(const cstr *s, u32 len, u64 i) {
  while (i < len && !cheese_text_is_word((u8)s[i]))
    i = cheese_utf8_next((cstr *)s, len, i);

  while (i < len && cheese_text_is_word((u8)s[i]))
    i = cheese_utf8_next((cstr *)s, len, i);

  return i;
}

//
//
//

static void cheese_text_selection(const cstr *buf, u32 len,
                                  const cheese_text_input_t *state, u32 *s,
                                  u32 *e) {
  u32 caret = cheese_utf8_offset(buf, len, (u32)max(0, state->caret));
  u32 anchor = cheese_utf8_offset(buf, len, (u32)max(0, state->anchor));
  *s = min(caret, anchor);
  *e = max(caret, anchor);
}

//
//
//

static b32 cheese_text_copy_range(cheese_t *cheese, const cstr *buf, u32 s,
                                  u32 e) {
  if (s == e || !cheese->clipboard_set)
    return false;

  char *sel = arena_alloc(cheese->frame_arena, char, e - s + 1);
  memcpy(sel, buf + s, e - s);
  sel[e - s] = '\0';
  cheese->clipboard_set(cheese->clipboard_userdata, sel);
  return true;
}

//
//
//

static void cheese_text_delete_range(cstr *buf, u32 *len, u32 s, u32 e) {
  memmove(buf + s, buf + e, *len - e);
  *len -= e - s;
  buf[*len] = '\0';
}

//
//
//

static b32 cheese_text_paste_text(cheese_t *cheese, cstr *buf, u32 cap,
                                  u32 *len, u32 caret, b32 multiline,
                                  u32 *out_caret) {
  if (!cheese->clipboard_get)
    return false;

  const cstr *text = cheese->clipboard_get(cheese->clipboard_userdata);
  if (!text || !*text)
    return false;

  u32 n = (u32)strlen(text);
  char *clean = arena_alloc(cheese->frame_arena, char, n + 1);
  u32 w = 0;
  for (u32 i = 0; i < n; i++) {
    char ch = text[i];
    if (!multiline && (ch == '\n' || ch == '\r'))
      continue;
    clean[w++] = ch;
  }
  clean[w] = '\0';

  u32 avail = cap > 0 ? cap - 1 : 0;
  if (avail > *len)
    avail -= *len;
  else
    avail = 0;
  if (w > avail)
    w = avail;
  while (w > 0 && ((u8)clean[w] & 0xC0) == 0x80)
    w--;

  if (w == 0)
    return false;

  memmove(buf + caret + w, buf + caret, *len - caret);
  memcpy(buf + caret, clean, w);
  *len += w;
  buf[*len] = '\0';
  *out_caret = caret + w;
  return true;
}

//
//
//

static void cheese_text_undo_push(cheese_text_input_t *state, const char *buf,
                                  u32 len, i32 caret, i32 anchor) {
  if (!state->undo_arena ||
      state->undo_bytes + len + 1 > CHEESE_TEXT_UNDO_BYTES)
    return;

  char *copy = arena_alloc(state->undo_arena, char, len + 1);
  memcpy(copy, buf, len);
  copy[len] = '\0';
  state->undo_bytes += len + 1;

  if (state->undo_count > 0 && state->undo_timer > 0.0f &&
      state->undo_pos > 0) {
    state->undo[state->undo_pos] =
        (cheese_text_undo_entry_t){copy, len, caret, anchor};
    state->undo_timer = CHEESE_TEXT_UNDO_COALESCE;
    return;
  }

  u32 next = state->undo_count == 0 ? 0 : state->undo_pos + 1;
  if (next >= CHEESE_TEXT_UNDO_MAX) {
    memmove(&state->undo[0], &state->undo[1],
            sizeof(cheese_text_undo_entry_t) * (CHEESE_TEXT_UNDO_MAX - 1));
    state->undo_count = CHEESE_TEXT_UNDO_MAX - 1;
    if (state->undo_pos > 0)
      state->undo_pos--;
    next = state->undo_count;
  }

  state->undo[next] = (cheese_text_undo_entry_t){copy, len, caret, anchor};
  state->undo_pos = next;
  state->undo_count = next + 1;
  state->undo_timer = CHEESE_TEXT_UNDO_COALESCE;
}

//
//
//

static void cheese_text_undo_seed(cheese_text_input_t *state, const char *buf,
                                  u32 len) {
  if (state->undo_arena && state->undo_count == 0)
    cheese_text_undo_push(state, buf, len, state->caret, state->anchor);
}

//
//
//

static void cheese_text_undo_apply(cheese_text_input_t *state, cstr *buf,
                                   u32 cap, u32 *len) {
  const cheese_text_undo_entry_t *entry = &state->undo[state->undo_pos];
  u32 n = min(entry->len, cap > 0 ? cap - 1 : 0);

  memcpy(buf, entry->text, n);
  buf[n] = '\0';
  *len = n;
  state->caret = entry->caret;
  state->anchor = entry->anchor;
}

//
//
//

static void cheese_text_mouse(cheese_text_mouse_t *mouse,
                              cheese_widget_t *scope, b32 hovered, b32 *focused,
                              b32 *editing) {
  cheese_t *cheese = mouse->cheese;
  cheese_text_input_t *state = mouse->state;

  if (!(cheese->mouse_buttons & CHEESE_MOUSE_LEFT) &&
      state->click_timer > 0.0f) {
    state->click_timer -= cheese->delta_time;
    if (state->click_timer <= 0.0f) {
      state->click_timer = 0.0f;
      state->click_count = 0;
    }
  }

  u32 clicked = cheese->mouse_buttons & ~cheese->mouse_prev_buttons;
  b32 press = hovered && (clicked & CHEESE_MOUSE_LEFT);
  b32 dragging = *editing && cheese_captured(cheese, scope->id);

  if (press) {
    f32 dx = cheese->mouse_x - state->click_x;
    f32 dy = cheese->mouse_y - state->click_y;

    if (state->click_timer > 0.0f && dx * dx + dy * dy <= 25.0f)
      state->click_count++;
    else
      state->click_count = 1;

    state->click_timer = 0.4f;
    state->click_x = cheese->mouse_x;
    state->click_y = cheese->mouse_y;

    cheese->focus_id = scope->id;
    *focused = true;

    scope->focused = true;
    scope->state |= CHEESE_STATE_FOCUSED;

    cheese->edit_id = scope->id;
    *editing = true;

    cheese_capture(cheese, scope->id);
  }

  if (!mouse->font || !(press || dragging))
    return;

  b32 extend = true;
  if (dragging && state->click_count >= 2) {
    f32 dx = cheese->mouse_x - state->click_x;
    f32 dy = cheese->mouse_y - state->click_y;
    extend = dx * dx + dy * dy > 25.0f;
  }

  u32 line_count = 0;
  cheese_text_line_t *lines =
      cheese_text_lines(cheese, mouse->font, mouse->cur, mouse->len,
                        mouse->wrap ? mouse->inner_w : 0.0f, &line_count);

  if (line_count == 0 || !extend)
    return;

  i32 li =
      (i32)((cheese->mouse_y - (mouse->y + mouse->pad_top) + mouse->scroll_y) /
            (mouse->line_height > 0.0f ? mouse->line_height : 1.0f));

  if (li < 0)
    li = 0;

  if ((u32)li >= line_count)
    li = (i32)line_count - 1;

  u32 caret = cheese_text_caret_from_x(
      mouse->font, mouse->cur, lines[li].start, lines[li].end,
      cheese->mouse_x - (mouse->x + mouse->pad_left) + state->scroll_x);

  state->caret = (i32)cheese_utf8_count(mouse->cur, caret);
  if (!press)
    return;

  state->anchor = state->caret;
  if (state->click_count == 2) {
    state->anchor = (i32)cheese_utf8_count(
        mouse->cur, cheese_text_word_prev(mouse->cur, caret));
    state->caret = (i32)cheese_utf8_count(
        mouse->cur, cheese_text_word_next(mouse->cur, mouse->len, caret));
  } else if (state->click_count >= 3) {
    state->anchor = (i32)cheese_utf8_count(mouse->cur, lines[li].start);
    state->caret = (i32)cheese_utf8_count(mouse->cur, lines[li].end);
  }
}

//
//
//

static void cheese_text_fit_scroll(cheese_text_draw_t *draw) {
  if (draw->state && !draw->wrap) {
    const cheese_text_line_t *line = &draw->lines[draw->caret_line];

    f32 caret_px = cheese_text_prefix_w(draw->font, draw->cur + line->start,
                                        draw->caret_b - line->start);
    f32 line_w = cheese_text_prefix_w(draw->font, draw->cur + line->start,
                                      line->end - line->start);

    if (caret_px - draw->state->scroll_x > draw->inner_w)
      draw->state->scroll_x = caret_px - draw->inner_w;

    if (caret_px - draw->state->scroll_x < 0.0f)
      draw->state->scroll_x = caret_px;

    if (draw->state->scroll_x < 0.0f)
      draw->state->scroll_x = 0.0f;

    if (line_w <= draw->inner_w)
      draw->state->scroll_x = 0.0f;

    draw->scroll_x = draw->state->scroll_x;
  }

  if (draw->state && draw->multiline) {
    f32 content_h = (f32)draw->line_count * draw->line_height;
    f32 max_scroll = max(0.0f, content_h - draw->inner_h);

    if (max_scroll > 0.0f && !draw->scroll.state)
      cheese_log_warn_once(
          "cheese_text_input: %.0fpx of content in a %.0fpx viewport with no "
          "bound scroll value; the field can not scroll",
          content_h, draw->inner_h);

    f32 caret_top = (f32)draw->caret_line * draw->line_height;
    if (caret_top < draw->scroll_y)
      draw->scroll_y = caret_top;

    if (caret_top + draw->line_height > draw->scroll_y + draw->inner_h)
      draw->scroll_y = caret_top + draw->line_height - draw->inner_h;

    draw->scroll_y = min(max_scroll, max(0.0f, draw->scroll_y));

    draw->state->content_h = content_h;
    draw->state->viewport_h = draw->inner_h;
  }
}

//
//
//

static void cheese_text_draw_selection(const cheese_text_draw_t *draw) {
  if (!draw->editing || !draw->state || draw->anchor_b == draw->caret_b)
    return;

  u32 s = min(draw->caret_b, draw->anchor_b);
  u32 e = max(draw->caret_b, draw->anchor_b);

  for (u32 i = 0; i < draw->line_count; i++) {
    u32 rs = max(s, draw->lines[i].start);
    u32 re = min(e, draw->lines[i].end);

    if (rs >= re)
      continue;

    f32 sx = draw->x + draw->pad_left +
             cheese_text_prefix_w(draw->font, draw->cur + draw->lines[i].start,
                                  rs - draw->lines[i].start) -
             draw->scroll_x;

    f32 ex = draw->x + draw->pad_left +
             cheese_text_prefix_w(draw->font, draw->cur + draw->lines[i].start,
                                  re - draw->lines[i].start) -
             draw->scroll_x;

    cheese_draw_rect(draw->cheese, (cheese_corners_t){0, 0, 0, 0}, sx,
                     draw->y + draw->pad_top + (f32)i * draw->line_height -
                         draw->scroll_y,
                     ex - sx, draw->line_height, draw->sel_color);
  }
}

//
//
//

static void cheese_text_draw_placeholder(const cheese_text_draw_t *draw) {
  if (draw->len != 0 || !draw->state || !draw->state->placeholder)
    return;

  string_slice hint = string_slice_from_cstr((u8 *)draw->state->placeholder,
                                             strlen(draw->state->placeholder));
  cheese_color_t hint_color =
      cheese_color_lerp(draw->text_color, draw->bg, 0.55f);

  cheese_draw_text(draw->cheese, draw->x + draw->pad_left - draw->scroll_x,
                   draw->y + draw->pad_top + draw->line_height * 0.75f -
                       draw->scroll_y,
                   &hint, draw->font, hint_color, 1.0f);
}

//
//
//

static void cheese_text_draw_glyphs(const cheese_text_draw_t *draw) {
  for (u32 i = 0; i < draw->line_count; i++) {
    if (draw->lines[i].end <= draw->lines[i].start)
      continue;

    string_slice line = {(u8 *)draw->cur + draw->lines[i].start,
                         draw->lines[i].end - draw->lines[i].start};
    f32 ty = draw->y + draw->pad_top + (f32)i * draw->line_height +
             draw->line_height * 0.75f - draw->scroll_y;

    if (draw->state && draw->state->masked) {
      u32 n =
          (u32)cheese_utf8_count(draw->cur + draw->lines[i].start, line.len);
      cstr *hidden = arena_alloc_zeroed(draw->cheese->frame_arena, cstr, n + 1);
      for (u32 c = 0; c < n; c++)
        hidden[c] = '*';
      hidden[n] = '\0';

      line = (string_slice){(u8 *)hidden, n};
    }

    cheese_draw_text(draw->cheese, draw->x + draw->pad_left - draw->scroll_x,
                     ty, &line, draw->font, draw->text_color, 1.0f);
  }
}

//
//
//

static void cheese_text_draw_caret(const cheese_text_draw_t *draw) {
  if (!draw->editing || !draw->state || draw->state->readonly)
    return;

  cheese_text_props(draw->cheese);

  cheese_color_t caret_color = cheese_style_get_prop_color(
      draw->style, text_prop_caret_color, draw->text_color);

  f32 caret_w =
      cheese_style_get_prop_f32(draw->style, text_prop_caret_width, 1.5f);
  u32 caret_shape = cheese_style_get_prop_u32(
      draw->style, text_prop_caret_style, CHEESE_TEXT_CARET_BAR);
  f32 blink =
      cheese_style_get_prop_f32(draw->style, text_prop_caret_blink, 0.0f);

  b32 caret_on = true;
  if (blink > 0.0f) {
    draw->state->caret_time += draw->cheese->delta_time;
    if (draw->state->caret_time >= blink)
      draw->state->caret_time -= blink;

    caret_on = draw->state->caret_time < blink * 0.5f;
    cheese_request_frame(draw->cheese);
  }

  if (!caret_on)
    return;

  f32 caret_px = cheese_text_prefix_w(
      draw->font, draw->cur + draw->lines[draw->caret_line].start,
      draw->caret_b - draw->lines[draw->caret_line].start);
  f32 cx = draw->x + draw->pad_left + caret_px - draw->scroll_x;
  f32 cy = draw->y + draw->pad_top + (f32)draw->caret_line * draw->line_height -
           draw->scroll_y;

  f32 cell_w = cheese_text_prefix_w(
      draw->font, draw->cur + draw->caret_b,
      cheese_utf8_next(draw->cur, draw->len, draw->caret_b) - draw->caret_b);

  if (cell_w <= 0.0f)
    cell_w = draw->line_height * 0.5f;

  if (caret_shape == CHEESE_TEXT_CARET_BLOCK)
    cheese_draw_rect(draw->cheese, (cheese_corners_t){0, 0, 0, 0}, cx, cy,
                     cell_w, draw->line_height, caret_color);

  else if (caret_shape == CHEESE_TEXT_CARET_UNDERLINE)
    cheese_draw_rect(draw->cheese, (cheese_corners_t){0, 0, 0, 0}, cx,
                     cy + draw->line_height - caret_w, cell_w, caret_w,
                     caret_color);

  else
    cheese_draw_rect(draw->cheese, (cheese_corners_t){0, 0, 0, 0}, cx, cy,
                     caret_w, draw->line_height, caret_color);
}

//
//
//

b32 cheese_text_edit_copy(cheese_t *cheese, const cheese_text_input_t *state,
                          const cstr *buf) {
  if (!cheese || !state || !buf)
    return false;

  u32 len = (u32)strlen(buf);
  u32 s, e;
  cheese_text_selection(buf, len, state, &s, &e);
  return cheese_text_copy_range(cheese, buf, s, e);
}

b32 cheese_text_edit_cut(cheese_t *cheese, cheese_text_input_t *state,
                         cstr *buf, u32 cap, u32 *len) {
  (void)cap;
  if (!cheese || !state || !buf || !len)
    return false;

  u32 s, e;
  cheese_text_selection(buf, *len, state, &s, &e);

  if (!cheese_text_copy_range(cheese, buf, s, e))
    return false;

  cheese_text_undo_seed(state, buf, *len);
  cheese_text_delete_range(buf, len, s, e);
  state->caret = state->anchor = (i32)cheese_utf8_count(buf, s);
  cheese_text_undo_push(state, buf, *len, state->caret, state->anchor);
  return true;
}

b32 cheese_text_edit_paste(cheese_t *cheese, cheese_text_input_t *state,
                           cstr *buf, u32 cap, u32 *len, b32 multiline) {
  if (!cheese || !state || !buf || !len)
    return false;

  u32 s, e;
  cheese_text_selection(buf, *len, state, &s, &e);
  cheese_text_undo_seed(state, buf, *len);

  b32 removed = false;
  if (s != e) {
    cheese_text_delete_range(buf, len, s, e);
    removed = true;
  }

  u32 out = 0;
  if (cheese_text_paste_text(cheese, buf, cap, len, s, multiline, &out)) {
    state->caret = state->anchor = (i32)cheese_utf8_count(buf, out);
    cheese_text_undo_push(state, buf, *len, state->caret, state->anchor);
    return true;
  }

  if (removed) {
    state->caret = state->anchor = (i32)cheese_utf8_count(buf, s);
    cheese_text_undo_push(state, buf, *len, state->caret, state->anchor);
    return true;
  }

  return false;
}

b32 cheese_text_edit(cheese_t *cheese, cheese_text_input_t *state, cstr *buf,
                     u32 cap, u32 *len, b32 multiline, cheese_font_t *font,
                     f32 wrap_w) {
  if (!cheese || !state || !buf || !len)
    return false;

  u32 blen = *len;
  u32 caret_b = cheese_utf8_offset(buf, blen, (u32)max(0, state->caret));
  u32 anchor_b = cheese_utf8_offset(buf, blen, (u32)max(0, state->anchor));
  b32 changed = false;
  b32 restored = false;
  b32 readonly = state->readonly;

  if (state->undo_timer > 0.0f)
    state->undo_timer -= cheese->delta_time;

  cheese_text_undo_seed(state, buf, blen);

  for (u32 i = 0; i < cheese->key_event_count; i++) {
    const cheese_key_event_t *ev = &cheese->key_events[i];
    u32 mods = ev->mods;
    b32 extend = (mods & CHEESE_MOD_SHIFT) != 0;

    if (ev->key == CHEESE_KEY_LEFT) {
      caret_b = (mods & CHEESE_MOD_CTRL) ? cheese_text_word_prev(buf, caret_b)
                                         : cheese_utf8_prev(buf, caret_b);
      if (!extend)
        anchor_b = caret_b;
      continue;
    }
    if (ev->key == CHEESE_KEY_RIGHT) {
      caret_b = (mods & CHEESE_MOD_CTRL)
                    ? cheese_text_word_next(buf, blen, caret_b)
                    : cheese_utf8_next(buf, blen, caret_b);
      if (!extend)
        anchor_b = caret_b;
      continue;
    }
    if (ev->key == CHEESE_KEY_HOME || ev->key == CHEESE_KEY_END ||
        (multiline &&
         (ev->key == CHEESE_KEY_UP || ev->key == CHEESE_KEY_DOWN))) {
      u32 line_count = 0;
      cheese_text_line_t *lines =
          cheese_text_lines(cheese, font, buf, blen, wrap_w, &line_count);
      u32 li = cheese_text_line_of(lines, line_count, caret_b);
      u32 col =
          cheese_utf8_count(buf + lines[li].start, caret_b - lines[li].start);

      if (ev->key == CHEESE_KEY_HOME) {
        caret_b = lines[li].start;
      } else if (ev->key == CHEESE_KEY_END) {
        caret_b = lines[li].end;
      } else if (ev->key == CHEESE_KEY_UP && li > 0) {
        u32 off =
            cheese_utf8_offset(buf + lines[li - 1].start,
                               lines[li - 1].end - lines[li - 1].start, col);
        caret_b = lines[li - 1].start + off;
      } else if (ev->key == CHEESE_KEY_DOWN && li + 1 < line_count) {
        u32 off =
            cheese_utf8_offset(buf + lines[li + 1].start,
                               lines[li + 1].end - lines[li + 1].start, col);
        caret_b = lines[li + 1].start + off;
      }
      if (!extend)
        anchor_b = caret_b;
      continue;
    }

    if (!readonly &&
        (ev->key == CHEESE_KEY_BACKSPACE || ev->key == CHEESE_KEY_DELETE)) {
      if (caret_b != anchor_b) {
        u32 s = min(caret_b, anchor_b);
        u32 e = max(caret_b, anchor_b);
        memmove(buf + s, buf + e, blen - e);
        blen -= e - s;
        caret_b = anchor_b = s;
        changed = true;
      } else if (ev->key == CHEESE_KEY_BACKSPACE && caret_b > 0) {
        u32 prev = (mods & CHEESE_MOD_CTRL)
                       ? cheese_text_word_prev(buf, caret_b)
                       : cheese_utf8_prev(buf, caret_b);
        memmove(buf + prev, buf + caret_b, blen - caret_b);
        blen -= caret_b - prev;
        caret_b = anchor_b = prev;
        changed = true;
      } else if (ev->key == CHEESE_KEY_DELETE && caret_b < blen) {
        u32 next = (mods & CHEESE_MOD_CTRL)
                       ? cheese_text_word_next(buf, blen, caret_b)
                       : cheese_utf8_next(buf, blen, caret_b);
        memmove(buf + caret_b, buf + next, blen - next);
        blen -= next - caret_b;
        changed = true;
      }
      continue;
    }

    if (mods & CHEESE_MOD_CTRL) {
      u32 ccp = ev->codepoint;
      b32 is_copy = ev->key == CHEESE_KEY_C || ccp == 'c' || ccp == 'C';
      b32 is_cut =
          !readonly && (ev->key == CHEESE_KEY_X || ccp == 'x' || ccp == 'X');
      b32 is_paste =
          !readonly && (ev->key == CHEESE_KEY_V || ccp == 'v' || ccp == 'V');
      b32 is_all = ev->key == CHEESE_KEY_A || ccp == 'a' || ccp == 'A';
      b32 is_z = ev->key == CHEESE_KEY_Z || ccp == 'z' || ccp == 'Z';
      b32 is_undo = !readonly && is_z && !(mods & CHEESE_MOD_SHIFT);
      b32 is_redo =
          !readonly && (ev->key == CHEESE_KEY_Y || ccp == 'y' || ccp == 'Y' ||
                        (is_z && (mods & CHEESE_MOD_SHIFT)));

      if (is_undo || is_redo) {
        i32 next = (i32)state->undo_pos + (is_redo ? 1 : -1);

        if (state->undo_arena && next >= 0 && next < (i32)state->undo_count) {
          state->undo_pos = (u32)next;
          cheese_text_undo_apply(state, buf, cap, &blen);
          caret_b = cheese_utf8_offset(buf, blen, (u32)max(0, state->caret));
          anchor_b = cheese_utf8_offset(buf, blen, (u32)max(0, state->anchor));
          changed = true;
          restored = true;
        }
      } else if (is_all) {
        anchor_b = 0;
        caret_b = blen;
      } else if ((is_copy || is_cut) && caret_b != anchor_b) {
        u32 s = min(caret_b, anchor_b);
        u32 e = max(caret_b, anchor_b);
        cheese_text_copy_range(cheese, buf, s, e);
        if (is_cut) {
          cheese_text_delete_range(buf, &blen, s, e);
          caret_b = anchor_b = s;
          changed = true;
        }
      } else if (is_paste) {
        if (caret_b != anchor_b) {
          u32 s = min(caret_b, anchor_b);
          u32 e = max(caret_b, anchor_b);
          cheese_text_delete_range(buf, &blen, s, e);
          caret_b = anchor_b = s;
          changed = true;
        }
        u32 out = 0;
        if (cheese_text_paste_text(cheese, buf, cap, &blen, caret_b, multiline,
                                   &out)) {
          caret_b = anchor_b = out;
          changed = true;
        }
      }
      continue;
    }

    u32 cp = ev->codepoint;
    if (ev->key == CHEESE_KEY_ENTER)
      cp = '\n';

    if (cp == '\n' && !multiline)
      continue;

    b32 printable =
        cp == '\n' ||
        (cp >= 32 && cp != 127 &&
         !(mods & (CHEESE_MOD_CTRL | CHEESE_MOD_ALT | CHEESE_MOD_SUPER)));
    if (!printable || readonly)
      continue;

    if (caret_b != anchor_b) {
      u32 s = min(caret_b, anchor_b);
      u32 e = max(caret_b, anchor_b);
      memmove(buf + s, buf + e, blen - e);
      blen -= e - s;
      caret_b = anchor_b = s;
    }

    char enc[4];
    u32 n = cheese_utf8_encode(cp, enc);
    if (blen + n < cap) {
      memmove(buf + caret_b + n, buf + caret_b, blen - caret_b);
      memcpy(buf + caret_b, enc, n);
      blen += n;
      caret_b += n;
      anchor_b = caret_b;
      changed = true;
    }
  }

  buf[blen] = '\0';
  *len = blen;
  state->caret = (i32)cheese_utf8_count(buf, caret_b);
  state->anchor = (i32)cheese_utf8_count(buf, anchor_b);

  if (changed && !restored)
    cheese_text_undo_push(state, buf, blen, state->caret, state->anchor);

  return changed;
}

b32 cheese_text_input(cheese_t *cheese, const cstr *classes,
                      cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                      cheese_value_t text, cheese_text_input_t *state,
                      cheese_value_t scroll,
                      cheese_text_input_variant_t variant,
                      cheese_font_t *font) {
  if (!cheese || !cheese->renderer)
    return false;

  b32 multiline = variant != CHEESE_TEXT_INPUT_LINE;
  b32 wrap = variant == CHEESE_TEXT_INPUT_WRAP;

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_TEXT_INPUT, classes,
                              null);

  cheese_widget_t scope;
  cheese_widget_begin(cheese, semantics, CHEESE_ROLE_TEXT_INPUT, x, y, w, h,
                      &scope);

  b32 hovered = scope.hovered;
  b32 focused = scope.focused;
  b32 editing = state && cheese->edit_id && cheese->edit_id == scope.id;

  const cstr *cur = cheese_value_str(text);
  u32 len = (u32)strlen(cur);

  if (scope.hovered)
    cheese_cursor_request(cheese, style.cursor >= 0
                                      ? (cheese_cursor_t)style.cursor
                                      : CHEESE_CURSOR_TEXT);

  f32 pad_left = cheese_style_get_pad_left(&style);
  f32 pad_right = cheese_style_get_pad_right(&style);
  f32 pad_top = cheese_style_get_pad_top(&style);
  f32 pad_bottom = cheese_style_get_pad_bottom(&style);
  if (pad_left < 0.0f)
    pad_left = CHEESE_TEXT_INPUT_PAD_X;
  if (pad_right < 0.0f)
    pad_right = CHEESE_TEXT_INPUT_PAD_X;
  if (pad_top < 0.0f)
    pad_top = CHEESE_TEXT_INPUT_PAD_Y;
  if (pad_bottom < 0.0f)
    pad_bottom = CHEESE_TEXT_INPUT_PAD_Y;

  f32 inner_w = w - pad_left - pad_right;
  if (inner_w < 0.0f)
    inner_w = 0.0f;

  f32 inner_h = h - pad_top - pad_bottom;
  if (inner_h < 0.0f)
    inner_h = 0.0f;

  f32 line_height = 16.0f;
  if (font) {
    u32 target_size = style.font_size ? style.font_size : font->default_size;
    cheese_font_set_size(font, target_size);
    line_height = font->active_variant->line_height;
  }

  if (focused)
    cheese_draw_focus_ring(cheese, &style, x, y, w, h);

  f32 scroll_y = cheese_value_f32(scroll);
  if (scroll_y < 0.0f)
    scroll_y = 0.0f;
  f32 scroll_in = scroll_y;
  if (multiline && hovered && scroll.state && cheese->scroll_y != 0.0f)
    scroll_y -= cheese->scroll_y * line_height * 3.0f;

  if (state) {
    cheese_text_mouse_t mouse = {
        .cheese = cheese,
        .state = state,
        .font = font,
        .cur = cur,
        .len = len,
        .wrap = wrap,
        .x = x,
        .y = y,
        .pad_left = pad_left,
        .pad_top = pad_top,
        .inner_w = inner_w,
        .line_height = line_height,
        .scroll_y = scroll_y,
    };

    cheese_text_mouse(&mouse, &scope, hovered, &focused, &editing);
  }

  b32 activated = false;
  if (focused && !editing && state && cheese_widget_activated(cheese, &scope)) {
    cheese->edit_id = scope.id;
    editing = true;
    activated = true;
  }

  if (editing && cheese_key_pressed(cheese, CHEESE_KEY_ESCAPE)) {
    cheese->edit_id = 0;
    editing = false;
  }

  b32 changed = false;
  if (editing && state && !activated) {
    u32 cap = len + CHEESE_MAX_KEY_EVENTS * 4 + 1;
    for (u32 i = 0; i < state->undo_count; i++)
      cap = max(cap, state->undo[i].len + CHEESE_MAX_KEY_EVENTS * 4 + 1);
    char *buf = arena_alloc(cheese->frame_arena, char, cap);
    memcpy(buf, cur, len);

    u32 blen = len;
    changed = cheese_text_edit(cheese, state, buf, cap, &blen, multiline, font,
                               wrap ? inner_w : 0.0f);
    if (changed) {
      cheese_value_set_str(text, buf);
      cur = buf;
      len = blen;
    }
  }

  cheese_color_t bg = style.bg_color ? style.bg_color : 0xFFFFFFFF;
  cheese_color_t text_color = style.text_color ? style.text_color : 0x000000FF;
  cheese_color_t sel_color = style.focus_color ? style.focus_color : 0x3B82F680;

  f32 alpha =
      cheese_style_get_prop_f32(&style, cheese->core_props.opacity, 1.0f);
  cheese_draw_bg(cheese, &style, x, y, w, h, bg, 0, alpha);
  cheese_draw_border(cheese, &style, x, y, w, h);

  cheese_widget_emit(cheese, semantics, CHEESE_ROLE_TEXT_INPUT, null, cur, 0,
                     &scope);

  f32 scroll_x = state ? state->scroll_x : 0.0f;

  if (font) {
    u32 line_count = 0;
    cheese_text_line_t *lines = cheese_text_lines(
        cheese, font, cur, len, wrap ? inner_w : 0.0f, &line_count);

    u32 caret_b = 0;
    u32 anchor_b = 0;
    if (state) {
      caret_b = cheese_utf8_offset(cur, len, (u32)max(0, state->caret));
      anchor_b = cheese_utf8_offset(cur, len, (u32)max(0, state->anchor));
    }

    cheese_text_draw_t draw = {
        .cheese = cheese,
        .state = state,
        .font = font,
        .style = &style,
        .scroll = scroll,
        .cur = cur,
        .len = len,
        .lines = lines,
        .line_count = line_count,
        .caret_b = caret_b,
        .anchor_b = anchor_b,
        .caret_line = cheese_text_line_of(lines, line_count, caret_b),
        .x = x,
        .y = y,
        .pad_left = pad_left,
        .pad_top = pad_top,
        .inner_w = inner_w,
        .inner_h = inner_h,
        .line_height = line_height,
        .scroll_x = scroll_x,
        .scroll_y = scroll_y,
        .multiline = multiline,
        .wrap = wrap,
        .editing = editing,
        .bg = bg,
        .text_color = text_color,
        .sel_color = sel_color,
    };

    cheese_text_fit_scroll(&draw);
    scroll_x = draw.scroll_x;
    scroll_y = draw.scroll_y;

    b32 clip = cheese->renderer->push_clip != null;
    if (clip)
      cheese_push_clip(cheese, x + pad_left, y + pad_top, inner_w, inner_h);

    cheese_text_draw_selection(&draw);
    cheese_text_draw_placeholder(&draw);
    cheese_text_draw_glyphs(&draw);
    cheese_text_draw_caret(&draw);

    if (clip)
      cheese_pop_clip(cheese);
  }

  if (multiline && scroll_y != scroll_in)
    cheese_value_set_f32(scroll, scroll_y);

  return changed;
}

b32 cheese_text_input_auto(cheese_t *cheese, const cstr *classes,
                           cheese_semantics_t semantics, cheese_value_t text,
                           cheese_text_input_t *state, cheese_value_t scroll,
                           cheese_text_input_variant_t variant,
                           cheese_font_t *font) {

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_TEXT_INPUT, classes,
                              null);

  f32 line_height = 16.0f;
  if (font) {
    u32 target_size = style.font_size ? style.font_size : font->default_size;
    cheese_font_set_size(font, target_size);
    line_height = font->active_variant->line_height;
  }

  f32 pad_y = cheese_style_get_pad_top(&style) >= 0.0f
                  ? cheese_style_get_pad_top(&style)
                  : CHEESE_TEXT_INPUT_PAD_Y;

  cheese_layout_t *layout = cheese_current_layout(cheese);
  f32 w = layout->width > 0.0f ? layout->width : 200.0f;
  f32 h = line_height * (variant != CHEESE_TEXT_INPUT_LINE ? 4.0f : 1.0f) +
          pad_y * 2.0f;

  f32 x, y;
  cheese_layout_place(cheese, &w, &h, &x, &y);

  return cheese_text_input(cheese, classes, semantics, x, y, w, h, text, state,
                           scroll, variant, font);
}
