/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

#include <cheese/core/app.h>
#include <cheese/core/state.h>

/***********************************/

cheese_app_result_t cheese_app_start(cheese_t *cheese,
                                     const cheese_app_t *app) {
  if (!cheese || !app || !app->start)
    return CHEESE_CONTINUE;

  return app->start(cheese, app->userdata);
}

cheese_app_result_t cheese_app_update(cheese_t *cheese, const cheese_app_t *app,
                                      f32 dt) {
  if (!cheese)
    return CHEESE_FAILURE;

  cheese_dispatch(cheese);

  if (!app || !app->update)
    return CHEESE_CONTINUE;

  return app->update(cheese, dt, app->userdata);
}

void cheese_app_render(cheese_t *cheese, const cheese_app_t *app) {
  if (!cheese || !app || !app->render)
    return;

  app->render(cheese, app->userdata);
}

void cheese_app_stop(cheese_t *cheese, const cheese_app_t *app) {
  if (!cheese || !app || !app->stop)
    return;

  app->stop(cheese, app->userdata);
}
