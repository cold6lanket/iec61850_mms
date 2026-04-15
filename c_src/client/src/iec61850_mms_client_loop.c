#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "iec61850_client.h"
#include "mms_type_spec.h"
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

    static char error_message[128]; 
    
    snprintf(error_message, sizeof(error_message), "Failed to connect (error code): %d", (int)error);

    return error_message;
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

static int
string_array_length(char** values)
{
    int count = 0;

    if (values == NULL)
        return 0;

    while (values[count] != NULL)
        count++;

    return count;
}

static int
linked_list_length(LinkedList list)
{
    int count = 0;
    LinkedList item;

    if (list == NULL)
        return 0;

    item = LinkedList_getNext(list);

    while (item != NULL) {
        count++;
        item = LinkedList_getNext(item);
    }

    return count;
}

static char*
string_dup_printf(const char* format, ...)
{
    va_list args;
    va_list copy;
    int length;
    char* buffer;

    va_start(args, format);
    va_copy(copy, args);
    length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);

    if (length < 0) {
        va_end(args);
        return NULL;
    }

    buffer = (char*) calloc((size_t) length + 1, sizeof(char));

    if (buffer != NULL)
        vsnprintf(buffer, (size_t) length + 1, format, args);

    va_end(args);

    return buffer;
}

static const char*
mms_type_to_string(MmsType type)
{
    switch (type) {
    case MMS_ARRAY:
        return "ARRAY";
    case MMS_STRUCTURE:
        return "STRUCTURE";
    case MMS_BOOLEAN:
        return "BOOLEAN";
    case MMS_BIT_STRING:
        return "BIT_STRING";
    case MMS_INTEGER:
        return "INTEGER";
    case MMS_UNSIGNED:
        return "UNSIGNED";
    case MMS_FLOAT:
        return "FLOAT";
    case MMS_OCTET_STRING:
        return "OCTET_STRING";
    case MMS_VISIBLE_STRING:
        return "VISIBLE_STRING";
    case MMS_GENERALIZED_TIME:
        return "GENERALIZED_TIME";
    case MMS_BINARY_TIME:
        return "BINARY_TIME";
    case MMS_BCD:
        return "BCD";
    case MMS_OBJ_ID:
        return "OBJ_ID";
    case MMS_STRING:
        return "STRING";
    case MMS_UTC_TIME:
        return "UTC_TIME";
    case MMS_DATA_ACCESS_ERROR:
        return "DATA_ACCESS_ERROR";
    default:
        return "UNKNOWN";
    }
}

static bool
parse_name_with_fc(const char* raw_name, char** name_out, char** fc_out)
{
    size_t length;

    if ((raw_name == NULL) || (name_out == NULL) || (fc_out == NULL))
        return false;

    *name_out = NULL;
    *fc_out = NULL;

    length = strlen(raw_name);

    if ((length < 4) || (raw_name[length - 4] != '[') || (raw_name[length - 1] != ']'))
        return false;

    *name_out = (char*) calloc(length - 3, sizeof(char));
    *fc_out = (char*) calloc(3, sizeof(char));

    if ((*name_out == NULL) || (*fc_out == NULL)) {
        free(*name_out);
        free(*fc_out);
        *name_out = NULL;
        *fc_out = NULL;
        return false;
    }

    memcpy(*name_out, raw_name, length - 4);
    (*name_out)[length - 4] = 0;

    memcpy(*fc_out, raw_name + length - 3, 2);
    (*fc_out)[2] = 0;

    return true;
}

static void
free_browse_model_nodes(BrowseModelNode** nodes)
{
    if (nodes == NULL)
        return;

    for (int i = 0; nodes[i] != NULL; i++) {
        BrowseModelNode* node = nodes[i];

        free(node->name);
        free(node->object_reference);
        free(node->tree_path);
        free(node->fc);
        free(node->mms_type);
        free_browse_model_nodes(node->children);
        free(node);
    }

    free(nodes);
}

static void
free_browse_model_node(BrowseModelNode* node)
{
    if (node == NULL)
        return;

    free(node->name);
    free(node->object_reference);
    free(node->tree_path);
    free(node->fc);
    free(node->mms_type);
    free_browse_model_nodes(node->children);
    free(node);
}

static void
populate_model_node_metadata(IedConnection connection, BrowseModelNode* node)
{
    FunctionalConstraint fc;
    IedClientError error;
    MmsVariableSpecification* spec;

    if ((node == NULL) || (node->fc == NULL))
        return;

    fc = get_fc_type(node->fc);

    if ((int) fc == -1)
        return;

    spec = IedConnection_getVariableSpecification(connection, &error, node->object_reference, fc);

    if ((error != IED_ERROR_OK) || (spec == NULL))
        return;

    free(node->mms_type);
    node->mms_type = strdup(mms_type_to_string(MmsVariableSpecification_getType(spec)));
    node->size = MmsVariableSpecification_getSize(spec);

    MmsVariableSpecification_destroy(spec);
}

