#ifndef CHEESE_LOG_H
#define CHEESE_LOG_H

/***********************************/

#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

void cheese_log(cheese_log_level_t level, const cstr *fmt, ...);

//
//
//

#define cheese_log_debug(fmt, ...)                                             \
  cheese_log(CHEESE_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define cheese_log_info(fmt, ...)                                              \
  cheese_log(CHEESE_LOG_INFO, fmt, ##__VA_ARGS__)
#define cheese_log_warning(fmt, ...)                                           \
  cheese_log(CHEESE_LOG_WARNING, fmt, ##__VA_ARGS__)
#define cheese_log_error(fmt, ...)                                             \
  cheese_log(CHEESE_LOG_ERROR, fmt, ##__VA_ARGS__)
#define cheese_log_fatal(fmt, ...)                                             \
  cheese_log(CHEESE_LOG_FATAL, fmt, ##__VA_ARGS__)

/**
 * @brief Log a warning once per call site.
 * @details Each expansion owns a block-scope flag, so the first time a given
 * call site hits it logs, and every later hit is silent - no per-frame spam.
 *
 * @param fmt The printf-style format string.
 * @param ... The format arguments.
 */
#define cheese_log_warn_once(fmt, ...)                                         \
  do {                                                                         \
    static b32 cheese_warned_once = false;                                     \
    if (!cheese_warned_once) {                                                 \
      cheese_warned_once = true;                                               \
      cheese_log(CHEESE_LOG_WARNING, fmt, ##__VA_ARGS__);                      \
    }                                                                          \
  } while (0)

#endif // !CHEESE_LOG_H
