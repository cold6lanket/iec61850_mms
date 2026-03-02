#include "iec61850_mms_server_loop.h"
#include "hal_thread.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"

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
//  eport_c request routing
//-----------------------------------------------------
static cJSON* on_request( char *method, cJSON *args, char **error ){
    
    cJSON *response = NULL;
    // Handle the request
    // LOGTRACE("handle the request %s", method);

    if (strcmp(method, "server_start") == 0) {
        response = iec61850_server_start( args, error );
    } else if (strcmp(method, "write_items") == 0) {
        response = iec61850_server_write_items( args, error );
    } else if (strcmp(method, "read_items") == 0) {
        response = iec61850_server_read_items( args, error );
    } else {
        *error = "invalid method";
    }

    return response;
}

//-----------------------------------------------------
// Catch signals
//-----------------------------------------------------
static void stopHandler(int sig) {
    // LOGINFO("received ctrl-c");
    stop();
}

//-----------------------------------------------------
// The Entry Point
//-----------------------------------------------------
int main(int argc, char** argv) {

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    // LOGINFO("enter eport_loop");
    // eport_loop( &on_request );
    
    stop();

    return EXIT_SUCCESS;
}