#ifndef PLCTAGS_H
#define PLCTAGS_H

#include <string>
#include <stdlib.h>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <array>

#include "utility.h"

using namespace std;

//#include "compat_utils.h"
#include <libplctag.h>

#define REQUIRED_VERSION 2, 4, 0
#define DATA_TIMEOUT 5000

#define DEFAULT_PROTOCOL "ab-eip"
#define DEFAULT_PATH "1,0"
//#define DEFAULT_CPU "LGX"
#define DEFAULT_CPU "compactlogix"

// #define TYPE_IS_STRUCT ((uint16_t)0x8000)
// #define TYPE_IS_SYSTEM ((uint16_t)0x1000)
// #define TYPE_DIM_MASK ((uint16_t)0x6000)
// #define TYPE_UDT_ID_MASK ((uint16_t)0x0FFF)
// #define TAG_DIM_MASK ((uint16_t)0x6000)

/* test against a DINT array. */
//python3 -m cpppo.server.enip -v real=REAL reals=REAL[100] dint=DINT dints=DINT[100]
// #define TAG_PATH_O "protocol=ab-eip&gateway=127.0.0.1&path=1,0&cpu=LGX&elem_count=10&name=reals"
// #define TAG_PATH_I "protocol=ab-eip&gateway=127.0.0.1&path=1,0&cpu=LGX&elem_count=10&name=dints"
// #define TAG_PATH_F "protocol=ab-eip&gateway=127.0.0.1&path=1,0&cpu=LGX&elem_count=10&name=reals"

extern const std::unordered_map<uint16_t, std::string> cip_data_map;
string getTagType(uint16_t type, string defaultValue = "");
uint16_t getTagTypeCode(string type);

string buildTagstring(
    string gateway,
    string tagname,
    int count = 1,
    string path = DEFAULT_PATH,
    string cpu = DEFAULT_CPU,
    string protocol = DEFAULT_PROTOCOL
    );
int32_t createTag(string &error_string, string tagstring, int time_out_ms = DATA_TIMEOUT);
// template <typename T>
// int32_t readNumericTag(string &error_string, int32_t tag, vector<T> &values, int time_out_ms = DATA_TIMEOUT);
// template <typename T>
// int32_t writeNumericTag(string &error_string, int32_t tag, vector<T> &values, int time_out_ms = DATA_TIMEOUT);
int32_t readStringTag(string &error_string, int32_t tag, vector<string> &values, int time_out_ms = DATA_TIMEOUT);
int32_t writeStringTag(string &error_string, int32_t tag, vector<string> &values, int time_out_ms = DATA_TIMEOUT);

//maybe move to cpp
inline array<int32_t, 3> tagGetSizes(string &error_string, int32_t tag) {
    int elem_size = 0;
    int elem_count = 0;
    int32_t rc;

    /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    if(elem_size == 0) {
        error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/unssupported/nonexistent tagname.\n");
        return {-1001, 0, 0};
    }

    //integer
    elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);
    if(elem_count == 0) {
        error_string = ssprintf("ERROR: elem_count is 0.\n");
        return {-1002, 0, 0};
    }

    return {rc, elem_size, elem_count};
}

