/*----------------------------------------------------------------
* IEC 61850 MMS Client Wrapper
* Logic mirrors the provided OPC UA client structure.
* Uses libiec61850 and cJSON.
----------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <eport_c.h>

// libiec61850 includes
#include "iec61850_client.h"
#include "hal_thread.h"

#include "iec61850_mms_client_loop.h"
#include "utils.h"

//-----------------------------------------------------
// API Implementation
//-----------------------------------------------------

// Emulates "browse_servers" - In 61850 we usually just connect to IP.
// We can use this to return the configured endpoint.
static cJSON* iec61850_client_info(cJSON* args, char **error) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "protocol", "IEC 61850 MMS");
    cJSON_AddStringToObject(response, "default_port", "102");
    return response;
}

static cJSON* iec61850_client_connect(cJSON* args, char **error) 
{
    if (is_connected()) 
    {
        *error = "already connected";
        goto on_error;
    }

    if (!cJSON_IsObject(args)) 
    {
        *error = "invalid parameters";
        goto on_error;
    }

    cJSON *host = cJSON_GetObjectItemCaseSensitive(args, "host");
    if (!cJSON_IsString(host) || (host->valuestring == NULL)) 
    {
        *error = "host is not defined";
        return NULL;
    }

    cJSON *port = cJSON_GetObjectItemCaseSensitive(args, "port");

    int portNum = 102;

    if (cJSON_IsNumber(port)) 
    {
        portNum = port->valueint;
    }

    char *_password = NULL;
 
    cJSON *password = cJSON_GetObjectItemCaseSensitive(args, "password");

    if (cJSON_IsString(password) && (password->valuestring != NULL)){
        _password = password->valuestring;
    }

    *error = start(
        host->valuestring,
        portNum,
        _password
    );
    if (*error) goto on_error;

    return cJSON_CreateString("ok");

on_error:
    return NULL;
}

static cJSON* iec61850_client_read_items(cJSON* args, char **error) 
{
    if (!is_connected()) 
    {
        *error = "no connection";
        return NULL;
    }

    cJSON *result = cJSON_CreateObject();
    cJSON *item = NULL;

    cJSON_ArrayForEach(item, args) 
    {

        const char *path = item->string; 
        
        cJSON *typeItem  = cJSON_GetObjectItem(item, "fcType");

        MmsValue* value = read_value(path, typeItem->valuestring);

        if (MmsValue_getType(value) == MMS_VISIBLE_STRING) {
            // error
            // printf("MMS value MMS_VISIBLE_STRING");
            cJSON_AddItemToObject(result, path, cJSON_CreateString( MmsValue_toString(value) ));
        } else {
            cJSON_AddItemToObject(result, path, mms2json(value));
        }

        MmsValue_delete(value);
    }

    return result;
}

static cJSON* iec61850_client_write_items(cJSON* args, char **error) 
{
    if (!is_connected()) 
    {
        *error = "no connection";
        return NULL;
    }

    if (!cJSON_IsObject(args))
    {
        *error = "invalid write arguments (expected object)";
        return NULL;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON *item = NULL;

    cJSON_ArrayForEach(item, args) {

        const char *path = item->string;

        cJSON *fcType  = cJSON_GetObjectItem(item, "fcType");
        cJSON *type = cJSON_GetObjectItem(item, "type");
        cJSON *val = cJSON_GetObjectItem(item, "value");

        /* Convert JSON value -> MmsValue (segregated in utils.c) */
        MmsValue* mmsVal = json2mms(type->valuestring, val);

        if (!mmsVal) {
            cJSON_AddStringToObject(response, path, "conversion failed");
            continue;
        }

        /* Perform the MMS write via helper in the loop module */
        IedClientError writeErr = write_value(path, fcType->valuestring, mmsVal);

        MmsValue_delete(mmsVal);

        if (writeErr == IED_ERROR_OK) {
            cJSON_AddStringToObject(response, path, "ok");
        } else {
            char errbuf[64];
            snprintf(errbuf, sizeof(errbuf), "write error %d", (int)writeErr);
            cJSON_AddStringToObject(response, path, errbuf);
        }
    }

    return response;
}

static const char*
browse_node_kind_to_string(BrowseNodeKind kind)
{
    switch (kind) {
    case BROWSE_NODE_DATA_OBJECT:
        return "data_object";
    case BROWSE_NODE_COMPONENT:
        return "component";
    case BROWSE_NODE_DATA_ATTRIBUTE:
        return "data_attribute";
    default:
        return "unknown";
    }
}

static cJSON*
browse_model_node_to_json(BrowseModelNode* node)
{
    cJSON* node_obj = cJSON_CreateObject();
    cJSON* children = cJSON_CreateArray();

    if ((node_obj == NULL) || (children == NULL)) {
        if (node_obj) cJSON_Delete(node_obj);
        if (children) cJSON_Delete(children);
        return NULL;
    }

    cJSON_AddStringToObject(node_obj, "name", node->name);
    cJSON_AddStringToObject(node_obj, "kind", browse_node_kind_to_string(node->kind));
    cJSON_AddStringToObject(node_obj, "object_reference", node->object_reference);
    cJSON_AddStringToObject(node_obj, "tree_path", node->tree_path);

    if (node->fc != NULL)
        cJSON_AddStringToObject(node_obj, "fc", node->fc);

    if (node->mms_type != NULL)
        cJSON_AddStringToObject(node_obj, "mms_type", node->mms_type);

    if (node->size >= 0)
        cJSON_AddNumberToObject(node_obj, "size", node->size);

    for (int i = 0; node->children && node->children[i]; i++) {
        cJSON* child_obj = browse_model_node_to_json(node->children[i]);
        if (child_obj == NULL) {
            cJSON_Delete(node_obj);
            cJSON_Delete(children);
            return NULL;
        }

        cJSON_AddItemToArray(children, child_obj);
    }

    cJSON_AddItemToObject(node_obj, "children", children);

    return node_obj;
}

