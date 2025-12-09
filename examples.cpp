//#include <signal.h>
#include <string>
#include <vector>
#include <random>
#include <iostream>

#include "plctags.h"


template <typename T>
int readTest(vector<T> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count);
template <typename T>
int writeTest(vector<T> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count);

void exercise() {
    int count;
    std::string tagname;
    // random
    double rnd = []{ std::mt19937 g(std::random_device{}()); return std::uniform_real_distribution<double>(1,1000)(g); }();

    tagname = "TEST_ARRAY";
    count = 10;
    vector<float_t> testSequence;
    for(int i=0; i<count; i++) {
        testSequence.push_back(rnd += i + 0.111);
    }
    printVector(testSequence, "values to be written to " + tagname);
    writeTest(testSequence, "192.168.0.102", "1,0", "contrologix", "ab-eip", tagname, count);

    vector<float_t> valuesRead;
    readTest(valuesRead, "192.168.0.102", "1,0", "contrologix", "ab-eip", tagname, count);
    printVector(valuesRead, "values       read  from " + tagname);
}


template <typename T>
int readTest(vector<T> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count) {
    std::string es, tagstring;
    tagstring = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    //std::cout << "creating tag: " << tagname << std::endl;
    int32_t tag = createTag(es, tagstring);
    if(tag < 0) {
        std::cout << es << std::endl;
        return tag;
    }

    //std::cout << "reading tag: " << tagname << std::endl;
    int32_t rc = readNumericTag(es, tag, values);

    if(rc < 0) {
        std::cout << es << std::endl;
        return rc;
    }

    return 0;
}

template <typename T>
int writeTest(vector<T> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count) {
    std::string es, tagstring;
    tagstring = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    //std::cout << "creating tag: " << tagname << std::endl;
    int32_t tag = createTag(es, tagstring);
    if(tag < 0) {
        std::cout << es << std::endl;
        return tag;
    }

    //std::cout << "writing tag: " << tagname << std::endl;
    int32_t rc = writeNumericTag(es, tag, values);
    if(rc < 0) {
        std::cout << es << std::endl;
        return rc;
    }

    return 0;
}

