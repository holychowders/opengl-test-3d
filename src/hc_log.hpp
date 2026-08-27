#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

// Continue a previous log message indented to a specified level on a new line
static inline void cont(unsigned int indent_level, const char *description) {
    for (unsigned int i{}; i < indent_level; i++) {
        if (i == 0) {
            printf("      "); // deeper initial indent to handle log message headers
        }
        else { printf("    "); }
    }
    printf("%s\n", description);
    fflush(stdout);
}
// Continue a previous log message indented on a new line
static inline void cont(const char *description) {
    cont(1, description);
}

static inline void marker() {
    puts("MARKER: ***************************************************************************\n");
    fflush(stdout);
}
static inline void info(const char *regarding, const char *description) {
    if (regarding) { fprintf(stdout, "INFO [%s]: %s\n", regarding, description); }
    else { fprintf(stdout, "INFO: %s\n", description); }
    fflush(stdout);
}
static inline void info_stderr(const char *regarding, const char *description) {
    if (regarding) { fprintf(stderr, "INFO [%s]: %s\n", regarding, description); }
    else { fprintf(stderr, "INFO: %s\n", description); }
}
static inline void warn(const char *regarding, const char *description) {
    if (regarding) { fprintf(stderr, "WARN [%s]: %s\n", regarding, description); }
    else { fprintf(stderr, "WARN: %s\n", description); }
}
static inline void error(const char *regarding, const char *description) {
    if (regarding) { fprintf(stderr, "FAIL [%s]: %s\n", regarding, description); }
    else { fprintf(stderr, "FAIL: %s\n", description); }
}
static inline void error(const char *regarding, const char *description, const char *fpath, int lineno) {
    fprintf(stderr, "FAIL [%s:%d] [%s]: %s\n", fpath, lineno, regarding, description);
}
static inline void error(const char *description, const char *fpath, int lineno) {
    fprintf(stderr, "FAIL [%s:%d]: %s\n", fpath, lineno, description);
}

//FAIL [main.cpp:420] [GLEW]: There was an error
//FAIL [main.cpp:420 | GLEW]: There was an error

// Wrappers

static inline void info(const char *description) {
    info(nullptr, description);
}
static inline void info_stderr(const char *description) {
    info_stderr(nullptr, description);
}
static inline void warn(const char *description) {
    warn(nullptr, description);
}
static inline void error(const char *description) {
    error(nullptr, description);
}

// Variadic

// NOTE: Do not use directly. Use finfo(...) instead.
__attribute__((format(printf, 2, 0))) static inline void v_info(const char *regarding, const char *fmt_msg, va_list fmt_args) {
    if (regarding) { printf("INFO [%s]: ", regarding); }
    else { printf("INFO: "); }
    vprintf(fmt_msg, fmt_args);
    fputc('\n', stdout);
    fflush(stdout);
}
__attribute__((format(printf, 2, 3))) static inline void finfo(const char *regarding, const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_info(regarding, fmt_msg, args);
    va_end(args);
}
__attribute__((format(printf, 1, 2))) static inline void finfo(const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_info(nullptr, fmt_msg, args);
    va_end(args);
}

// NOTE: Do not use directly. Use fwarn(...) instead.
__attribute__((format(printf, 2, 0))) static inline void v_warn(const char *regarding, const char *fmt_msg, va_list fmt_args) {
    if (regarding) { fprintf(stderr, "WARN [%s]: ", regarding); }
    else { fprintf(stderr, "WARN: "); }
    vfprintf(stderr, fmt_msg, fmt_args);
    fputc('\n', stderr);
}
__attribute__((format(printf, 1, 2))) static inline void fwarn(const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_warn(nullptr, fmt_msg, args);
    va_end(args);
}
__attribute__((format(printf, 2, 3))) static inline void fwarn(const char *regarding, const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_warn(regarding, fmt_msg, args);
    va_end(args);
}

// NOTE: Do not use directly. Use ferror(...) instead.
__attribute__((format(printf, 2, 0))) static inline void v_error(const char *regarding, const char *fmt_msg, va_list fmt_args) {
    if (regarding) { fprintf(stderr, "FAIL [%s]: ", regarding); }
    else { fprintf(stderr, "FAIL: "); }
    vfprintf(stderr, fmt_msg, fmt_args);
    fputc('\n', stderr);
}
__attribute__((format(printf, 2, 3))) static inline void ferror(const char *regarding, const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_error(regarding, fmt_msg, args);
    va_end(args);
}
