/***********************************/

#include <assert.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/init.h>
#include <cheese/core/style.h>

/***********************************/

static void stub(void *userdata) { (void)userdata; }

//
//
//

static cheese_renderer_t g_renderer = {
    .flush_deferred = stub,
    .flush_draws = stub,
};

int main(void) {
  arena_t *frame = arena_new(MiB(16), MiB(1));

  cheese_t cheese = cheese_default(frame);
  cheese_input_t input = {0};

  cheese_begin(&cheese, frame, null, &g_renderer, input, 0.016f);

  u32 prop = cheese_prop_register(&cheese, "scoped/test", CHEESE_PROP_F32);

  cheese_style_t def = cheese_style_new();
  cheese_style_set_bg_color(&def, 0x11);
  cheese_style_set_text_color(&def, 0x99);
  cheese_style_set_prop_f32(&cheese, &def, prop, 1.0f);
  cheese_style_role_register(&cheese, CHEESE_ROLE_SLIDER, def);

  cheese_style_t scope = cheese_style_new();
  cheese_style_set_bg_color(&scope, 0x22);
  cheese_style_set_prop_f32(&cheese, &scope, prop, 2.0f);
  cheese_push_style(&cheese, scope);

  cheese_style_t cls = cheese_style_new();
  cheese_style_set_prop_f32(&cheese, &cls, prop, 3.0f);
  cheese_style_class_register(&cheese, "test-class", cls);

  cheese_style_t explicit = cheese_style_new();
  cheese_style_set_bg_color(&explicit, 0x44);
  cheese_style_set_prop_f32(&cheese, &explicit, prop, 4.0f);

  cheese_style_t out;

  cheese_style_resolve_scoped(&cheese, &out, CHEESE_ROLE_SLIDER, "test-class",
                              &explicit);
  assert(out.bg_color == 0x44 && "explicit wins the body field");
  assert(cheese_style_get_prop_f32(&out, prop, -1.0f) == 4.0f &&
         "explicit wins the property");

  cheese_style_resolve_scoped(&cheese, &out, CHEESE_ROLE_SLIDER, "test-class",
                              null);
  assert(cheese_style_get_prop_f32(&out, prop, -1.0f) == 3.0f &&
         "own class beats the scope");
  assert(out.bg_color == 0x22 && "scope beats the role default");
  assert(out.text_color == 0x99 && "the role default survives when unset");

  cheese_style_resolve_scoped(&cheese, &out, CHEESE_ROLE_SLIDER, null, null);
  assert(cheese_style_get_prop_f32(&out, prop, -1.0f) == 2.0f &&
         "without a class the scope wins");

  cheese_end(&cheese);
  arena_free(frame);
  return 0;
}
