#include <stdarg.h>
#include <stdio.h>

#include "log.h"

void log_trace(char *format, ...) {
	va_list args;
	va_start(args, format);
	fprintf(stdout, "TRACE ");
	vfprintf(stdout, format, args);
	fflush(stdout);
	va_end(args);
}

void log_debug(char *format, ...) {
	va_list args;
	va_start(args, format);
	fprintf(stdout, "DEBUG ");
	vfprintf(stdout, format, args);
	fflush(stdout);
	va_end(args);
}

void log_info(char *format, ...) {
	va_list args;
	va_start(args, format);
	fprintf(stdout, "INFO ");
	vfprintf(stdout, format, args);
	fflush(stdout);
	va_end(args);
}

void log_error(char *format, ...) {
	va_list args;
	va_start(args, format);
	fprintf(stderr, "ERROR ");
	vfprintf(stderr, format, args);
	fflush(stderr);
	va_end(args);
}