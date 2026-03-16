#ifndef mms_client_loop__h
#define mms_client_loop__h


// #include <eport_c.h>

// --- Data Structures ---
typedef struct {
    char *name;
    bool deletable;
    char **members; // NULL-terminated array of strings
} BrowseDataSet;

typedef struct {
    char *ln_name;
    char **data_objects; // NULL-terminated array of strings
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