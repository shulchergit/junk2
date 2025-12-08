#include <stdlib.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdarg>
#include <unordered_map>

#include "plctags.h"
#include "utility.h"

string buildTagstring(
    string gateway,
    string tagname,
    int count,
    string path,
    string cpu,
    string protocol
    ) {

    stringstream loc;
    loc << "protocol=" << protocol << "&gateway=" << gateway
        << "&name=" << tagname << "&elem_count=" << count;
    if(protocol == "modbus_tcp") {
        loc << "&path=0";
    } else {
        loc << "&cpu=" << cpu << "&path=" << path;
    }
    return loc.str();
}

// Define the unordered map: keys are uint8_t codes, values are string literals
const std::unordered_map<uint16_t, std::string> cip_data_map = {
    {0xC0, "DT"},            /* DT value, 64 bit */
    {0xC1, "BIT"},           /* Boolean value, 1 bit */
    {0xC2, "SINT"},          /* Signed 8–bit integer value */
    {0xC3, "INT"},           /* Signed 16–bit integer value */
    {0xC4, "DINT"},          /* Signed 32–bit integer value */
    {0xC5, "LINT"},          /* Signed 64–bit integer value */
    {0xC6, "USINT"},         /* Unsigned 8–bit integer value */
    {0xC7, "UINT"},          /* Unsigned 16–bit integer value */
    {0xC8, "UDINT"},         /* Unsigned 32–bit integer value */
    {0xC9, "ULINT"},         /* Unsigned 64–bit integer value */
    {0xCA, "REAL"},          /* 32–bit floating point value, IEEE format */
    {0xCB, "LREAL"},         /* 64–bit floating point value, IEEE format */
    {0xCC, "STIME"},         /* Synchronous time value */
    {0xCD, "DATE"},          /* Date value */
    {0xCE, "TIME_OF_DAY"},   /* Time of day value */
    {0xCF, "DATE_AND_TIME"}, /* Date and time of day value */
    {0xD0, "STRING"},        /* Character string, 1 byte per character */
    {0xD1, "BYTE"},          /* 8-bit bit string */
    {0xD2, "WORD"},          /* 16-bit bit string */
    {0xD4, "LWORD"},         /* 64-bit bit string */
    {0xD5, "STRING2"},       /* Wide char character string, 2 bytes per character */
    {0xD6, "FTIME"},         /* High resolution duration value */
    {0xD7, "LTIME"},         /* Medium resolution duration value */
    {0xD8, "ITIME"},         /* Low resolution duration value */
    {0xD9, "STRINGN"},       /* N-byte per char character string */
    {0xDA, "SHORT_STRING"},  /* Counted character sting with 1 byte per character and 1 byte length indicator */
    {0xDB, "TIME"},          /* Duration in milliseconds */
    {0xDC, "EPATH"},         /* CIP path segment(s) */
    {0xDD, "ENGUNIT"},       /* Engineering units */
    {0xDE, "STRINGI"},       /* International character string (encoding?) */

    // Aggregate data type byte values
    {0xA0, "ABREV_STRUCT"},  /* Data is an abbreviated struct type, i.e. a CRC of the actual type descriptor */
    {0xA1, "ABREV_ARRAY"},   /* Data is an abbreviated array type. The limits are left off */
    {0xA2, "FULL_STRUCT"},   /* Data is a struct type descriptor */
    {0xA3, "FULL_ARRAY"},    /* Data is a full array type descriptor */

    // Added via test
    {0x8fce, "STRING_LGX"},         /*  what list_tags returns for sting on comtrollogix  */

    {0x8f83, "Timer"},              /*  */
    {0x8ffb, "ALARM_DIGITAL"},      /*  */
    {0x8fff, "MESSAGE"},            /*  */
    {0x8f82, "COUNTER"},            /*  */
    {0x8573, "RADIO_COMM_TYPE"},    /*  */
    {0xa11a, "XFR_TYPE"},           /*  */
    {0x853f, "TIMESTAMP2_TYPE"},    /*  */
    {0xad45, "PM_XFR_STATUS_TYPE"}, /*  SR1_STATUS     */
    {0x8a82, "CONVEYOR_TYPE"},      /*  CONVEYOR_8     */
    {0x1338, "I_BUTTON"},           /*  I_BUTTON       */
    {0x8c9b, "SCALE_TYPE"},         /*  C9B_SCALE      */
    {0xabd1, "PM_XFR_STATUS_TYPE"}, /*  SR1_STATUS      */

    //0xabf9

    // 1756
    {0x8624, "1756_DI_C"},          /*  Local:7:C   AB:1756_DI:C:0  */
    {0x82e9, "1756_DO_C"},          /*  Local:8:C   AB:1756_DO:C:0  */
    {0x8ad5, "1756_DI_I"},          /*  Local:6:I   AB:1756_DI:I:0  */
    {0x8efa, "1756_IF16_C"},        /*  Local:12:C  AB:1756_IF16_Float_No_Alm:C:0 */
    {0x8920, "1756_IF16_I"},        /*  Local:12:I  AB:1756_IF16_Float_No_Alm:I:0 */

    // not strictly data
    {0x106D, "Routine"},            /*  */
    {0x1068, "Program"},            /*  */
    {0x1070, "Task"},               /*  */

    // unlocated tags
    {0x107e, "CXN_STANDARD"},       /*  Cxn:Standard:<8 hex digits>   */
    {0x1069, "MAP_R999"},           /*  older, address-based notation  */


    {0x8a75, ".UNKNOWN."},           /*  __DEFVAL_00000A75  */


};
//     // Example of how to access the map:
//     uint8_t key = 0xC1;
//     if (cip_data_map.count(key)) {
//         std::cout << "Key 0xC1 corresponds to: " << cip_data_map[key] << std::endl;
//     }

