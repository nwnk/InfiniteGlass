#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

extern FILE *eventlog;

extern int debug_enabled(const int dfl, const char * prefix, const char *file, const char *func, const char *entry);
extern void debug_print(FILE *fd, const int dfl, const char *prefix, const char *file, const char *func, const char *entry, const char * format, ...);

#define DEBUG(entry, ...) { if (DEBUG_ENABLED(entry)) debug_print(stderr, 0, "GLASS_DEBUG.renderer", __FILE__, __func__, entry, __VA_ARGS__); }
#define DEBUG_ENABLED(entry) ({ static int res = -1; if (res == -1) res = debug_enabled(0, "GLASS_DEBUG.renderer", __FILE__, __func__, entry); res; })

#define ERROR(entry, ...) { if (ERROR_ENABLED(entry)) debug_print(stderr, 1, "GLASS_ERROR.renderer", __FILE__, __func__, entry, __VA_ARGS__); }
#define ERROR_ENABLED(entry) ({ static int res = -1; if (res == -1) res = debug_enabled(1, "GLASS_ERROR.renderer", __FILE__, __func__, entry); res; })

#define EVENTLOG(entry, ...) { if (EVENTLOG_ENABLED(entry)) { if (!eventlog) eventlog = fopen("eventlog.log", "w"); fprintf(eventlog, __VA_ARGS__); }; }
#define EVENTLOG_ENABLED(entry) ({ static int res = -1; if (res == -1) res = debug_enabled(0, "GLASS_EVENTLOG.renderer", __FILE__, __func__, entry); res; })

#endif