static BrowseModelNode*
create_model_node(const char* name, const char* object_reference, const char* tree_path,
        const char* fc, BrowseNodeKind kind)
{
    BrowseModelNode* node = (BrowseModelNode*) calloc(1, sizeof(BrowseModelNode));

    if (node == NULL)
        return NULL;

    node->name = strdup(name);
    node->object_reference = strdup(object_reference);
    node->tree_path = strdup(tree_path);
    node->fc = (fc != NULL) ? strdup(fc) : NULL;
    node->size = -1;
    node->kind = kind;

    if ((node->name == NULL) || (node->object_reference == NULL) || (node->tree_path == NULL) ||
            ((fc != NULL) && (node->fc == NULL))) {
        free(node->name);
        free(node->object_reference);
        free(node->tree_path);
        free(node->fc);
        free(node);
        return NULL;
    }

    return node;
}

static BrowseModelNode**
build_fc_subtree(IedConnection connection, const char* object_reference, const char* ln_name,
        const char* relative_path, const char* fc, char** error);

static BrowseModelNode**
build_do_children(IedConnection connection, const char* object_reference, const char* ln_name,
        const char* relative_path, char** error)
{
    IedClientError ied_error;
    LinkedList child_list = IedConnection_getDataDirectoryFC(connection, &ied_error, object_reference);
    BrowseModelNode** children = NULL;
    int child_count = 0;
    int index = 0;

    if (ied_error != IED_ERROR_OK) {
        *error = "failed to browse data object";
        return NULL;
    }

    if (child_list == NULL)
        return NULL;

    child_count = linked_list_length(child_list);
    if (child_count == 0) {
        LinkedList_destroy(child_list);
        return NULL;
    }

    children = (BrowseModelNode**) calloc((size_t) child_count + 1, sizeof(BrowseModelNode*));
    if (children == NULL) {
        LinkedList_destroy(child_list);
        *error = "out of memory";
        return NULL;
    }

    LinkedList child = LinkedList_getNext(child_list);
    while (child != NULL) {
        char* raw_name = (char*) child->data;
        char* child_name = NULL;
        char* child_fc = NULL;
        char* child_reference = NULL;
        char* child_relative_path = NULL;
        char* child_tree_path = NULL;
        BrowseModelNode* child_node = NULL;

        if (!parse_name_with_fc(raw_name, &child_name, &child_fc)) {
            *error = "invalid data directory entry";
            goto on_error;
        }

        child_reference = string_dup_printf("%s.%s", object_reference, child_name);
        child_relative_path = string_dup_printf("%s.%s", relative_path, child_name);
        child_tree_path = string_dup_printf("%s.%s.%s", ln_name, child_fc, child_relative_path);

        if ((child_reference == NULL) || (child_relative_path == NULL) || (child_tree_path == NULL)) {
            *error = "out of memory";
            goto on_error;
        }

        child_node = create_model_node(child_name, child_reference, child_tree_path, child_fc,
                BROWSE_NODE_DATA_ATTRIBUTE);

        if (child_node == NULL) {
            *error = "out of memory";
            goto on_error;
        }

        child_node->children = build_fc_subtree(connection, child_reference, ln_name, child_relative_path, child_fc, error);
        if (*error != NULL) {
            free_browse_model_node(child_node);
            goto on_error;
        }

        if (child_node->children != NULL)
            child_node->kind = BROWSE_NODE_COMPONENT;

        populate_model_node_metadata(connection, child_node);

        children[index++] = child_node;

        free(child_name);
        free(child_fc);
        free(child_reference);
        free(child_relative_path);
        free(child_tree_path);

        child = LinkedList_getNext(child);
        continue;

on_error:
        free(child_name);
        free(child_fc);
        free(child_reference);
        free(child_relative_path);
        free(child_tree_path);
        free_browse_model_nodes(children);
        LinkedList_destroy(child_list);
        return NULL;
    }

    LinkedList_destroy(child_list);
    return children;
}