//     return 0;
// }
string getTagType(uint16_t type, string defaultValue) {
    string r = mapFind(cip_data_map, type);
    //uint16_t x = type & 0xff;
    if(r.empty()) {
        r = mapFind(cip_data_map, type & 0xff, defaultValue);
    }
    return r;
}
uint16_t getTagTypeCode(string type) {
    return mapFindKey(cip_data_map, type);
}

int32_t createTag(string &error_string, string tagstring, int time_out_ms) {
    int32_t tag;

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!\n", REQUIRED_VERSION);
        return -1001;
    }

    /* create the tag */
    tag = plc_tag_create(tagstring.c_str(), time_out_ms);

    /* everything OK? */
    if(tag < 0) {
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return tag;
    }

    return tag;
}


int32_t readStringTag(string &error_string, int32_t tag, vector<string> &values, int time_out_ms) {
    int str_num = 1;
    int offset = 0;

    /* get the data */
    int32_t rc2 = plc_tag_read(tag, time_out_ms);
    if(rc2 != PLCTAG_STATUS_OK) {
        // fprintf(stderr, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc2, plc_tag_decode_error(rc2));
        return rc2;
    }

    auto [rc, elem_size, elem_count] = tagGetSizes(error_string, tag);
    if(rc != PLCTAG_STATUS_OK) {
        return rc;
    }

    /* load the return vector*/
    offset = 0;
    str_num = 1;
    while(offset < plc_tag_get_size(tag)) {
        char *str = NULL;
        int str_cap = plc_tag_get_string_length(tag, offset) + 1; /* +1 for the zero termination. */

        str = (char*)malloc((size_t)(unsigned int)str_cap);
        if(!str) {
            // NOLINTNEXTLINE
            //fprintf(stderr, "Unable to allocate memory for the string %d of tag!\n", str_num);
            error_string = ssprintf("Unable to allocate memory for the string %d of tag!", str_num);
            plc_tag_destroy(tag);
            return PLCTAG_ERR_NO_MEM;
        }

        rc = plc_tag_get_string(tag, offset, str, str_cap);
        //rc = plc_tag_set_string(tag, offset, str);
        if(rc != PLCTAG_STATUS_OK) {
            // NOLINTNEXTLINE
            //fprintf(stderr, "Unable to get string %d of tag, got error %s!\n", str_num, plc_tag_decode_error(rc));
            error_string = ssprintf("Unable to get string %d of tag, got error %s!", str_num, plc_tag_decode_error(rc));
            free(str);
            plc_tag_destroy(tag);
            return rc;
        }

        // NOLINTNEXTLINE
        //fprintf(stderr, "tag string %d (%u chars) = '%s'\n", str_num, (unsigned int)strlen(str), str);
        values.push_back(str);

        free(str);

        str_num++;

        offset += plc_tag_get_string_total_length(tag, offset);
    }

    return 0;
}

