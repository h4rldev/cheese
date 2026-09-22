/***********************************/

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

#include <cheese/core/input.h>
#include <cheese/core/layout.h>
#include <cheese/core/semantics.h>

#include <cheese/style/prop.h>
#include <cheese/style/resolve.h>

#include <cheese/render/draw.h>
#include <cheese/render/font.h>

#include <cheese/widgets/image.h>

/***********************************/

void cheese_image(cheese_t *cheese, const cstr *classes,
                  cheese_semantics_t semantics, f32 x, f32 y, f32 w, f32 h,
                  cheese_texture_t texture, cheese_fit_t fit,
                  cheese_color_t tint) {
  if (!cheese || !cheese->renderer)
    return;

  cheese_style_t style;
  cheese_style_resolve_scoped(cheese, &style, CHEESE_ROLE_IMAGE, classes, null);

  cheese_semantics_emit(cheese, semantics, CHEESE_ROLE_IMAGE, semantics.name,
                        null, 0,
                        (cheese_rect_t){(i32)x, (i32)y, (u32)w, (u32)h});

  if (!texture.id || texture.width == 0 || texture.height == 0) {
    if (style.bg_color) {
      f32 alpha =
          cheese_style_prop_get_f32(&style, cheese->core_props.opacity, 1.0f);
      cheese_draw_bg(cheese, &style, x, y, w, h, style.bg_color, 0, alpha);
    }
    return;
  }

  f32 tw = (f32)texture.width;
  f32 th = (f32)texture.height;
  f32 dx = x, dy = y, dw = w, dh = h;
  b32 crop = false;

  if (fit == CHEESE_FIT_CONTAIN) {
    f32 scale = min(w / tw, h / th);
    dw = tw * scale;
    dh = th * scale;
    dx = x + (w - dw) * 0.5f;
    dy = y + (h - dh) * 0.5f;
  } else if (fit == CHEESE_FIT_COVER) {
    f32 scale = max(w / tw, h / th);
    dw = tw * scale;
    dh = th * scale;
    dx = x + (w - dw) * 0.5f;
    dy = y + (h - dh) * 0.5f;
    crop = true;
  }

  b32 clip = crop && cheese->renderer->push_clip != null;
  if (clip)
    cheese_push_clip(cheese, x, y, w, h);

  cheese_draw_texture(cheese, dx, dy, dw, dh, texture.id,
                      tint ? tint : 0xFFFFFFFF);

  if (clip)
    cheese_pop_clip(cheese);
}

void cheese_image_auto(cheese_t *cheese, const cstr *classes,
                       cheese_semantics_t semantics, cheese_texture_t texture,
                       cheese_fit_t fit, cheese_color_t tint) {
  cheese_layout_t *layout = cheese_current_layout(cheese);

  f32 w = layout->width > 0.0f
              ? layout->width
              : (texture.width > 0 ? (f32)texture.width : 100.0f);
  f32 h =
      texture.width > 0 ? w * (f32)texture.height / (f32)texture.width : 100.0f;

  f32 x, y;
  cheese_layout_place(cheese, &w, &h, &x, &y);

  cheese_image(cheese, classes, semantics, x, y, w, h, texture, fit, tint);
}
