#ifndef mms_client_loop__h
#define mms_client_loop__h

#include <eport_c.h>

// --- Data Structures ---
typedef struct {
    char *name;
    bool deletable;
    char **members; // NULL-terminated array of strings
} BrowseDataSet;

typedef enum {
    BROWSE_NODE_DATA_OBJECT = 0,
    BROWSE_NODE_COMPONENT = 1,
    BROWSE_NODE_DATA_ATTRIBUTE = 2
} BrowseNodeKind;

typedef struct sBrowseModelNode {
    char *name;
    char *object_reference;
    char *tree_path;
    char *fc;
    char *mms_type;
    int size;
    BrowseNodeKind kind;
    struct sBrowseModelNode **children; // NULL-terminated array of structs
} BrowseModelNode;

typedef struct {
    char *ln_name;
    char **data_objects; // NULL-terminated array of strings
    BrowseModelNode **data_object_tree; // NULL-terminated array of structs
    BrowseDataSet **data_sets; // NULL-terminated array of structs
    char **urcbs; // NULL-terminated array of strings
    char **brcbs; // NULL-terminated array of strings
} BrowseLN;

typedef struct {
    char *ld_name;
    BrowseLN **logical_nodes; // NULL-terminated array of structs
} BrowseLD;

char *start(char *host, int port, const char *password);
bool is_connected(void);
MmsValue* read_value(const char *path, const char *fcType);
IedClientError write_value(const char *path, const char *fcType, MmsValue* value);

// --- Function Prototypes ---
char* browse_server(char *host, int port, BrowseLD ***devices_out);
void free_browse_results(BrowseLD **devices);

#endif
