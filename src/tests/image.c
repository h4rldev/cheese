/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>

#include <cheese/render/draw.h>

#include <cheese/widgets/image.h>

/***********************************/

static u32 g_last_id;
static f32 g_dx, g_dy, g_dw, g_dh;
static u32 g_draws;
static u32 g_clips;
static u32 g_destroyed;

static void stub(void *userdata) { (void)userdata; }

//
//
//

static b32 approx(f32 a, f32 b) {
  f32 d = a - b;
  return (d < 0.0f ? -d : d) < 0.01f;
}

//
//
//

static void stub_rect(void *userdata, cheese_corners_t radius, f32 x, f32 y,
                      f32 w, f32 h, cheese_color_t color) {
  (void)userdata;
  (void)radius;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)color;
}

//
//
//

static void stub_texture(void *userdata, f32 x, f32 y, f32 w, f32 h,
                         u32 texture_id, cheese_color_t color) {
  (void)userdata;
  (void)color;
  g_last_id = texture_id;
  g_dx = x;
  g_dy = y;
  g_dw = w;
  g_dh = h;
  g_draws++;
}

//
//
//

static i32 stub_create(void *userdata, u32 width, u32 height,
                       cheese_texture_format_t format, const u8 *data) {
  (void)userdata;
  (void)width;
  (void)height;
  (void)format;
  (void)data;
  return 7;
}

//
//
//

static void stub_destroy(void *userdata, u32 texture_id) {
  (void)userdata;
  g_destroyed = texture_id;
}

//
//
//

static void stub_push_clip(void *userdata, f32 x, f32 y, f32 w, f32 h) {
  (void)userdata;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  g_clips++;
}

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
    .draw_rect = stub_rect,
    .draw_texture = stub_texture,
    .push_clip = stub_push_clip,
    .pop_clip = stub,
    .create_texture = stub_create,
    .delete_texture = stub_destroy,
};

static void begin(cheese_t *cheese, arena_t *arena) {
  cheese_begin(cheese, arena, null, &g_renderer, (cheese_input_t){0}, 0.016f);
}

//
//
//

int main(void) {
  arena_t *arena = arena_new(MiB(16), MiB(1));
  cheese_t cheese = cheese_default(arena);

  u8 pixels[4] = {0};

  begin(&cheese, arena);
  cheese_texture_t texture = cheese_texture_upload(&cheese, 100, 50, pixels);
  assert(texture.id == 7 && texture.width == 100 && texture.height == 50 &&
         "upload wraps create_texture");

  cheese_image(&cheese, null, (cheese_semantics_t){.key = "img"}, 0.0f, 0.0f,
               100.0f, 80.0f, texture, CHEESE_FIT_STRETCH, 0);
  assert(g_draws == 1 && g_last_id == 7 && "stretch draws the texture");
  assert(approx(g_dx, 0.0f) && approx(g_dy, 0.0f) && approx(g_dw, 100.0f) &&
         approx(g_dh, 80.0f) && "stretch fills the box");
  cheese_end(&cheese);

  begin(&cheese, arena);
  g_draws = 0;
  g_clips = 0;
  cheese_image(&cheese, null, (cheese_semantics_t){.key = "img"}, 0.0f, 0.0f,
               100.0f, 100.0f, texture, CHEESE_FIT_CONTAIN, 0);
  assert(approx(g_dw, 100.0f) && approx(g_dh, 50.0f) && approx(g_dx, 0.0f) &&
         approx(g_dy, 25.0f) && "contain letterboxes");
  assert(g_clips == 0 && "contain needs no clip");
  cheese_end(&cheese);

  begin(&cheese, arena);
  g_draws = 0;
  g_clips = 0;
  cheese_image(&cheese, null, (cheese_semantics_t){.key = "img"}, 0.0f, 0.0f,
               50.0f, 100.0f, texture, CHEESE_FIT_COVER, 0);
  assert(approx(g_dw, 200.0f) && approx(g_dh, 100.0f) && approx(g_dx, -75.0f) &&
         approx(g_dy, 0.0f) && "cover scales to fill");
  assert(g_clips == 1 && "cover clips to the box");
  cheese_end(&cheese);

  begin(&cheese, arena);
  g_draws = 0;
  cheese_image(&cheese, null, (cheese_semantics_t){.key = "img"}, 0.0f, 0.0f,
               50.0f, 50.0f, (cheese_texture_t){0}, CHEESE_FIT_STRETCH, 0);
  assert(g_draws == 0 && "no texture draws no quad");
  cheese_texture_destroy(&cheese, texture);
  cheese_end(&cheese);
  assert(g_destroyed == 7 && "destroy wraps delete_texture");

  arena_free(arena);
  return 0;
}