static BrowseModelNode**
build_fc_subtree(IedConnection connection, const char* object_reference, const char* ln_name,
        const char* relative_path, const char* fc, char** error)
{
    FunctionalConstraint functional_constraint = get_fc_type(fc);
    IedClientError ied_error;
    LinkedList child_list;
    BrowseModelNode** children = NULL;
    int child_count = 0;
    int index = 0;

    if ((int) functional_constraint == -1) {
        *error = "invalid functional constraint";
        return NULL;
    }

    child_list = IedConnection_getDataDirectoryByFC(connection, &ied_error, object_reference, functional_constraint);

    if (ied_error != IED_ERROR_OK) {
        *error = "failed to browse data attribute subtree";
        return NULL;
    }

    if (child_list == NULL)
        return NULL;

    LinkedList child = LinkedList_getNext(child_list);
    while (child != NULL) {
        child_count++;
        child = LinkedList_getNext(child);
    }

    if (child_count == 0) {
        LinkedList_destroy(child_list);
        return NULL;
    }

    children = (BrowseModelNode**) calloc((size_t) child_count + 1, sizeof(BrowseModelNode*));
    if (children == NULL) {
        LinkedList_destroy(child_list);
        *error = "out of memory";
        return NULL;
    }

    child = LinkedList_getNext(child_list);
    while (child != NULL) {
        char* child_name = (char*) child->data;
        char* child_reference = string_dup_printf("%s.%s", object_reference, child_name);
        char* child_relative_path = string_dup_printf("%s.%s", relative_path, child_name);
        char* child_tree_path = string_dup_printf("%s.%s.%s", ln_name, fc, child_relative_path);
        BrowseModelNode* child_node = NULL;

        if ((child_reference == NULL) || (child_relative_path == NULL) || (child_tree_path == NULL)) {
            *error = "out of memory";
            free(child_reference);
            free(child_relative_path);
            free(child_tree_path);
            free_browse_model_nodes(children);
            LinkedList_destroy(child_list);
            return NULL;
        }

        child_node = create_model_node(child_name, child_reference, child_tree_path, fc, BROWSE_NODE_DATA_ATTRIBUTE);
        if (child_node == NULL) {
            *error = "out of memory";
            free(child_reference);
            free(child_relative_path);
            free(child_tree_path);
            free_browse_model_nodes(children);
            LinkedList_destroy(child_list);
            return NULL;
        }

        child_node->children = build_fc_subtree(connection, child_reference, ln_name, child_relative_path, fc, error);
        if (*error != NULL) {
            free(child_reference);
            free(child_relative_path);
            free(child_tree_path);
            free_browse_model_node(child_node);
            free_browse_model_nodes(children);
            LinkedList_destroy(child_list);
            return NULL;
        }

        if (child_node->children != NULL)
            child_node->kind = BROWSE_NODE_COMPONENT;

        populate_model_node_metadata(connection, child_node);

        children[index++] = child_node;
        free(child_reference);
        free(child_relative_path);
        free(child_tree_path);
        child = LinkedList_getNext(child);
    }

    LinkedList_destroy(child_list);
    return children;
}

static BrowseModelNode**
build_data_object_tree(IedConnection connection, const char* ld_name, const char* ln_name, char** data_objects,
        char** error)
{
    int do_count = string_array_length(data_objects);
    BrowseModelNode** roots = NULL;

    if (do_count == 0)
        return NULL;

    roots = (BrowseModelNode**) calloc((size_t) do_count + 1, sizeof(BrowseModelNode*));
    if (roots == NULL) {
        *error = "out of memory";
        return NULL;
    }

    for (int i = 0; i < do_count; i++) {
        char* do_reference = string_dup_printf("%s/%s.%s", ld_name, ln_name, data_objects[i]);
        char* tree_path = string_dup_printf("%s.%s", ln_name, data_objects[i]);

        if ((do_reference == NULL) || (tree_path == NULL)) {
            free(do_reference);
            free(tree_path);
            free_browse_model_nodes(roots);
            *error = "out of memory";
            return NULL;
        }

        roots[i] = create_model_node(data_objects[i], do_reference, tree_path, NULL, BROWSE_NODE_DATA_OBJECT);

        free(do_reference);
        free(tree_path);

        if (roots[i] == NULL) {
            free_browse_model_nodes(roots);
            *error = "out of memory";
            return NULL;
        }

        roots[i]->children = build_do_children(connection, roots[i]->object_reference, ln_name, roots[i]->name, error);
        if (*error != NULL) {
            free_browse_model_nodes(roots);
            return NULL;
        }
    }

    return roots;
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

            if (err != IED_ERROR_OK) {
                error = "failed to browse logical node data objects";
                LinkedList_destroy(logicalNodes);
                goto on_clear;
            }

            ln_struct->data_object_tree = build_data_object_tree(connection, ldName, lnName, ln_struct->data_objects, &error);
            if (error != NULL) {
                LinkedList_destroy(logicalNodes);
                goto on_clear;
            }

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
                if (ln->data_object_tree) free_browse_model_nodes(ln->data_object_tree);
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
