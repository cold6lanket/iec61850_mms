#include <stdio.h>
#include <string.h>

#include "iec61850_client.h"
#include "iec61850_mms_client_loop.h"
#include "utils.h"


struct MMS_CLIENT {
  IedConnection connection;
  bool is_connected;
  int port_cache;
  char* hostname_cache;
} mms_client;

char *start(char *host, int port, const char *password)
{
    IedClientError error;

    mms_client.connection = IedConnection_create();

    AcseAuthenticationParameter auth = NULL;

    if (password != NULL && strlen(password) > 0) {
        MmsConnection mmsConnection = IedConnection_getMmsConnection(mms_client.connection);
        IsoConnectionParameters parameters = MmsConnection_getIsoConnectionParameters(mmsConnection);

        auth = AcseAuthenticationParameter_create();
        AcseAuthenticationParameter_setAuthMechanism(auth, ACSE_AUTH_PASSWORD);
        AcseAuthenticationParameter_setPassword(auth, password);

        IsoConnectionParameters_setAcseAuthenticationParameter(parameters, auth);

        IedConnection_setConnectTimeout(mms_client.connection, 10000);
    }

    IedConnection_connect(mms_client.connection, &error, host, port);

    if (error != IED_ERROR_OK) {
       goto on_error;
    }

    mms_client.is_connected = true;
    
    if (mms_client.hostname_cache) {
        free(mms_client.hostname_cache);
    }
    mms_client.hostname_cache = strdup(host);
    mms_client.port_cache = port;

    return NULL;

on_error:
    // IedConnection_destroy handles the cleanup of the attached 'auth' parameter,
    // so we ONLY destroy 'auth' manually if we created it but never attached it 
    // to the connection (which shouldn't happen here, but good practice).
    if (mms_client.connection != NULL) {
        IedConnection_destroy(mms_client.connection);
        mms_client.connection = NULL;
    }
    
    mms_client.is_connected = false;
    return "Failed to connect (error code)";
}

bool is_connected()
{
    return mms_client.is_connected;
}

MmsValue* read_value(const char *path, const char *fcType) 
{
    IedClientError err; 
    
    FunctionalConstraint fc = get_fc_type(fcType);
    if ((int)fc == -1) 
    {
        // Return an MMS String with the error
        return MmsValue_newVisibleString("ERROR: Invalid Functional Constraint");
    }

    if (!is_connected()) 
    {
        return MmsValue_newVisibleString("ERROR: Not connected to IED");
    }

    MmsValue* value = IedConnection_readObject(mms_client.connection, &err, path, fc); 


    // Check for success
    if (err == IED_ERROR_OK && value != NULL) 
    {
        // SUCCESS: Return the actual value we read from the server.
        // DO NOT delete it here!
        return value; 
    } 
    else 
    {
        // FAILURE: Create a custom error string
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "ERROR: Read failed with error code %d", (int)err);
        
        // If value was somehow allocated during a fail, clean it up
        if (value != NULL) {
            MmsValue_delete(value);
        }

        // Return the error message as an MMS String
        return MmsValue_newVisibleString(error_msg);
    }
}

IedClientError write_value(const char *path, const char *fcType, MmsValue* value)
{
    IedClientError err;

    FunctionalConstraint fc = get_fc_type(fcType);
   
    if (!is_connected()) {
        return IED_ERROR_CONNECTION_LOST;
    }

    IedConnection_writeObject(mms_client.connection, &err, path, fc, value);

    return err;
}

// Helper: Converts a LinkedList of strings to a NULL-terminated char**
static char** list_to_array(LinkedList list) {
    if (!list) return NULL;
    int count = 0;
    LinkedList item = LinkedList_getNext(list);
    while (item) { count++; item = LinkedList_getNext(item); }

    char **arr = calloc(count + 1, sizeof(char*));
    if (!arr) return NULL;

    int i = 0;
    item = LinkedList_getNext(list);
    while (item) {
        arr[i++] = strdup((char*)item->data);
        item = LinkedList_getNext(item);
    }
    return arr; // arr[count] is NULL
}

