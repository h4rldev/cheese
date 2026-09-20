/***********************************/

#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/overlay.h>
#include <cheese/core/semantics.h>
#include <cheese/core/style.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/patterns/toast.h>

/***********************************/

typedef struct {
  f32 x, y, w, h;
  f32 alpha;
  const cstr *text;
  const cstr *classes;
  cheese_font_t *font;
} cheese_toast_ctx_t;

//
//
//

static f32 cheese_toast_alpha(const cheese_toast_t *toast) {
  f32 fade = 0.25f;
  if (fade <= 0.0f || toast->remaining >= fade)
    return 1.0f;

  f32 alpha = toast->remaining / fade;
  return alpha < 0.0f ? 0.0f : alpha;
}

//
//
//

static cheese_color_t cheese_toast_with_alpha(cheese_color_t color, f32 alpha) {
  u32 a = (u32)((f32)((color >> 24) & 0xFF) * alpha);
  return (color & 0x00FFFFFFu) | (a << 24);
}

//
//
//

static void cheese_toast_overlay(cheese_t *cheese, void *userdata) {
  cheese_toast_ctx_t *ctx = userdata;

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_NONE, ctx->classes,
                              null);

  cheese_color_t bg = style.bg_color ? style.bg_color : 0x1E1E1EFF;
  cheese_draw_rect(cheese, style.corner_radius, ctx->x, ctx->y, ctx->w, ctx->h,
                   cheese_toast_with_alpha(bg, ctx->alpha));

  cheese_semantics_emit(
      cheese, (cheese_semantics_t){.name = ctx->text}, CHEESE_ROLE_LABEL,
      ctx->text, null, 0,
      (cheese_rect_t){(i32)ctx->x, (i32)ctx->y, (u32)ctx->w, (u32)ctx->h});

  if (!ctx->font || !ctx->text || ctx->text[0] == '\0')
    return;

  u32 size = style.font_size ? style.font_size : ctx->font->default_size;
  cheese_font_set_size(ctx->font, size);

  cheese_color_t text = style.text_color ? style.text_color : 0xFFFFFFFF;
  string *label = string_from_cstr(cheese->frame_arena, ctx->text);
  f32 text_w = cheese_font_measure_text(ctx->font, label);
  f32 text_h = ctx->font->active_variant->line_height;
  f32 tx = ctx->x + (ctx->w - text_w) * 0.5f;
  f32 ty = ctx->y + (ctx->h - text_h) * 0.5f + text_h * 0.75f;

  cheese_draw_text(cheese, tx, ty, label, ctx->font,
                   cheese_toast_with_alpha(text, ctx->alpha), 1.0f);
}

//
//
//

void cheese_toast_stack_init(cheese_toast_stack_t *stack, arena_t *arena,
                             u32 capacity) {
  if (!stack)
    return;

  stack->arena = arena;
  stack->capacity = capacity;
  stack->count = 0;

  if (capacity) {
    stack->items = arena_alloc_zeroed(arena, cheese_toast_t, capacity);
  } else {
    stack->items = null;
  }
}

void cheese_toast_push(cheese_toast_stack_t *stack, const cstr *text,
                       f32 duration) {
  if (!stack || !text || stack->capacity == 0 || !stack->items)
    return;

  if (stack->count >= stack->capacity) {
    for (u32 i = 1; i < stack->count; i++)
      stack->items[i - 1] = stack->items[i];
    stack->count--;
  }

  size_t len = strlen(text) + 1;
  cstr *copy = arena_alloc(stack->arena, char, len);
  memcpy(copy, text, len);

  cheese_toast_t toast = {
      .text = copy, .remaining = duration, .duration = duration};
  stack->items[stack->count++] = toast;
}

void cheese_toast_tick(cheese_toast_stack_t *stack, f32 delta_time) {
  if (!stack)
    return;

  u32 live = 0;
  for (u32 i = 0; i < stack->count; i++) {
    cheese_toast_t toast = stack->items[i];
    toast.remaining -= delta_time;
    if (toast.remaining > 0.0f)
      stack->items[live++] = toast;
  }
  stack->count = live;
}

void cheese_toast_draw(cheese_t *cheese, const cstr *classes,
                       cheese_toast_stack_t *stack, cheese_font_t *font) {
  if (!cheese || !cheese->renderer || !stack || stack->count == 0)
    return;

  f32 win_w = cheese->layout_stack[0].width;
  f32 margin = 16.0f;
  f32 gap = 8.0f;
  f32 pad_x = 14.0f;
  f32 h = 40.0f;

  f32 y = margin;
  for (u32 i = 0; i < stack->count; i++) {
    const cheese_toast_t *toast = &stack->items[i];

    f32 w = 140.0f;
    if (font && toast->text && toast->text[0] != '\0') {
      string *label = string_from_cstr(cheese->frame_arena, toast->text);
      f32 text_w = cheese_font_measure_text(font, label) + pad_x * 2.0f;
      if (text_w > w)
        w = text_w;
    }

    f32 x = win_w > w + margin ? win_w - margin - w : margin;

    cheese_toast_ctx_t *ctx =
        arena_alloc_zeroed(cheese->frame_arena, cheese_toast_ctx_t, 1);
    ctx->x = x;
    ctx->y = y;
    ctx->w = w;
    ctx->h = h;
    ctx->alpha = cheese_toast_alpha(toast);
    ctx->text = toast->text;
    ctx->classes = classes;
    ctx->font = font;

    cheese_overlay(cheese, x, y, w, h, cheese_toast_overlay, ctx);

    y += h + gap;
  }
}
