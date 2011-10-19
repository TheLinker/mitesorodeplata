#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#define MAX_LOGMESSAGE_LENGTH 1024

typedef enum {
    LOG_ERROR   = 1,
    LOG_WARNING = 2,
    LOG_INFO    = 3
} log_level;

typedef enum {
    LOG_OUTPUT_NONE           = 1,
    LOG_OUTPUT_CONSOLE        = 2,
    LOG_OUTPUT_FILE           = 3,
    LOG_OUTPUT_CONSOLEANDFILE = 4
} log_output;

typedef struct log_t {
    char      *path;
    FILE      *file;
    int        programPID;
    char      *program_name;
    log_output mode;
} log_t;

struct log_t* log_new(char* path, char *program_name, log_output mode);
int           log_info(struct log_t *self, char *thread_name, char *format, ... );
int           log_warning(struct log_t *self, char *thread_name, char *format, ... );
int           log_error(struct log_t *self, char *thread_name, char *format, ... );
void          log_delete( struct log_t * self );

#endif /*LOG_H_*/