char* browse_server(char *host, int port, BrowseLD ***devices_out) {
    char *error = NULL;
    
    BrowseLD **result_devices = NULL;

    IedClientError err;
    IedConnection connection = IedConnection_create();

    LinkedList deviceList = NULL;

    IedConnection_connect(connection, &err, host, port);

    if (err != IED_ERROR_OK) {
        error = "failed to connect";
        goto on_clear;
    }

    deviceList = IedConnection_getLogicalDeviceList(connection, &err);
    if (err != IED_ERROR_OK) {
        error = "failed to browse devices";
        goto on_clear;
    }

    // Count LDs
    int ld_count = 0;
    LinkedList item = LinkedList_getNext(deviceList);
    while (item) { ld_count++; item = LinkedList_getNext(item); }

    result_devices = calloc(ld_count + 1, sizeof(BrowseLD*));
    if (!result_devices) { error = "out of memory"; goto on_clear; }

    int ld_idx = 0;
    LinkedList device = LinkedList_getNext(deviceList);

    while (device != NULL) {
        char* ldName = (char*) device->data;
        BrowseLD *ld_struct = calloc(1, sizeof(BrowseLD));
        ld_struct->ld_name = strdup(ldName);
        result_devices[ld_idx++] = ld_struct;

        LinkedList logicalNodes = IedConnection_getLogicalDeviceDirectory(connection, &err, ldName);
        
        // Count LNs
        int ln_count = 0;
        item = LinkedList_getNext(logicalNodes);
        while (item) { ln_count++; item = LinkedList_getNext(item); }
        
        ld_struct->logical_nodes = calloc(ln_count + 1, sizeof(BrowseLN*));

        int ln_idx = 0;
        LinkedList logicalNode = LinkedList_getNext(logicalNodes);

        while (logicalNode != NULL) {
            char* lnName = (char*) logicalNode->data;
            BrowseLN *ln_struct = calloc(1, sizeof(BrowseLN));
            ln_struct->ln_name = strdup(lnName);
            ld_struct->logical_nodes[ln_idx++] = ln_struct;

            char lnRef[129];
            snprintf(lnRef, sizeof(lnRef), "%s/%s", ldName, lnName);

            // Fetch Data Objects
            LinkedList doList = IedConnection_getLogicalNodeDirectory(connection, &err, lnRef, ACSI_CLASS_DATA_OBJECT);
            ln_struct->data_objects = list_to_array(doList);
            LinkedList_destroy(doList);

            // Fetch URCB / BRCB
            LinkedList urcbList = IedConnection_getLogicalNodeDirectory(connection, &err, lnRef, ACSI_CLASS_URCB);
            ln_struct->urcbs = list_to_array(urcbList);
            LinkedList_destroy(urcbList);

            LinkedList brcbList = IedConnection_getLogicalNodeDirectory(connection, &err, lnRef, ACSI_CLASS_BRCB);
            ln_struct->brcbs = list_to_array(brcbList);
            LinkedList_destroy(brcbList);

            // Fetch Data Sets
            LinkedList dsList = IedConnection_getLogicalNodeDirectory(connection, &err, lnRef, ACSI_CLASS_DATA_SET);
            int ds_count = 0;
            item = LinkedList_getNext(dsList);
            while (item) { ds_count++; item = LinkedList_getNext(item); }

            ln_struct->data_sets = calloc(ds_count + 1, sizeof(BrowseDataSet*));
            int ds_idx = 0;
            LinkedList dataSet = LinkedList_getNext(dsList);

            while (dataSet != NULL) {
                BrowseDataSet *ds_struct = calloc(1, sizeof(BrowseDataSet));
                ds_struct->name = strdup((char*) dataSet->data);
                
                char dsRef[130];
                snprintf(dsRef, sizeof(dsRef), "%s.%s", lnRef, ds_struct->name);

                LinkedList dsMembers = IedConnection_getDataSetDirectory(connection, &err, dsRef, &ds_struct->deletable);
                ds_struct->members = list_to_array(dsMembers);
                LinkedList_destroy(dsMembers);

                ln_struct->data_sets[ds_idx++] = ds_struct;
                dataSet = LinkedList_getNext(dataSet);
            }
            LinkedList_destroy(dsList);
            logicalNode = LinkedList_getNext(logicalNode);
        }
        LinkedList_destroy(logicalNodes);
        device = LinkedList_getNext(device);
    }

on_clear:
    if (deviceList) LinkedList_destroy(deviceList);

    IedConnection_destroy(connection);

    if (!error) {
        *devices_out = result_devices;
        return NULL; // Success
    }

    // If error occurred partway through, clean up what we built
    if (result_devices) free_browse_results(result_devices);
    return error;
}

void free_browse_results(BrowseLD **devices) {
    if (!devices) return;
    for (int i = 0; devices[i]; i++) {
        free(devices[i]->ld_name);
        if (devices[i]->logical_nodes) {
            for (int j = 0; devices[i]->logical_nodes[j]; j++) {
                BrowseLN *ln = devices[i]->logical_nodes[j];
                free(ln->ln_name);
                if (ln->data_objects) { for(int k=0; ln->data_objects[k]; k++) free(ln->data_objects[k]); free(ln->data_objects); }
                if (ln->urcbs) { for(int k=0; ln->urcbs[k]; k++) free(ln->urcbs[k]); free(ln->urcbs); }
                if (ln->brcbs) { for(int k=0; ln->brcbs[k]; k++) free(ln->brcbs[k]); free(ln->brcbs); }
                if (ln->data_sets) {
                    for(int k=0; ln->data_sets[k]; k++) {
                        free(ln->data_sets[k]->name);
                        if (ln->data_sets[k]->members) {
                            for(int m=0; ln->data_sets[k]->members[m]; m++) free(ln->data_sets[k]->members[m]);
                            free(ln->data_sets[k]->members);
                        }
                        free(ln->data_sets[k]);
                    }
                    free(ln->data_sets);
                }
                free(ln);
            }
            free(devices[i]->logical_nodes);
        }
        free(devices[i]);
    }
    free(devices);
}