static cJSON*
browse_model_tree_to_json(BrowseModelNode** nodes)
{
    cJSON* tree = cJSON_CreateArray();

    if (tree == NULL)
        return NULL;

    for (int i = 0; nodes && nodes[i]; i++) {
        cJSON* node_obj = browse_model_node_to_json(nodes[i]);
        if (node_obj == NULL) {
            cJSON_Delete(tree);
            return NULL;
        }

        cJSON_AddItemToArray(tree, node_obj);
    }

    return tree;
}

static cJSON* iec61850_client_browse(cJSON* args, char **error) 
{
    cJSON *response = NULL;
    BrowseLD **devices = NULL;

    if ( !cJSON_IsObject(args) ) 
    {
        *error = "invalid parameters";
        goto on_clear;
    }

    cJSON *host = cJSON_GetObjectItemCaseSensitive(args, "host");
    if (!cJSON_IsString(host) || (host->valuestring == NULL))
    {
        *error = "host is not defined";
        goto on_clear; 
    }

    cJSON *port = cJSON_GetObjectItemCaseSensitive(args, "port");
    if (!cJSON_IsNumber(port))
    {
        *error = "port is not defined";
        goto on_clear; 
    }

    // Call Core Logic
    *error = browse_server(host->valuestring, port->valueint, &devices);
    if (*error) goto on_clear;

    // Build the JSON Response
    response = cJSON_CreateArray();
    if (!response) 
    {
        *error = "unable to allocate CJSON object for response";
        goto on_clear;
    }

    for (int i = 0; devices && devices[i]; i++) 
    {
        cJSON *ld_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(ld_obj, "ld_name", devices[i]->ld_name);
        
        cJSON *ln_array = cJSON_CreateArray();
        cJSON_AddItemToObject(ld_obj, "logical_nodes", ln_array);
        
        for (int j = 0; devices[i]->logical_nodes && devices[i]->logical_nodes[j]; j++) {
            BrowseLN *ln = devices[i]->logical_nodes[j];
            cJSON *ln_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(ln_obj, "ln_name", ln->ln_name);
            
            // Add Data Objects
            cJSON *do_array = cJSON_CreateArray();
            for (int k = 0; ln->data_objects && ln->data_objects[k]; k++) 
                cJSON_AddItemToArray(do_array, cJSON_CreateString(ln->data_objects[k]));
            cJSON_AddItemToObject(ln_obj, "data_objects", do_array);

            cJSON *tree_array = browse_model_tree_to_json(ln->data_object_tree);
            if (tree_array == NULL) {
                *error = "unable to allocate browse tree JSON";
                cJSON_Delete(ln_obj);
                goto on_clear;
            }
            cJSON_AddItemToObject(ln_obj, "data_object_tree", tree_array);

            // Add Data Sets
            cJSON *ds_array = cJSON_CreateArray();
            for (int k = 0; ln->data_sets && ln->data_sets[k]; k++) {
                cJSON *ds_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(ds_obj, "name", ln->data_sets[k]->name);
                cJSON_AddBoolToObject(ds_obj, "deletable", ln->data_sets[k]->deletable);
                
                cJSON *members_array = cJSON_CreateArray();
                for (int m = 0; ln->data_sets[k]->members && ln->data_sets[k]->members[m]; m++)
                    cJSON_AddItemToArray(members_array, cJSON_CreateString(ln->data_sets[k]->members[m]));
                cJSON_AddItemToObject(ds_obj, "members", members_array);
                cJSON_AddItemToArray(ds_array, ds_obj);
            }
            cJSON_AddItemToObject(ln_obj, "data_sets", ds_array);

            // Add Reports
            cJSON *urcb_array = cJSON_CreateArray();
            for (int k = 0; ln->urcbs && ln->urcbs[k]; k++) cJSON_AddItemToArray(urcb_array, cJSON_CreateString(ln->urcbs[k]));
            cJSON_AddItemToObject(ln_obj, "urcb", urcb_array);

            cJSON_AddItemToArray(ln_array, ln_obj);
        }
        cJSON_AddItemToArray(response, ld_obj);
    }

on_clear:
    if (devices) free_browse_results(devices);

    if (!*error) return response;

    if (response) cJSON_Delete(response);
    return NULL;
}

//-----------------------------------------------------
// Request Router (Matches your 'on_request')
//-----------------------------------------------------
static cJSON* on_request(char *method, cJSON *args, char **error) {
    cJSON *response = NULL;
    
    if (strcmp(method, "info") == 0) {
        response = iec61850_client_info(args, error);
    } else if (strcmp(method, "connect") == 0) {
        response = iec61850_client_connect(args, error);
    } else if (strcmp(method, "read_items") == 0) {
        response = iec61850_client_read_items(args, error);
    } else if (strcmp(method, "write_items") == 0) {
        response = iec61850_client_write_items(args, error);
    } else if (strcmp(method, "browse") == 0) {
        response = iec61850_client_browse(args, error);
    } else {
        *error = "invalid method";
    }

    return response;
}

//-----------------------------------------------------
// Main Entry
//-----------------------------------------------------
#ifndef IEC61850_MMS_CLIENT_DISABLE_MAIN
int main(int argc, char *argv[]) {
    LOGINFO("enter eport_loop");

    eport_loop( &on_request );

    return EXIT_SUCCESS;
}
#endif
