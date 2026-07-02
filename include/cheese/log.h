#ifndef CHEESE_LOG_H
#define CHEESE_LOG_H

#include <htils/basictypes.h>

#include <cheese/types.h>

void cheese_log(cheese_log_level_t level, const cstr *fmt, ...);

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

#endif // !CHEESE_LOG_H
