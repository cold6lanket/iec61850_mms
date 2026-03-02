#include "iec61850_mms_server_loop.h"
#include "hal_thread.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"

static int running = 0;
void sigint_handler(int signalId) { running = 0; }

//-----------------------------------------------------
// Server Start Controller
//-----------------------------------------------------
static cJSON* iec61850_server_start(cJSON* args, char **error) {
    int tcpPort = 102; 
    if (cJSON_IsObject(args)) {
        cJSON *portItem = cJSON_GetObjectItemCaseSensitive(args, "port");
        if (cJSON_IsNumber(portItem)) tcpPort = portItem->valueint;
    }

    *error = start(tcpPort);
    if (*error) return NULL;

    return cJSON_CreateString("ok");
}

//-----------------------------------------------------
// Write Controller
//-----------------------------------------------------
static cJSON* iec61850_server_write_items(cJSON* args, char **error) {
    if (!cJSON_IsObject(args)) {
        *error = "invalid arguments";
        return NULL;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON *item = NULL;

    cJSON_ArrayForEach(item, args) {
        const char *path = item->string; 
        cJSON *typeJson = cJSON_GetObjectItemCaseSensitive(item, "type");
        cJSON *valueJson = cJSON_GetObjectItemCaseSensitive(item, "value");
        
        char *err = NULL;

        if (!typeJson || !typeJson->valuestring || !valueJson) {
            err = "missing type or value";
        } else {
            // Pass straight to loop logic
            err = write_server_item(path, typeJson->valuestring, valueJson);
        }
        
        cJSON_AddStringToObject(response, path, err ? err : "ok");
    }
    return response;
}

//-----------------------------------------------------
// Read Controller
//-----------------------------------------------------
static cJSON* iec61850_server_read_items(cJSON* args, char **error) {
    if (!cJSON_IsArray(args)) {
        *error = "invalid arguments";
        return NULL;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON *item = NULL;

    cJSON_ArrayForEach(item, args) {
        if (!cJSON_IsString(item)) continue;
        
        cJSON *valJson = NULL;
        char *err = read_server_item(item->valuestring, &valJson);
        
        if (err) {
            cJSON_AddStringToObject(response, item->valuestring, err);
        } else {
            // Simply attach the JSON value we got back from mms2json
            cJSON_AddItemToObject(response, item->valuestring, valJson);
        }
    }
    return response;
}

//-----------------------------------------------------
// Main Test Harness
//-----------------------------------------------------
int main(int argc, char** argv) {
    running = 1;
    signal(SIGINT, sigint_handler);

    printf("Starting IEC 61850 Server...\n");

    char *err = NULL;
    cJSON *response = NULL;

    // Start
    cJSON *startArgs = cJSON_Parse("{\"port\": 8102}");
    response = iec61850_server_start(startArgs, &err);
    if (err) { printf("Start Error: %s\n", err); return 1; }
    printf("Server started on port 8102.\n");
    cJSON_Delete(startArgs); cJSON_Delete(response);

    const char* test_path = "SampleIEDDevice1/MMXU1.Mod.ctlModel";

    // WRITE: Notice the type is "Float", matching json2mms in utils.c!
    char writeJson[256];
    snprintf(writeJson, sizeof(writeJson), "{\"%s\": {\"type\": \"Int\", \"value\": 1}}", test_path);
    cJSON *writeArgs = cJSON_Parse(writeJson);
    response = iec61850_server_write_items(writeArgs, &err);
    printf("Write Result: %s\n", response ? cJSON_PrintUnformatted(response) : err);
    cJSON_Delete(writeArgs); cJSON_Delete(response);

    Thread_sleep(1000);

    // READ
    char readJson[256];
    snprintf(readJson, sizeof(readJson), "[\"%s\"]", test_path);
    cJSON *readArgs = cJSON_Parse(readJson);
    err = NULL;
    response = iec61850_server_read_items(readArgs, &err);
    printf("Read Result: %s\n", response ? cJSON_PrintUnformatted(response) : err);
    cJSON_Delete(readArgs); cJSON_Delete(response);

    printf("\nServer running. Press Ctrl+C to stop.\n");
    while (running) { Thread_sleep(100); }

    printf("\nStopping Server...\n");
    stop();
    return 0;
}