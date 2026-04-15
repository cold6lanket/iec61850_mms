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

static BrowseModelNode *make_model_node(const char *name,
                                        const char *object_reference,
                                        const char *tree_path,
                                        const char *fc,
                                        const char *mms_type,
                                        int size,
                                        BrowseNodeKind kind)
{
    BrowseModelNode *node = calloc(1, sizeof(BrowseModelNode));
    if (node == NULL) return NULL;

    node->name = dup_cstr(name);
    node->object_reference = dup_cstr(object_reference);
    node->tree_path = dup_cstr(tree_path);
    node->fc = (fc != NULL) ? dup_cstr(fc) : NULL;
    node->mms_type = (mms_type != NULL) ? dup_cstr(mms_type) : NULL;
    node->size = size;
    node->kind = kind;

    if (node->name == NULL || node->object_reference == NULL || node->tree_path == NULL ||
        ((fc != NULL) && node->fc == NULL) ||
        ((mms_type != NULL) && node->mms_type == NULL)) {
        free(node->name);
        free(node->object_reference);
        free(node->tree_path);
        free(node->fc);
        free(node->mms_type);
        free(node);
        return NULL;
    }

    return node;
}

static void free_model_nodes(BrowseModelNode **nodes)
{
    if (nodes == NULL) return;

    for (int i = 0; nodes[i] != NULL; i++) {
        BrowseModelNode *node = nodes[i];
        free(node->name);
        free(node->object_reference);
        free(node->tree_path);
        free(node->fc);
        free(node->mms_type);
        free_model_nodes(node->children);
        free(node);
    }

    free(nodes);
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

char *start(char *host, int port, const char *password)
{
    (void)host;
    (void)port;
    (void)password;
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
    char **data_objects = calloc(3, sizeof(char *));
    BrowseModelNode **data_object_tree = calloc(3, sizeof(BrowseModelNode *));
    BrowseModelNode **mod_children = calloc(4, sizeof(BrowseModelNode *));
    BrowseModelNode **phv_children = calloc(3, sizeof(BrowseModelNode *));
    BrowseModelNode **phsA_children = calloc(3, sizeof(BrowseModelNode *));
    BrowseModelNode **cVal_children = calloc(3, sizeof(BrowseModelNode *));
    BrowseModelNode **ang_children = calloc(2, sizeof(BrowseModelNode *));

    if (!devices || !nodes || !data_objects || !data_object_tree || !mod_children ||
        !phv_children || !phsA_children || !cVal_children || !ang_children) {
        free(devices);
        free(nodes);
        free(data_objects);
        free(data_object_tree);
        free(mod_children);
        free(phv_children);
        free(phsA_children);
        free(cVal_children);
        free(ang_children);
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
        free(data_object_tree);
        free(mod_children);
        free(phv_children);
        free(phsA_children);
        free(cVal_children);
        free(ang_children);
        return "out of memory";
    }

    ld->ld_name = dup_cstr("MockLD1");
    ln->ln_name = dup_cstr("LLN0");
    data_objects[0] = dup_cstr("Mod");
    data_objects[1] = dup_cstr("PhV");
    ln->data_objects = data_objects;

    data_object_tree[0] = make_model_node("Mod",
                                          "MockLD1/LLN0.Mod",
                                          "LLN0.Mod",
                                          NULL,
                                          NULL,
                                          -1,
                                          BROWSE_NODE_DATA_OBJECT);
    data_object_tree[1] = make_model_node("PhV",
                                          "MockLD1/LLN0.PhV",
                                          "LLN0.PhV",
                                          NULL,
                                          NULL,
                                          -1,
                                          BROWSE_NODE_DATA_OBJECT);

    mod_children[0] = make_model_node("q",
                                      "MockLD1/LLN0.Mod.q",
                                      "LLN0.ST.Mod.q",
                                      "ST",
                                      "BIT_STRING",
                                      13,
                                      BROWSE_NODE_DATA_ATTRIBUTE);
    mod_children[1] = make_model_node("t",
                                      "MockLD1/LLN0.Mod.t",
                                      "LLN0.ST.Mod.t",
                                      "ST",
                                      "UTC_TIME",
                                      -1,
                                      BROWSE_NODE_DATA_ATTRIBUTE);
    mod_children[2] = make_model_node("ctlModel",
                                      "MockLD1/LLN0.Mod.ctlModel",
                                      "LLN0.CF.Mod.ctlModel",
                                      "CF",
                                      "INTEGER",
                                      32,
                                      BROWSE_NODE_DATA_ATTRIBUTE);

    phv_children[0] = make_model_node("phsA",
                                      "MockLD1/LLN0.PhV.phsA",
                                      "LLN0.MX.PhV.phsA",
                                      "MX",
                                      "STRUCTURE",
                                      2,
                                      BROWSE_NODE_COMPONENT);
    phv_children[1] = make_model_node("q",
                                      "MockLD1/LLN0.PhV.q",
                                      "LLN0.MX.PhV.q",
                                      "MX",
                                      "BIT_STRING",
                                      13,
                                      BROWSE_NODE_DATA_ATTRIBUTE);

    phsA_children[0] = make_model_node("cVal",
                                       "MockLD1/LLN0.PhV.phsA.cVal",
                                       "LLN0.MX.PhV.phsA.cVal",
                                       "MX",
                                       "STRUCTURE",
                                       2,
                                       BROWSE_NODE_COMPONENT);
    phsA_children[1] = make_model_node("q",
                                       "MockLD1/LLN0.PhV.phsA.q",
                                       "LLN0.MX.PhV.phsA.q",
                                       "MX",
                                       "BIT_STRING",
                                       13,
                                       BROWSE_NODE_DATA_ATTRIBUTE);

    cVal_children[0] = make_model_node("mag",
                                       "MockLD1/LLN0.PhV.phsA.cVal.mag",
                                       "LLN0.MX.PhV.phsA.cVal.mag",
                                       "MX",
                                       "STRUCTURE",
                                       1,
                                       BROWSE_NODE_COMPONENT);
    cVal_children[1] = make_model_node("ang",
                                       "MockLD1/LLN0.PhV.phsA.cVal.ang",
                                       "LLN0.MX.PhV.phsA.cVal.ang",
                                       "MX",
                                       "STRUCTURE",
                                       1,
                                       BROWSE_NODE_COMPONENT);

    ang_children[0] = make_model_node("f",
                                      "MockLD1/LLN0.PhV.phsA.cVal.ang.f",
                                      "LLN0.MX.PhV.phsA.cVal.ang.f",
                                      "MX",
                                      "FLOAT",
                                      32,
                                      BROWSE_NODE_DATA_ATTRIBUTE);

    if (data_object_tree[0] == NULL || data_object_tree[1] == NULL ||
        mod_children[0] == NULL || mod_children[1] == NULL || mod_children[2] == NULL ||
        phv_children[0] == NULL || phv_children[1] == NULL ||
        phsA_children[0] == NULL || phsA_children[1] == NULL ||
        cVal_children[0] == NULL || cVal_children[1] == NULL ||
        ang_children[0] == NULL) {
        free_model_nodes(data_object_tree);
        free_model_nodes(mod_children);
        free_model_nodes(phv_children);
        free_model_nodes(phsA_children);
        free_model_nodes(cVal_children);
        free_model_nodes(ang_children);
        free(ld->ld_name);
        free(ln->ln_name);
        free(ld);
        free(ln);
        free(devices);
        free(nodes);
        for (int i = 0; i < 2; i++) free(data_objects[i]);
        free(data_objects);
        return "out of memory";
    }

    cVal_children[1]->children = ang_children;
    phsA_children[0]->children = cVal_children;
    phv_children[0]->children = phsA_children;
    data_object_tree[0]->children = mod_children;
    data_object_tree[1]->children = phv_children;
    ln->data_object_tree = data_object_tree;

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

                if (ln->data_object_tree) {
                    free_model_nodes(ln->data_object_tree);
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