template <typename T>
int32_t readNumericTag(string &error_string, int32_t tag, vector<T> &values, int time_out_ms = DATA_TIMEOUT) {
    // int elem_size = 0;
    // int elem_count = 0;
    // int32_t rc;

    // /* get the data */
    // rc = plc_tag_read(tag, time_out_ms);
    // if(rc != PLCTAG_STATUS_OK) {
    //     // NOLINTNEXTLINE
    //     // fprintf(stderr, "ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
    //     error_string = ssprintf("ERROR: Unable to read the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
    //     plc_tag_destroy(tag);
    //     return rc;
    // }

    // /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the tag is! */
    // elem_size = plc_tag_get_int_attribute(tag, "elem_size", 0);
    // if(elem_size == 0) {
    //     error_string = ssprintf("ERROR: elem_size is 0. Can also signify invalid/nonexistent tagname.\n");
    //     return rc;
    // }

    // //integer
    // elem_count = plc_tag_get_int_attribute(tag, "elem_count", 0);

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

    /* retrieve the data */
    for(int i = 0; i < elem_count; i++) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "d data[%d]=%d\n", i, plc_tag_get_int32(tag, (i * elem_size)));
        //fprintf(stderr, "f data[%d]=%f\n", i, plc_tag_get_float32(tag, (i * elem_size)));
        T val;

        if        constexpr (std::is_same_v<T, int8_t>) {
            val = plc_tag_get_int8(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            val = plc_tag_get_uint8(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, int16_t>) {
            val = plc_tag_get_int16(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            val = plc_tag_get_uint16(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, int32_t>) {
            val = plc_tag_get_int32(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            val = plc_tag_get_uint32(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, int64_t>) {
            val = plc_tag_get_int64(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            val = plc_tag_get_uint64(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, float_t>) {
            val = plc_tag_get_float32(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, double_t>) {
            val = plc_tag_get_float64(tag, (i * elem_size));
        } else if constexpr (std::is_same_v<T, bool>) {
            val = plc_tag_get_bit(tag, (i * elem_size));
        }

        // else if constexpr (std::is_same_v<T, long>) {
        // }
        // else if constexpr (std::is_same_v<T, long long>) {
        // }
        // if(val == error_sentinal) {
        //     //fprintf(stderr, "ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
        //     error_string = ssprintf("ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
        //     plc_tag_destroy(tag);
        //     return PLCTAGT_RESULT::ERR_TAG_READ;
        // }
        values.push_back(val);
    }

    return 0;
}

template <typename T>
int32_t writeNumericTag(string &error_string, int32_t tag, vector<T> &values, int time_out_ms = DATA_TIMEOUT) {

    auto [rc, elem_size, elem_count] = tagGetSizes(error_string, tag);

    // /* nset the data to be written */
    // //int offset = 0;
    // for(int i = 0; i < elem_count; i++) {
    //     // NOLINTNEXTLINE
    //     //fprintf(stderr, "Setting element %d to %f\n", i, val);
    //     plc_tag_set_float32(tag, (i * elem_size), values[i]);
    // }

    /* write the data to the PLC */
    for(int i = 0; i < elem_count; i++) {
        //T val;

        if        constexpr (std::is_same_v<T, int8_t>) {
            rc = plc_tag_set_int8(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            rc = plc_tag_set_uint8(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, int16_t>) {
            rc = plc_tag_set_int16(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            rc = plc_tag_set_uint16(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, int32_t>) {
            rc = plc_tag_set_int32(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            rc = plc_tag_set_uint32(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            rc = plc_tag_set_int64(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            rc = plc_tag_set_uint64(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, float_t>) {
            rc = plc_tag_set_float32(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, double_t>) {
            rc = plc_tag_set_float64(tag, (i * elem_size), values[i]);
        } else if constexpr (std::is_same_v<T, bool>) {
            rc = plc_tag_set_bit(tag, (i * elem_size), values[i]);
        }

        // else if constexpr (std::is_same_v<T, long>) {
        // }
        // else if constexpr (std::is_same_v<T, long long>) {
        // }
        // if(val == error_sentinal) {
        //     //fprintf(stderr, "ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
        //     error_string = ssprintf("ERROR: Read sentinal value (1.17549e-38) [%s].\n", tagname.c_str());
        //     plc_tag_destroy(tag);
        //     return PLCTAGT_RESULT::ERR_TAG_READ;
        // }
        //values.push_back(val);
        if(rc != PLCTAG_STATUS_OK) {
            error_string = ssprintf("ERROR: Unable to set the data! GError code %d: %s\n", rc, plc_tag_decode_error(rc));
            return rc;
        }
    }

    rc = plc_tag_write(tag, time_out_ms);
    if(rc != PLCTAG_STATUS_OK) {
        // NOLINTNEXTLINE
        //fprintf(stderr, "ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        error_string = ssprintf("ERROR: Unable to write the data! Got error code %d: %s\n", rc, plc_tag_decode_error(rc));
        return rc;
    }

    return 0;
}



//============================================================================
enum class PLCTAGT_RESULT { SUCCESS=0, ERR_TAG_CREATE, ERR_TAG_READ, ERR_TAG_WRITE };

PLCTAGT_RESULT readInt8s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count = 1,
                         string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                         );

PLCTAGT_RESULT writeInt8s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count = 1,
                          string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                          );

PLCTAGT_RESULT readInt16s(string &error_string, string gateway, string tagname, vector<int16_t> &values, int count = 1,
                          string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                          );

PLCTAGT_RESULT writeInt16s(string &error_string, string gateway, string tagname, vector<int16_t> &values, int count,
                           string path, string protocol, string cpu
                           );

PLCTAGT_RESULT readInt32s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count = 1,
                          string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                          );

PLCTAGT_RESULT writeInt32s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count,
                           string path, string protocol, string cpu
                           );

PLCTAGT_RESULT readInt64s(string &error_string, string gateway, string tagname, vector<int64_t> &values, int count = 1,
                          string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                          );

PLCTAGT_RESULT writeInt64s(string &error_string, string gateway, string tagname, vector<int64_t> &values, int count,
                           string path, string protocol, string cpu
                           );


PLCTAGT_RESULT readReal32s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count = 1,
                           string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                           );

PLCTAGT_RESULT writeReal32s(string &error_string, string gateway, string tagname, vector<float_t> &values, int count = 1,
                            string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                            );

PLCTAGT_RESULT readReal64s(string &error_string, string gateway, string tagname, vector<double_t> &values, int count = 1,
                           string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                           );

PLCTAGT_RESULT writeReal64s(string &error_string, string gateway, string tagname, vector<double_t> &values, int count = 1,
                            string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                            );

PLCTAGT_RESULT readBits(string &error_string, string gateway, string tagname, vector<float_t> &values, int count = 1,
                        string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                        );

PLCTAGT_RESULT writeBits(string &error_string, string gateway, string tagname, vector<float_t> &values, int count = 1,
                         string path = DEFAULT_PATH, string protocol = DEFAULT_PROTOCOL, string cpu = DEFAULT_CPU
                         );

int testStringRead(string &error_string, const char *tag_string, vector<string> &values);

int testStringWrite(string &error_string, const char *tag_string, vector<string> &values);

int writeStringsHack(string &error_string, const string &tag_string, vector<string> &values);


#endif // PLCTAGS_H