int32_t writeStringTag(string &error_string, int32_t tag, vector<string> &values, int time_out_ms) {
    int str_num = 1;
    int offset = 0;

    auto [rc, elem_size, elem_count] = tagGetSizes(error_string, tag);

    /* update the string. */
    for(int i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //compat_snprintf(str, sizeof(str), "string value for element %d is %d.", i, (int)(rand() % 1000));
        int str_total_length = plc_tag_get_string_total_length(tag, 0); /* assume all are the same size */

        rc = plc_tag_set_string(tag, str_total_length * i, values[i].c_str());
        if(rc != PLCTAG_STATUS_OK) {
            // NOLINTNEXTLINE
            //fprintf(stderr, "Error setting string %d, error %s!\n", i, plc_tag_decode_error(rc));
            error_string = ssprintf("Error setting string %d, error %s!\n", i, plc_tag_decode_error(rc));
            return rc;
        }
    }

    /* write the data */
    rc = plc_tag_write(tag, time_out_ms);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stdout, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        return rc;
    }

    return rc;
}




//===============================================================================================================

PLCTAGT_RESULT readReal32s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count,
                           string path, string protocol, string cpu
                           ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;
    float_t error_sentinal = 1.17549435e-38;  //1.17549e-38;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!\n", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the data */
    rc = plc_tag_read(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        //fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    // NOLINTNEXTLINE
    //fprintf(stderr, "Tag has %d elements each of %d bytes.\n", elem_count, elem_size);

    /* print out the data */
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "d data[%d]=%d\n", i, plc_tag_get_int32(tag, (i * elem_size)));
        //fprintf(stderr, "f data[%d]=%f\n", i, plc_tag_get_float32(tag, (i * elem_size)));
        float_t val = plc_tag_get_float32(tag, (i * elem_size));
        if(val == error_sentinal) {
            //fprintf(stderr, "ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            error_string = ssprintf("ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            plc_tag_destroy(tag);
            return PLCTAGT_RESULT::ERR_TAG_READ;
        }
        values.push_back(val);
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT writeReal32s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count,
                            string path, string protocol, string cpu
                            ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        // fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    /* now test a write */
    int offset = 0;
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "Setting element %d to %f\n", i, val);
        plc_tag_set_float32(tag, (i * elem_size), values[i]);
    }

    rc = plc_tag_write(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT readReal64s(string &error_string, string gateway, string tagname, vector<double_t> &values, int count,
                           string path, string protocol, string cpu
                           ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;
    float_t error_sentinal = 1.17549435e-38;  //1.17549e-38;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!\n", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the data */
    rc = plc_tag_read(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        //fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    // NOLINTNEXTLINE
    //fprintf(stderr, "Tag has %d elements each of %d bytes.\n", elem_count, elem_size);

    /* print out the data */
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "d data[%d]=%d\n", i, plc_tag_get_int32(tag, (i * elem_size)));
        //fprintf(stderr, "f data[%d]=%f\n", i, plc_tag_get_float32(tag, (i * elem_size)));
        double_t val = plc_tag_get_float64(tag, (i * elem_size));
        if(val == error_sentinal) {
            //fprintf(stderr, "ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            error_string = ssprintf("ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            plc_tag_destroy(tag);
            return PLCTAGT_RESULT::ERR_TAG_READ;
        }
        values.push_back(val);
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT writeReal64s(string &error_string, string gateway, string tagname, vector<double_t> &values, int count,
                            string path, string protocol, string cpu
                            ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        // fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    /* now test a write */
    int offset = 0;
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "Setting element %d to %f\n", i, val);
        plc_tag_set_float64(tag, (i * elem_size), values[i]);
    }

    rc = plc_tag_write(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT readInt16s(string &error_string, string gateway, string tagname, vector<int16_t> &values, int count,
                          string path, string protocol, string cpu
                          ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;
    float_t error_sentinal = 1.17549435e-38;  //1.17549e-38;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!\n", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* int rcreate the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the data */
    rc = plc_tag_read(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        //fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    // NOLINTNEXTLINE
    //fprintf(stderr, "Tag has %d elements each of %d bytes.\n", elem_count, elem_size);

    /* print out the data */
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "d data[%d]=%d\n", i, plc_tag_get_int32(tag, (i * elem_size)));
        //fprintf(stderr, "f data[%d]=%f\n", i, plc_tag_get_float32(tag, (i * elem_size)));
        int16_t val = plc_tag_get_int16(tag, (i * elem_size));
        if(val == error_sentinal) {
            //fprintf(stderr, "ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            error_string = ssprintf("ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            plc_tag_destroy(tag);
            return PLCTAGT_RESULT::ERR_TAG_READ;
        }
        values.push_back(val);
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT writeInt16s(string &error_string, string gateway, string tagname, vector<int16_t> &values, int count,
                           string path, string protocol, string cpu
                           ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        // fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    /* now test a write */
    int offset = 0;
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "Setting element %d to %f\n", i, val);
        int16_t v = values[i];
        plc_tag_set_int16(tag, (i * elem_size), v);
    }

    rc = plc_tag_write(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}


PLCTAGT_RESULT readInt32s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count,
                          string path, string protocol, string cpu
                          ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;
    float_t error_sentinal = 1.17549435e-38;  //1.17549e-38;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!\n", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the data */
    rc = plc_tag_read(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        //fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    // NOLINTNEXTLINE
    //fprintf(stderr, "Tag has %d elements each of %d bytes.\n", elem_count, elem_size);

    /* print out the data */
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "d data[%d]=%d\n", i, plc_tag_get_int32(tag, (i * elem_size)));
        //fprintf(stderr, "f data[%d]=%f\n", i, plc_tag_get_float32(tag, (i * elem_size)));
        int32_t val = plc_tag_get_int32(tag, (i * elem_size));
        if(val == error_sentinal) {
            //fprintf(stderr, "ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            error_string = ssprintf("ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            plc_tag_destroy(tag);
            return PLCTAGT_RESULT::ERR_TAG_READ;
        }
        values.push_back(val);
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT writeInt32s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count,
                           string path, string protocol, string cpu
                           ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        // fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    /* now test a write */
    int offset = 0;
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "Setting element %d to %f\n", i, val);
        int32_t v = values[i];
        plc_tag_set_int32(tag, (i * elem_size), v);
    }

    rc = plc_tag_write(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT readInt64s(string &error_string, string gateway, string tagname, vector<int64_t> &values, int count,
                          string path, string protocol, string cpu
                          ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;
    float_t error_sentinal = 1.17549435e-38;  //1.17549e-38;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!\n", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the data */
    rc = plc_tag_read(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        //fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    // NOLINTNEXTLINE
    //fprintf(stderr, "Tag has %d elements each of %d bytes.\n", elem_count, elem_size);

    /* print out the data */
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "d data[%d]=%d\n", i, plc_tag_get_int32(tag, (i * elem_size)));
        //fprintf(stderr, "f data[%d]=%f\n", i, plc_tag_get_float32(tag, (i * elem_size)));
        int64_t val = plc_tag_get_int64(tag, (i * elem_size));
        if(val == error_sentinal) {
            //fprintf(stderr, "ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            error_string = ssprintf("ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            plc_tag_destroy(tag);
            return PLCTAGT_RESULT::ERR_TAG_READ;
        }
        values.push_back(val);
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT writeInt64s(string &error_string, string gateway, string tagname, vector<int64_t> &values, int count,
                           string path, string protocol, string cpu
                           ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        // fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    /* now test a write */
    int offset = 0;
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "Setting element %d to %f\n", i, val);
        int64_t v = values[i];
        plc_tag_set_int64(tag, (i * elem_size), v);
    }

    rc = plc_tag_write(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}


PLCTAGT_RESULT readBits(string &error_string, string gateway, string tagname, vector<float_t> &values, int count,
                        string path, string protocol, string cpu
                        ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;
    float_t error_sentinal = 1.17549435e-38;  //1.17549e-38;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!\n", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the data */
    rc = plc_tag_read(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        //fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    // NOLINTNEXTLINE
    //fprintf(stderr, "Tag has %d elements each of %d bytes.\n", elem_count, elem_size);

    /* print out the data */
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "d data[%d]=%d\n", i, plc_tag_get_int32(tag, (i * elem_size)));
        //fprintf(stderr, "f data[%d]=%f\n", i, plc_tag_get_float32(tag, (i * elem_size)));
        int val = plc_tag_get_bit(tag, (i * elem_size));
        if(val == error_sentinal) {
            //fprintf(stderr, "ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            error_string = ssprintf("ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            plc_tag_destroy(tag);
            return PLCTAGT_RESULT::ERR_TAG_READ;
        }
        values.push_back(val);
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT writeBits(string &error_string, string gateway, string tagname, vector<float_t> &values, int count,
                         string path, string protocol, string cpu
                         ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        // fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    /* now test a write */
    int offset = 0;
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "Setting element %d to %f\n", i, val);
        int v = values[i];
        plc_tag_set_bit(tag, (i * elem_size), v);
    }

    rc = plc_tag_write(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}


PLCTAGT_RESULT readInt8s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count,
                         string path, string protocol, string cpu
                         ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;
    float_t error_sentinal = 1.17549435e-38;  //1.17549e-38;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);
    //"protocol=ab_eip&gateway=10.206.1.27&path=1,0&cpu=LGX&elem_size=1&elem_count=1&debug=1&name=pcomm_test_bool"
    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!\n", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the data */
    rc = plc_tag_read(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        //fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_READ;
    }
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    // NOLINTNEXTLINE
    //fprintf(stderr, "Tag has %d elements each of %d bytes.\n", elem_count, elem_size);

    /* print out the data */
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "d data[%d]=%d\n", i, plc_tag_get_int32(tag, (i * elem_size)));
        //fprintf(stderr, "f data[%d]=%f\n", i, plc_tag_get_float32(tag, (i * elem_size)));
        int32_t val = plc_tag_get_int8(tag, (i * elem_size));
        if(val == error_sentinal) {
            //fprintf(stderr, "ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            error_string = ssprintf("ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
            plc_tag_destroy(tag);
            return PLCTAGT_RESULT::ERR_TAG_READ;
        }
        values.push_back(val);
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}

