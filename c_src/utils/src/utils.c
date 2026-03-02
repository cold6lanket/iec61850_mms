#include <string.h>
#include <stdio.h>
#include "utils.h"

cJSON* mms2json(MmsValue* value) {
    if (value == NULL) return cJSON_CreateNull();

    MmsType type = MmsValue_getType(value);

    switch (type) {
        case MMS_BOOLEAN:
            return cJSON_CreateBool(MmsValue_getBoolean(value));
        case MMS_INTEGER:
            return cJSON_CreateNumber((double)MmsValue_toInt64(value));
        case MMS_UTC_TIME:
            return cJSON_CreateNumber((int)MmsValue_toUnixTimestamp(value));
        case MMS_UNSIGNED:
            return cJSON_CreateNumber((double)MmsValue_toUint32(value));
        case MMS_FLOAT:
            return cJSON_CreateNumber((double)MmsValue_toFloat(value));
        case MMS_VISIBLE_STRING:
        case MMS_STRING:
            return cJSON_CreateString(MmsValue_toString(value));
        case MMS_BIT_STRING: {
            char buffer[64];
            sprintf(buffer, "0x%X", MmsValue_getBitStringAsInteger(value));
            return cJSON_CreateString(buffer);
        }
        case MMS_DATA_ACCESS_ERROR:
            return cJSON_CreateString("Access Error");
        default:
            return cJSON_CreateString("Unsupported Type");
    }
}


MmsValue *json2mms(const char* typeStr, cJSON* value) {
    if (strcmp(typeStr, "Boolean") == 0) {
        return MmsValue_newBoolean(cJSON_IsTrue(value));
    } 
    else if (strcmp(typeStr, "Int") == 0) {
        return MmsValue_newInteger((int)value->valuedouble);
    }
    else if (strcmp(typeStr, "Float") == 0) {
        return MmsValue_newFloat((float)value->valuedouble);
    }
    else if (strcmp(typeStr, "Int32") == 0) {
        return MmsValue_newIntegerFromInt32(value->valueint);
    }
    else if (strcmp(typeStr, "Int64") == 0) {
        return MmsValue_newIntegerFromInt64(value->valueint);
    }
    else if (strcmp(typeStr, "String") == 0) {
        return MmsValue_newVisibleString(value->valuestring);
    }
    // Add other types (BitString, UtcTime) as needed
    return NULL;
}

FunctionalConstraint get_fc_type(const char* fcString) {
    if (fcString == NULL) return -1; // Error safety

    // Status information
    if (strcmp(fcString, "ST") == 0) return IEC61850_FC_ST;
    
    // Measurands (Analog values)
    if (strcmp(fcString, "MX") == 0) return IEC61850_FC_MX;
    
    // Setpoints (Settings)
    if (strcmp(fcString, "SP") == 0) return IEC61850_FC_SP;
    
    // Controls
    if (strcmp(fcString, "CO") == 0) return IEC61850_FC_CO;

    if (strcmp(fcString, "SG") == 0) return IEC61850_FC_SG;
    if (strcmp(fcString, "SR") == 0) return IEC61850_FC_SR;
    
    // Configuration
    if (strcmp(fcString, "CF") == 0) return IEC61850_FC_CF;
    
    // Description
    if (strcmp(fcString, "DC") == 0) return IEC61850_FC_DC;
    
    // Parameters (less common)
  
    if (strcmp(fcString, "SE") == 0) return IEC61850_FC_SE;
    if (strcmp(fcString, "SV") == 0) return IEC61850_FC_SV;

    if (strcmp(fcString, "OR") == 0) return IEC61850_FC_OR;
    if (strcmp(fcString, "BL") == 0) return IEC61850_FC_BL;
    if (strcmp(fcString, "EX") == 0) return IEC61850_FC_EX;
    if (strcmp(fcString, "US") == 0) return IEC61850_FC_US;
    if (strcmp(fcString, "MS") == 0) return IEC61850_FC_MS;
    if (strcmp(fcString, "RP") == 0) return IEC61850_FC_RP;

    if (strcmp(fcString, "BR") == 0) return IEC61850_FC_BR;
    if (strcmp(fcString, "LG") == 0) return IEC61850_FC_LG;

    if (strcmp(fcString, "GO") == 0) return IEC61850_FC_GO;

    if (strcmp(fcString, "ALL") == 0) return IEC61850_FC_ALL;
    if (strcmp(fcString, "NONE") == 0) return IEC61850_FC_NONE;

    return (FunctionalConstraint) -1; 
}

void printSpaces(int spaces)
{
    int i;

    for (i = 0; i < spaces; i++)
        printf(" ");
}

void printDataDirectory(char* doRef, IedConnection con, int spaces)
{
    IedClientError error;

    LinkedList dataAttributes = IedConnection_getDataDirectory(con, &error, doRef);

    //LinkedList dataAttributes = IedConnection_getDataDirectoryByFC(con, &error, doRef, MX);

    if (dataAttributes != NULL) {
        LinkedList dataAttribute = LinkedList_getNext(dataAttributes);

        while (dataAttribute != NULL) {
            char* daName = (char*) dataAttribute->data;

            printSpaces(spaces);
            printf("DA: %s\n", (char*) dataAttribute->data);

            dataAttribute = LinkedList_getNext(dataAttribute);

            char daRef[130];
            sprintf(daRef, "%s.%s", doRef, daName);
            printDataDirectory(daRef, con, spaces + 2);
        }
    }

    LinkedList_destroy(dataAttributes);
}