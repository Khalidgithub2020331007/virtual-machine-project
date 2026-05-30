#include <stdio.h>
#include <stdint.h>
#include "io.h"

static IOOutputFn g_out_cb = NULL;

void io_set_output_callback(IOOutputFn fn) {
    g_out_cb = fn;
}

void io_print_int(uint32_t value) {
    if (g_out_cb) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%u", value);
        g_out_cb(buf);
    } else {
        printf("%u\n", value);
    }
}

void io_print_string(const char *str) {
    if (g_out_cb) g_out_cb(str);
    else          printf("%s\n", str);
}

uint32_t io_read_int(void) {
    if (g_out_cb) g_out_cb("[INPUT] Enter integer value in terminal:");
    printf("Input: ");
    fflush(stdout);
    uint32_t val = 0;
    scanf("%u", &val);
    if (g_out_cb) {
        char buf[32];
        snprintf(buf, sizeof(buf), "[INPUT] >> %u", val);
        g_out_cb(buf);
    }
    return val;
}
