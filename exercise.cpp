//#include <signal.h>
#include <string>
#include <vector>

#include "plctags.h"

void exercise() {
    std::string es, tagstring;
    tagstring = buildTagstring("192.168.0.102", "TEST_ARRAY", 10, "1,0", "contrologix", "ab-eip");
    int32_t tag = createTag(es, tagstring);
    if(tag < 0) {
        //raise(SIGTRAP);
        printf("failed 01....\n");
        fprintf(stderr, "stderr failed 01....\n");
        return;
    }
    printf("created tag\n");
    fprintf(stderr, "stderr created tag\n");

    vector<float_t> values;
    int32_t rc = readNumericTag(es, tag, values);
    if(rc < 0) {
        //raise(SIGTRAP);
        printf("failed 02....\n");
        fprintf(stderr, "stderr failed 02....\n");
        return;
    }
    printf("read tag\n");
    fprintf(stderr, "stderr read tag\n");



    printf("OK....\n");
    fprintf(stderr, "stderr OK....\n");
}
