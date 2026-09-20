/***********************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#include <htils/basictypes.h>

#include <cheese/log.h>
#include <cheese/types.h>

/***********************************/

#define COLOR_RESET "\x1b[0m"
#define COLOR_DARK_RED "\x1b[31m"
#define COLOR_RED "\x1b[91m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_CYAN "\x1b[36m"

#define CHEESE_LOG_HISTORY_SIZE 256

typedef struct {
  u64 hash;
  u64 count;
} cheese_log_entry_t;

static thread_local cheese_log_entry_t history[CHEESE_LOG_HISTORY_SIZE] = {0};
static thread_local u32 history_count = 0;
static thread_local u64 last_hash = 0;
static thread_local b32 line_active = false;
static once_flag log_atexit_once = ONCE_FLAG_INIT;

//
//
//

static u64 cheese_log_hash(const cstr *level, const cstr *msg) {
  u64 h = 0xcbf29ce484222325ULL;
  for (const cstr *p = level; *p; p++)
    h = (h ^ (u64)(u8)*p) * 0x100000001b3ULL;
  for (const cstr *p = msg; *p; p++)
    h = (h ^ (u64)(u8)*p) * 0x100000001b3ULL;
  return h;
}

//
//
//

static cheese_log_entry_t *cheese_log_find(u64 hash) {
  for (u32 i = 0; i < history_count; i++) {
    if (history[i].hash == hash)
      return &history[i];
  }
  u32 idx = history_count;
  if (idx >= CHEESE_LOG_HISTORY_SIZE)
    idx = 0;
  history[idx].hash = hash;
  history[idx].count = 0;
  if (idx == history_count)
    history_count++;
  return &history[idx];
}

//
//
//

static void cheese_log_atexit(void) {
  if (line_active) {
    fputc('\n', stderr);
    fflush(stderr);
    line_active = false;
  }
}

//
//
//

static void cheese_log_register_atexit(void) { atexit(cheese_log_atexit); }

//
//
//

void cheese_log(cheese_log_level_t level, const cstr *fmt, ...) {
#ifndef CHEESE_DEBUG
  if (level == CHEESE_LOG_DEBUG)
    return;
#endif

  call_once(&log_atexit_once, cheese_log_register_atexit);

  static cstr level_str[20] = {0};
  static cstr fmt_str[4096] = {0};

  switch (level) {
  case CHEESE_LOG_DEBUG:
    snprintf(level_str, 20, "%s[DEBUG]%s", COLOR_CYAN, COLOR_RESET);
    break;
  case CHEESE_LOG_INFO:
    snprintf(level_str, 20, "%s[INFO]%s", COLOR_CYAN, COLOR_RESET);
    break;
  case CHEESE_LOG_WARNING:
    snprintf(level_str, 20, "%s[WARN]%s", COLOR_YELLOW, COLOR_RESET);
    break;
  case CHEESE_LOG_ERROR:
    snprintf(level_str, 20, "%s[ERROR]%s", COLOR_RED, COLOR_RESET);
    break;
  case CHEESE_LOG_FATAL:
    snprintf(level_str, 20, "%s[FATAL]%s", COLOR_DARK_RED, COLOR_RESET);
    break;
  }

  va_list args;
  va_start(args, fmt);
  vsnprintf(fmt_str, 4096, fmt, args);
  va_end(args);

  u64 hash = cheese_log_hash(level_str, fmt_str);
  cheese_log_entry_t *entry = cheese_log_find(hash);
  entry->count++;

  if (hash == last_hash && line_active) {
    if (entry->count >= 2) {
      fprintf(stderr, "\r[CHEESE] %s: %s [x%lu]", level_str, fmt_str,
              entry->count);
      fflush(stderr);
    }
    return;
  }

  if (line_active)
    fprintf(stderr, "\n");

  if (entry->count == 1)
    fprintf(stderr, "\r[CHEESE] %s: %s", level_str, fmt_str);
  else
    fprintf(stderr, "\r[CHEESE] %s: %s [x%lu]", level_str, fmt_str,
            entry->count);
  fflush(stderr);

  last_hash = hash;
  line_active = true;
}