PLCTAGT_RESULT writeInt8s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count,
                          string path, string protocol, string cpu
                          ) {
    int32_t tag = 0;
    int rc;
    int i;
    int elem_size = 0;
    int elem_count = 0;

    string location = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        // exit(1);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    //plc_tag_set_debug_level(PLCTAG_DEBUG_DETAIL);

    /* create the tag */
    tag = plc_tag_create(location.c_str(), DATA_TIMEOUT);

    /* everything OK? */
    if(tag < 0) {
        // NOLINTNEXTLINE
        // fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return PLCTAGT_RESULT::ERR_TAG_CREATE;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        // fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname [%s].\n", tagname.c_str());
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    /* now test a write */
    int offset = 0;
    for(i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "Setting element %d to %f\n", i, val);
        int8_t v = values[i];
        plc_tag_set_int8(tag, (i * elem_size), v);
    }

    rc = plc_tag_write(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return PLCTAGT_RESULT::ERR_TAG_WRITE;
    }

    /* we are done */
    plc_tag_destroy(tag);

    return PLCTAGT_RESULT::SUCCESS;
}


#include <cstring>



int testStringRead(string &error_string, const char *tag_string, vector<string> &values) {
    int32_t tag = 0;
    int rc;
    int str_num = 1;
    int offset = 0;
    int elem_size = 0;
    int elem_count = 0;

    /* check the library version. */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        error_string = ssprintf("Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        return 1;
    }

    // NOLINTNEXTLINE
    //fprintf(stderr, "Using library version %d.%d.%d.\n", plc_tag_get_int_attribute(0, "version_major", -1),
    //        plc_tag_get_int_attribute(0, "version_minor", -1), plc_tag_get_int_attribute(0, "version_patch", -1));
    error_string = ssprintf("Using library version %d.%d.%d.", plc_tag_get_int_attribute(0, "version_major", -1),
                            plc_tag_get_int_attribute(0, "version_minor", -1), plc_tag_get_int_attribute(0, "version_patch", -1));


    /* turn off debugging output. */
    plc_tag_set_debug_level(PLCTAG_DEBUG_NONE);

    /* loop over the tag strings. */
    tag = plc_tag_create(tag_string, DATA_TIMEOUT);

    /* everything OK? */
    if((rc = plc_tag_status(tag)) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //printf(stderr, "Error creating tag! Error %s\n", plc_tag_decode_error(rc));
        error_string = ssprintf("Error creating tag! Error %s", plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return rc;
    }

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        //fprintf(stderr, "ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname.\n");
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname.");
        plc_tag_destroy(tag);
        return rc;
    }

    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

    /* get the data */
    rc = plc_tag_read(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR: Unable to read the data for tag! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data for tag! Got error code %d: %s", rc, plc_tag_decode_error(rc));
        plc_tag_destroy(tag);
        return rc;
    }

    /* print out the data */
    offset = 0;
    str_num = 1;
    while(offset < plc_tag_get_size(tag)) {
        char *str = NULL;
        int str_cap = plc_tag_get_string_length(tag, offset) + 1; /* +1 for the zero termination. */

        str = (char*)malloc((size_t)(unsigned int)str_cap);
        if(!str) {
            // NOLINTNEXTLINE
            //fprintf(stderr, "Unable to allocate memory for the string %d of tag!\n", str_num);
            error_string = ssprintf("Unable to allocate memory for the string %d of tag!", str_num);
            plc_tag_destroy(tag);
            return PLCTAG_ERR_NO_MEM;
        }

        rc = plc_tag_get_string(tag, offset, str, str_cap);
        //rc = plc_tag_set_string(tag, offset, str);
        if(rc != PLCTAG_STATUS_OK) {
            // NOLINTNEXTLINE
            //fprintf(stderr, "Unable to get string %d of tag, got error %s!\n", str_num, plc_tag_decode_error(rc));
            error_string = ssprintf("Unable to get string %d of tag, got error %s!", str_num, plc_tag_decode_error(rc));
            free(str);
            plc_tag_destroy(tag);
            return rc;
        }

        // NOLINTNEXTLINE
        //fprintf(stderr, "tag string %d (%u chars) = '%s'\n", str_num, (unsigned int)strlen(str), str);
        values.push_back(str);

        free(str);

        str_num++;

        offset += plc_tag_get_string_total_length(tag, offset);
    }

    /* we are done */
    plc_tag_destroy(tag);

    return 0;
}

