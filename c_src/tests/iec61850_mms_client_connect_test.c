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

static void run_connect_load(int iterations, LoadStat *stat)
{
    stat->name = "connect";
    stat->iterations = iterations;
    stat->success = 0;
    stat->failure = 0;

    double start_ms = test_now_ms();
    for (int i = 0; i < iterations; i++) {
        test_set_connected(false);

        char *error = NULL;
        cJSON *args = test_make_connect_args("127.0.0.1", 4102);
        cJSON *response = test_invoke_api("connect", args, &error);
        cJSON_Delete(args);

        if (response != NULL && error == NULL &&
            cJSON_IsString(response) &&
            strcmp(response->valuestring, "ok") == 0) {
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
    int connect_iters = test_env_int("MMS_CONNECT_ITERS", 20000);
    LoadStat stat;

    test_reset_state();
    run_connect_load(connect_iters, &stat);

    printf("C API connect load test (mocked backend)\n");
    test_print_stat(&stat);
    printf("connect backend calls: %d\n", test_get_connect_count());

    return (stat.failure == 0) ? 0 : 1;
}
