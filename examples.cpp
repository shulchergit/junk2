//#include <signal.h>
#include <string>
#include <vector>
#include <random>
#include <iostream>

#include "plctags.h"

template <typename T>
int readTestNumeric(vector<T> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count);
template <typename T>
int writeTestNumeric(vector<T> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count);
template <typename T>
void exerciseNumeric(std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count);

int readTestString(vector<std::string> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count);
int writeTestString(vector<std::string> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count);
void exerciseString(std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count);


void exercise() {
    // exerciseNumeric<int8_t>("192.168.0.102", "1,0", "contrologix", "ab-eip", "TSINT", 10);        // SINT
    // exerciseNumeric<uint16_t>("192.168.0.102", "1,0", "contrologix", "ab-eip", "SE_STATUS", 10);  // INT
    // exerciseNumeric<uint32_t>("192.168.0.102", "1,0", "contrologix", "ab-eip", "SE_STATUS", 10);  // DINT
    // exerciseNumeric<uint64_t>("192.168.0.102", "1,0", "contrologix", "ab-eip", "TLINT", 1);       // LINT
    // exerciseNumeric<float>("192.168.0.102", "1,0", "contrologix", "ab-eip", "TEST_ARRAY", 10);    // REAL
    // //exerciseNumeric<double>("192.168.0.102", "1,0", "contrologix", "ab-eip", "LREAL_ARRAY", 10); // LREAL - not supported by this PLC

    exerciseString("192.168.0.102", "1,0", "contrologix", "ab-eip", "TEST1", 10);    // STRING
}

template <typename T>
void exerciseNumeric(std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count) {
    // random
    double rnd = []{ std::mt19937 g(std::random_device{}()); return std::uniform_real_distribution<double>(1,10)(g); }();

    // load data to write and verify
    vector<T> testSequence;
    for(int i=0; i<count; i++) {
        testSequence.push_back(rnd += i + 0.111);
    }
    printVector(testSequence, "values to be written to " + tagname);
    writeTestNumeric(testSequence, gateway, path, cpu, protocol, tagname, count);

    vector<T> valuesRead;
    readTestNumeric(valuesRead, gateway, path, cpu, protocol, tagname, count);
    printVector(valuesRead, "values       read  from " + tagname);
}


template <typename T>
int readTestNumeric(vector<T> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count) {
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
int writeTestNumeric(vector<T> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count) {
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

void exerciseString(std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count) {
    // random
    int rnd = []{ std::mt19937 g(std::random_device{}()); return std::uniform_real_distribution<double>(1,10)(g); }();

    // load data to write and verify
    vector<std::string> testSequence;
    for(int i=0; i<count; i++) {
        testSequence.push_back("data " + to_string(rnd += i));
    }
    printVector(testSequence, "values to be written to " + tagname);
    writeTestString(testSequence, gateway, path, cpu, protocol, tagname, count);

    vector<std::string> valuesRead;
    readTestString(valuesRead, gateway, path, cpu, protocol, tagname, count);
    printVector(valuesRead, "values       read  from " + tagname);
}


int readTestString(vector<std::string> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count) {
    std::string es, tagstring;
    tagstring = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    //std::cout << "creating tag: " << tagname << std::endl;
    int32_t tag = createTag(es, tagstring);
    if(tag < 0) {
        std::cout << es << std::endl;
        return tag;
    }

    //std::cout << "reading tag: " << tagname << std::endl;
    int32_t rc = readStringTag(es, tag, values);

    if(rc < 0) {
        std::cout << es << std::endl;
        return rc;
    }

    return 0;
}

int writeTestString(vector<std::string> &values, std::string gateway, std::string path, std::string cpu, std::string protocol, std::string tagname, int count) {
    std::string es, tagstring;
    tagstring = buildTagstring(gateway, tagname, count, path, cpu, protocol);

    //std::cout << "creating tag: " << tagname << std::endl;
    int32_t tag = createTag(es, tagstring);
    if(tag < 0) {
        std::cout << es << std::endl;
        return tag;
    }

    //std::cout << "writing tag: " << tagname << std::endl;
    int32_t rc = writeStringTag(es, tag, values);
    if(rc < 0) {
        std::cout << es << std::endl;
        return rc;
    }

    return 0;
}
