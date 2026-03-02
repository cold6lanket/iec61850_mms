#ifndef IEC61850_MMS_SERVER_LOOP_H
#define IEC61850_MMS_SERVER_LOOP_H

#include "cJSON.h"

// Core Functions
char* start(int tcpPort);
void stop(void);

char* write_server_item(const char *pathStr, const char *typeStr, cJSON *valueJson);
char* read_server_item(const char *pathStr, cJSON **outJson);

#endif