#include "mms_client_test_common.h"

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

static void run_info_load(int iterations, LoadStat *stat)
{
    stat->name = "info";
    stat->iterations = iterations;
    stat->success = 0;
    stat->failure = 0;

    double start_ms = test_now_ms();
    for (int i = 0; i < iterations; i++) {
        char *error = NULL;
        cJSON *args = cJSON_CreateObject();
        cJSON *response = test_invoke_api("info", args, &error);
        cJSON_Delete(args);

        if (response != NULL && error == NULL &&
            cJSON_IsObject(response) &&
            cJSON_GetObjectItemCaseSensitive(response, "protocol") != NULL) {
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
    int info_iters = test_env_int("MMS_INFO_ITERS", 20000);
    LoadStat stat;

    test_reset_state();
    run_info_load(info_iters, &stat);

    printf("C API info load test (mocked backend)\n");
    test_print_stat(&stat);

    return (stat.failure == 0) ? 0 : 1;
}
