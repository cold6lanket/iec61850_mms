#include <stdio.h>
#include <stdlib.h>

#define IEC61850_MMS_CLIENT_DISABLE_MAIN
#include "../client/src/iec61850_mms_client.c"

static int parse_port(const char *raw, int *port_out)
{
    char *end = NULL;
    long value = strtol(raw, &end, 10);

    if (raw == NULL || *raw == '\0' || end == raw || *end != '\0') {
        return -1;
    }

    if (value < 1 || value > 65535) {
        return -1;
    }

    *port_out = (int)value;
    return 0;
}

int main(int argc, char *argv[])
{
    const char *host = "127.0.0.1";
    int port = 102;
    const char *password = NULL;

    if (argc > 1) {
        host = argv[1];
    }

    if (argc > 2 && parse_port(argv[2], &port) != 0) {
        fprintf(stderr, "usage: %s [host] [port] [password]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc > 3) {
        password = argv[3];
    }

    cJSON *args = cJSON_CreateObject();
    cJSON_AddStringToObject(args, "host", host);
    cJSON_AddNumberToObject(args, "port", port);

    if (password != NULL && password[0] != '\0') {
        cJSON_AddStringToObject(args, "password", password);
    }

    char *error = NULL;
    cJSON *response = on_request("connect", args, &error);

    cJSON_Delete(args);

    if (response == NULL || error != NULL) {
        fprintf(stderr, "connect failed to %s:%d: %s\n", host, port, error ? error : "unknown error");
        if (response != NULL) {
            cJSON_Delete(response);
        }
        return EXIT_FAILURE;
    }

    printf("connect succeeded to %s:%d\n", host, port);
    cJSON_Delete(response);
    return EXIT_SUCCESS;
}
