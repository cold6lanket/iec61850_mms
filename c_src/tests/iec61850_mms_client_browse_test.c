#include "mms_client_test_common.h"

#include <stdio.h>

#define IEC61850_MMS_CLIENT_DISABLE_MAIN
#include "../client/src/iec61850_mms_client.c"

static cJSON *on_request(char *method, cJSON *args, char **error);

static cJSON *test_invoke_api(const char *method, cJSON *args, char **error_out)
{
    char *error = NULL;
    cJSON *response = on_request((char *)method, args, &error);
    if (error_out) *error_out = error;
    return response;
}

static cJSON *find_named_node(cJSON *nodes, const char *name)
{
    cJSON *node = NULL;

    cJSON_ArrayForEach(node, nodes) {
        cJSON *node_name = cJSON_GetObjectItemCaseSensitive(node, "name");
        if (cJSON_IsString(node_name) && strcmp(node_name->valuestring, name) == 0) {
            return node;
        }
    }

    return NULL;
}

static int assert_browse_shape(void)
{
    char *error = NULL;
    cJSON *args = test_make_browse_args("127.0.0.1", 4102);
    cJSON *response = test_invoke_api("browse", args, &error);

    if (error != NULL || response == NULL || !cJSON_IsArray(response)) {
        fprintf(stderr, "browse should return a JSON array without error\n");
        cJSON_Delete(args);
        if (response) cJSON_Delete(response);
        return 1;
    }

    cJSON *ld = cJSON_GetArrayItem(response, 0);
    cJSON *logical_nodes = cJSON_GetObjectItemCaseSensitive(ld, "logical_nodes");
    cJSON *ln = cJSON_GetArrayItem(logical_nodes, 0);
    cJSON *tree = cJSON_GetObjectItemCaseSensitive(ln, "data_object_tree");

    if (!cJSON_IsArray(tree) || cJSON_GetArraySize(tree) != 2) {
        fprintf(stderr, "expected two root data objects in data_object_tree\n");
        cJSON_Delete(args);
        cJSON_Delete(response);
        return 1;
    }

    cJSON *phv = find_named_node(tree, "PhV");
    cJSON *phv_children = (phv != NULL) ? cJSON_GetObjectItemCaseSensitive(phv, "children") : NULL;
    cJSON *phsA = find_named_node(phv_children, "phsA");
    cJSON *phsA_children = (phsA != NULL) ? cJSON_GetObjectItemCaseSensitive(phsA, "children") : NULL;
    cJSON *cVal = find_named_node(phsA_children, "cVal");
    cJSON *cVal_children = (cVal != NULL) ? cJSON_GetObjectItemCaseSensitive(cVal, "children") : NULL;
    cJSON *ang = find_named_node(cVal_children, "ang");
    cJSON *ang_children = (ang != NULL) ? cJSON_GetObjectItemCaseSensitive(ang, "children") : NULL;
    cJSON *f = find_named_node(ang_children, "f");

    if (phv == NULL || !cJSON_IsArray(phv_children) || phsA == NULL || !cJSON_IsArray(phsA_children) ||
        cVal == NULL || !cJSON_IsArray(cVal_children) || ang == NULL || !cJSON_IsArray(ang_children) || f == NULL) {
        fprintf(stderr, "expected nested PhV -> phsA -> cVal -> ang -> f tree nodes\n");
        cJSON_Delete(args);
        cJSON_Delete(response);
        return 1;
    }

    cJSON *ang_fc = cJSON_GetObjectItemCaseSensitive(ang, "fc");
    cJSON *ang_kind = cJSON_GetObjectItemCaseSensitive(ang, "kind");
    cJSON *f_type = cJSON_GetObjectItemCaseSensitive(f, "mms_type");
    cJSON *f_path = cJSON_GetObjectItemCaseSensitive(f, "tree_path");

    if (!cJSON_IsString(ang_fc) || strcmp(ang_fc->valuestring, "MX") != 0 ||
        !cJSON_IsString(ang_kind) || strcmp(ang_kind->valuestring, "component") != 0 ||
        !cJSON_IsString(f_type) || strcmp(f_type->valuestring, "FLOAT") != 0 ||
        !cJSON_IsString(f_path) || strcmp(f_path->valuestring, "LLN0.MX.PhV.phsA.cVal.ang.f") != 0) {
        fprintf(stderr, "nested browse metadata does not match expected FC/type/tree path values\n");
        cJSON_Delete(args);
        cJSON_Delete(response);
        return 1;
    }

    cJSON_Delete(args);
    cJSON_Delete(response);
    return 0;
}

static void run_browse_load(int iterations, LoadStat *stat)
{
    stat->name = "browse";
    stat->iterations = iterations;
    stat->success = 0;
    stat->failure = 0;

    double start_ms = test_now_ms();
    for (int i = 0; i < iterations; i++) {
        char *error = NULL;
        cJSON *args = test_make_browse_args("127.0.0.1", 4102);
        cJSON *response = test_invoke_api("browse", args, &error);
        cJSON_Delete(args);

        if (response != NULL && error == NULL && cJSON_IsArray(response)) {
            stat->success++;
        } else {
            stat->failure++;
        }

        if (response) cJSON_Delete(response);
    }
    stat->elapsed_ms = test_now_ms() - start_ms;
}

int main(void)
{
    int browse_iters = test_env_int("MMS_BROWSE_ITERS", 10000);
    LoadStat stat;

    test_reset_state();

    if (assert_browse_shape() != 0) {
        return 1;
    }

    run_browse_load(browse_iters, &stat);

    printf("C API browse load test (mocked backend)\n");
    test_print_stat(&stat);

    return (stat.failure == 0) ? 0 : 1;
}
