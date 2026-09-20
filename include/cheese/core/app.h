#ifndef CHEESE_CORE_APP_H
#define CHEESE_CORE_APP_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Run the app's @c start callback, if set.
 *
 * @param cheese The cheese context.
 * @param app The app harness.
 *
 * @return CHEESE_CONTINUE, or the callback's failure result.
 */
cheese_app_result_t cheese_app_start(cheese_t *cheese, const cheese_app_t *app);

//
//
//

/**
 * @brief Tick: dispatch the store, then run the app's @c update callback.
 *
 * @param cheese The cheese context.
 * @param app The app harness.
 * @param dt Seconds since the previous tick.
 *
 * @return The callback's result (CHEESE_CONTINUE if absent).
 */
cheese_app_result_t cheese_app_update(cheese_t *cheese, const cheese_app_t *app,
                                      f32 dt);

//
//
//

/**
 * @brief Run the app's @c render callback.
 * @details Call between @ref cheese_begin and @ref cheese_end.
 *
 * @param cheese The cheese context.
 * @param app The app harness.
 */
void cheese_app_render(cheese_t *cheese, const cheese_app_t *app);

//
//
//

/**
 * @brief Run the app's @c stop callback.
 *
 * @param cheese The cheese context.
 * @param app The app harness.
 */
void cheese_app_stop(cheese_t *cheese, const cheese_app_t *app);

#endif // !CHEESE_CORE_APP_H
