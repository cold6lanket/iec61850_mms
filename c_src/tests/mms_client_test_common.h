#ifndef MMS_CLIENT_TEST_COMMON_H
#define MMS_CLIENT_TEST_COMMON_H

#include <stdbool.h>
#include <time.h>

#include "iec61850_client.h"
#include "iec61850_mms_client_loop.h"
#include "utils.h"
#include "cJSON.h"

typedef struct {
    const char *name;
    int iterations;
    int success;
    int failure;
    double elapsed_ms;
} LoadStat;

int test_env_int(const char *name, int fallback);
double test_now_ms(void);
void test_print_stat(const LoadStat *stat);
void test_reset_state(void);
void test_set_connected(bool connected);
int test_get_connect_count(void);

cJSON *test_make_connect_args(const char *host, int port);
cJSON *test_make_browse_args(const char *host, int port);
cJSON *test_make_read_args(const char *path, const char *fc);
cJSON *test_make_write_args(const char *path, const char *fc, const char *type, int value);

#endif