void update_string(int32_t tag, int str_number, const string &str) {
    int rc = 0;
    int str_total_length = plc_tag_get_string_total_length(tag, 0); /* assume all are the same size */

    rc = plc_tag_set_string(tag, str_total_length * str_number, str.c_str());
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        fprintf(stderr, "Error setting string %d, error %s!\n", str_number, plc_tag_decode_error(rc));
        return;
    }
}

int writeStringsHack(string &error_string, const string &tag_string, vector<string> &values) {
    int rc = PLCTAG_STATUS_OK;
    //char str[STRING_DATA_SIZE + 1] = "homminahomminahomminahomminamama";

    int32_t tag = 0;
    int string_count = 0;

    /* check library API version */
    if(plc_tag_check_lib_version(REQUIRED_VERSION) != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        fprintf(stderr, "Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        //error_string = ssprintf("Required compatible library version %d.%d.%d not available!", REQUIRED_VERSION);
        exit(1);
    }

    // NOLINTNEXTLINE
    //fprintf(stderr, "Using library version %d.%d.%d.\n", plc_tag_get_int_attribute(0, "version_major", -1),
    //        plc_tag_get_int_attribute(0, "version_minor", -1), plc_tag_get_int_attribute(0, "version_patch", -1));
    error_string = ssprintf("Using library version %d.%d.%d.\n", plc_tag_get_int_attribute(0, "version_major", -1),
                            plc_tag_get_int_attribute(0, "version_minor", -1), plc_tag_get_int_attribute(0, "version_patch", -1));


    /* set up debugging output. */
    plc_tag_set_debug_level(PLCTAG_DEBUG_NONE);

    /* set up the RNG */
    //srand((unsigned int)(uint64_t)compat_time_ms());

    /* create the tag. */
    if((tag = plc_tag_create(tag_string.c_str(), DATA_TIMEOUT)) < 0) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        error_string = ssprintf("ERROR %s: Could not create tag!\n", plc_tag_decode_error(tag));
        return rc;
    }

    // /* get the data */
    // rc = plc_tag_read(tag, DATA_TIMEOUT);
    // if(rc != PLCTAG_STATUS_OK) {
    //     // NOLINTNEXTLINE
    //     //fprintf(stdout, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
    //     error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
    //     return rc;
    // }

    // /* dump the "before" state. */
    // printf("Strings before update:\n");
    // dump_strings(tag);

    /* how many strings do we have? */
    string_count = plc_tag_get_int_attribute(tag, "elem_count", 1);

    /* update the string. */
    for(int i = 0; i < string_count; i++) {
        // NOLINTNEXTLINE
        //compat_snprintf(str, sizeof(str), "string value for element %d is %d.", i, (int)(rand() % 1000));
        update_string(tag, i, values[i].c_str());
    }

    /* write the data */
    rc = plc_tag_write(tag, DATA_TIMEOUT);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stdout, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        return rc;
    }

    // /* get the data again */
    // rc = plc_tag_read(tag, DATA_TIMEOUT);
    // if(rc != PLCTAG_STATUS_OK) {
    //     // NOLINTNEXTLINE
    //     //fprintf(stdout, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
    //     error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
    //     return rc;
    // }

    // /* dump the "after" state */
    // printf("\nStrings after update:\n");
    // dump_strings(tag);

    plc_tag_destroy(tag);

    return rc;
}
