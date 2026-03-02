#include "iec61850_mms_server_loop.h"
#include "iec61850_server.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

extern IedModel iedModel;
static IedServer iecServer = NULL;
static IedModel* iecModel = NULL;

char* start(int tcpPort) {
    if (iecServer) return "server already running";

    iecModel = &iedModel; 
    iecServer = IedServer_create(iecModel);
    
    if (!iecServer) return "failed to allocate server";

    IedServer_start(iecServer, tcpPort);

    if (!IedServer_isRunning(iecServer)) {
        IedServer_destroy(iecServer);
        iecServer = NULL;
        return "Failed to start server on specified port";
    }
    return NULL; 
}

void stop(void) {
    if (iecServer) {
        IedServer_stop(iecServer);
        IedServer_destroy(iecServer);
        iecServer = NULL;
    }
}

char* write_server_item(const char *pathStr, const char *typeStr, cJSON *valueJson) {
    if (!iecServer) return "server not started";

    ModelNode* node = IedModel_getModelNodeByObjectReference(iecModel, pathStr);
    if (node == NULL) return "node not found in static model";
    // if (ModelNode_getType(node) != DATA_ATTRIBUTE) return "path is not a leaf data attribute";

    MmsValue* mmsValue = json2mms(typeStr, valueJson);
    if (!mmsValue) return "unsupported or mismatched type conversion";

    // 2. Safely write to the server
    IedServer_lockDataModel(iecServer);
    IedServer_updateAttributeValue(iecServer, (DataAttribute*)node, mmsValue);
    IedServer_unlockDataModel(iecServer);
    
    MmsValue_delete(mmsValue);
    return NULL;
}

char* read_server_item(const char *pathStr, cJSON **outJson) {
    if (!iecServer) return "server not started";

    ModelNode* node = IedModel_getModelNodeByObjectReference(iecModel, pathStr);
    if (node == NULL) return "node not found in static model";
    // if (ModelNode_getType(node) != DATA_ATTRIBUTE) return "path is not a data attribute";

    IedServer_lockDataModel(iecServer);
    MmsValue* val = IedServer_getAttributeValue(iecServer, (DataAttribute*)node);
    
    if (val == NULL) {
        IedServer_unlockDataModel(iecServer);
        return "internal value is null";
    }

    *outJson = mms2json(val);
    
    IedServer_unlockDataModel(iecServer);

    if (*outJson == NULL) return "failed to convert MMS to JSON";
    return NULL;
}