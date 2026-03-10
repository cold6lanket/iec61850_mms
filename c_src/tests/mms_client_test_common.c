#include "mms_client_test_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool g_connected = false;
static int g_read_counter = 0;
static int g_connect_counter = 0;

static char *dup_cstr(const char *text)
{
    size_t size = strlen(text) + 1;
    char *out = malloc(size);
    if (out != NULL) {
        memcpy(out, text, size);
    }
    return out;
}

int test_env_int(const char *name, int fallback)
{
    const char *raw = getenv(name);
    if (raw == NULL || *raw == '\0') return fallback;

    char *end = NULL;
    long value = strtol(raw, &end, 10);
    if (end == raw || *end != '\0' || value <= 0) return fallback;

    return (int)value;
}

double test_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + ((double)ts.tv_nsec / 1000000.0);
}

void test_print_stat(const LoadStat *stat)
{
    double seconds = stat->elapsed_ms / 1000.0;
    double rps = (seconds > 0.0) ? ((double)stat->iterations / seconds) : 0.0;

    printf("%-12s iterations=%d success=%d failure=%d elapsed_ms=%.2f throughput=%.2f req/s\n",
           stat->name,
           stat->iterations,
           stat->success,
           stat->failure,
           stat->elapsed_ms,
           rps);
}

void test_reset_state(void)
{
    g_connected = false;
    g_read_counter = 0;
    g_connect_counter = 0;
}

void test_set_connected(bool connected)
{
    g_connected = connected;
}

int test_get_connect_count(void)
{
    return g_connect_counter;
}

cJSON *test_make_connect_args(const char *host, int port)
{
    cJSON *args = cJSON_CreateObject();
    cJSON_AddStringToObject(args, "host", host);
    cJSON_AddNumberToObject(args, "port", port);
    return args;
}

cJSON *test_make_browse_args(const char *host, int port)
{
    cJSON *args = cJSON_CreateObject();
    cJSON_AddStringToObject(args, "host", host);
    cJSON_AddNumberToObject(args, "port", port);
    return args;
}

cJSON *test_make_read_args(const char *path, const char *fc)
{
    cJSON *args = cJSON_CreateObject();
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "fcType", fc);
    cJSON_AddItemToObject(args, path, item);
    return args;
}

cJSON *test_make_write_args(const char *path, const char *fc, const char *type, int value)
{
    cJSON *args = cJSON_CreateObject();
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "fcType", fc);
    cJSON_AddStringToObject(item, "type", type);
    cJSON_AddNumberToObject(item, "value", value);
    cJSON_AddItemToObject(args, path, item);
    return args;
}

/* -------------------------------------------------------------------------- */
/* Mock IEC 61850 client loop functions used by iec61850_mms_client.c         */
/* -------------------------------------------------------------------------- */

char *start(char *host, int port)
{
    (void)host;
    (void)port;
    g_connected = true;
    g_connect_counter++;
    return NULL;
}

bool is_connected(void)
{
    return g_connected;
}

MmsValue *read_value(const char *path, const char *fcType)
{
    if (!g_connected) return MmsValue_newVisibleString("ERROR: Not connected");
    if (path == NULL || fcType == NULL) return MmsValue_newVisibleString("ERROR: Invalid args");

    g_read_counter++;
    return MmsValue_newIntegerFromInt32(g_read_counter);
}

IedClientError write_value(const char *path, const char *fcType, MmsValue *value)
{
    (void)path;
    (void)fcType;
    (void)value;

    if (!g_connected) return IED_ERROR_CONNECTION_LOST;
    return IED_ERROR_OK;
}

char *browse_server(char *host, int port, BrowseLD ***devices_out)
{
    (void)host;
    (void)port;

    BrowseLD **devices = calloc(2, sizeof(BrowseLD *));
    BrowseLN **nodes = calloc(2, sizeof(BrowseLN *));
    char **data_objects = calloc(2, sizeof(char *));

    if (!devices || !nodes || !data_objects) {
        free(devices);
        free(nodes);
        free(data_objects);
        return "out of memory";
    }

    BrowseLD *ld = calloc(1, sizeof(BrowseLD));
    BrowseLN *ln = calloc(1, sizeof(BrowseLN));
    if (!ld || !ln) {
        free(ld);
        free(ln);
        free(devices);
        free(nodes);
        free(data_objects);
        return "out of memory";
    }

    ld->ld_name = dup_cstr("MockLD1");
    ln->ln_name = dup_cstr("LLN0");
    data_objects[0] = dup_cstr("Mod");
    ln->data_objects = data_objects;

    nodes[0] = ln;
    ld->logical_nodes = nodes;
    devices[0] = ld;

    *devices_out = devices;
    return NULL;
}

void free_browse_results(BrowseLD **devices)
{
    if (!devices) return;

    for (int i = 0; devices[i]; i++) {
        BrowseLD *ld = devices[i];
        free(ld->ld_name);

        if (ld->logical_nodes) {
            for (int j = 0; ld->logical_nodes[j]; j++) {
                BrowseLN *ln = ld->logical_nodes[j];
                free(ln->ln_name);

                if (ln->data_objects) {
                    for (int k = 0; ln->data_objects[k]; k++) {
                        free(ln->data_objects[k]);
                    }
                    free(ln->data_objects);
                }

                if (ln->data_sets) free(ln->data_sets);
                if (ln->urcbs) free(ln->urcbs);
                if (ln->brcbs) free(ln->brcbs);
                free(ln);
            }
            free(ld->logical_nodes);
        }
        free(ld);
    }
    free(devices);
}
