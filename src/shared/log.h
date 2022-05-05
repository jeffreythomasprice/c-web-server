#ifndef base64_h
#define base64_h

#ifdef __cplusplus
extern "C" {
#endif

// TODO disable log levels based on a preprocessor or runtime config

void log_trace(char *format, ...);
void log_debug(char *format, ...);
void log_info(char *format, ...);
void log_error(char *format, ...);

#ifdef __cplusplus
}
#endif

#endif