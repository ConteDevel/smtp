#include "log.h"

#include <stdio.h>

void log_stdout(const char lvl, const char *msg) {
	printf("%c: %s\n", lvl, msg);
}

void log_stderr(const char lvl, const char *msg) {
	printf("%c: %s\n", lvl, msg);
}

void log_d(const char *msg) {
    log_stdout('D', msg);
}

void log_e(const char *msg) {
    log_stderr('E', msg);
}

void log_f(const char *msg) {
    log_stderr('F', msg);
}

void log_i(const char *msg) {
    log_stdout('I', msg);
}

void log_w(const char *msg) {
    log_stdout('W', msg);
}
