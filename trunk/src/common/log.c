#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <pthread.h>

#include "log.h"

struct log_t * log_new(char* path, char *program_name, log_output mode) {
    log_t *self = calloc(sizeof(log_t), 1);

    self->path = malloc( strlen(path) + 1 );
    strcpy(self->path, path);

    self->program_name = malloc( strlen(program_name) + 1 );
    strcpy(self->program_name, program_name);

    self->mode = mode;

    self->programPID = getpid();

    if( self->mode == LOG_OUTPUT_FILE || self->mode == LOG_OUTPUT_CONSOLEANDFILE ){
        self->file = fopen( self->path , "a");
    }

    return self;
}

char *log_LevelAsString( log_level level ){
    if( level == LOG_ERROR)        return "ERROR";
    if( level == LOG_WARNING)    return "WARNING";
    if( level == LOG_INFO)        return "INFO";
    return "UNKNOW_LOG_LEVEL";
}

int log_write( struct log_t *self, char *thread_name , log_level level , const char* format, va_list args_list ){
    time_t log_time = time(NULL);
    struct tm *log_tm = localtime( &log_time );
    struct timeb tmili;
    char str_time[128] ;
    unsigned int thread_id = pthread_self();
    char *str_type = log_LevelAsString(level);
    char logbuff[MAX_LOGMESSAGE_LENGTH+100];
    char msgbuff[MAX_LOGMESSAGE_LENGTH];

    if( self->mode == LOG_OUTPUT_NONE )    return 1;

    vsprintf(msgbuff, format, args_list);

    strftime( str_time , 127 , "%H:%M:%S" , log_tm ) ;

    if (ftime (&tmili)){
        perror("[[CRITICAL]] :: No se han podido obtener los milisegundos\n");
        return 0;
    }

    sprintf( logbuff, "%s.%.3hu %s/%d %s/%u %s: %s\n",
            str_time, tmili.millitm, self->program_name , self->programPID ,
            thread_name , thread_id , str_type , msgbuff );

    if( self->mode == LOG_OUTPUT_FILE || self->mode == LOG_OUTPUT_CONSOLEANDFILE ){
        fprintf( self->file , "%s", logbuff );
        fflush( self->file ) ;
    }
    if( self->mode == LOG_OUTPUT_CONSOLE || self->mode == LOG_OUTPUT_CONSOLEANDFILE ){
        printf("%s", logbuff);
    }

    return 1;
}

int log_info(struct log_t *self, char *thread_name, char *format, ... ){
    va_list args_list;
    int ret=0;
    va_start(args_list, format);
    ret =  log_write(self, thread_name, LOG_INFO, format, args_list);
    va_end(args_list);
    return ret;
}

int log_warning(struct log_t *self, char *thread_name, char *format, ... ){
    va_list args_list;
    int ret=0;
    va_start(args_list, format);
    ret = log_write(self, thread_name, LOG_WARNING, format, args_list);
    va_end(args_list);
    return ret;
}

int log_error(struct log_t *self, char *thread_name, char *format, ... ){
    va_list args_list;
    int ret=0;
    va_start(args_list, format);
    ret = log_write(self, thread_name, LOG_ERROR, format, args_list);
    va_end(args_list);
    return ret;
}

void log_delete( struct log_t *self ){
    if( self->mode == LOG_OUTPUT_FILE || self->mode == LOG_OUTPUT_CONSOLEANDFILE ) fclose(self->file);
    free( self->path );
    free( self->program_name );
    free( self );
}
