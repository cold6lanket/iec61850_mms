#ifndef mms_utilities__h
#define mms_utilities__h

#include "mms_value.h"
#include "iec61850_common.h"
#include "iec61850_client.h"
#include "cJSON.h"

cJSON* mms2json(MmsValue* value);
MmsValue *json2mms(const char* typeStr, cJSON* value);

FunctionalConstraint get_fc_type(const char* fcString);

#endif
