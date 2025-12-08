#ifndef UTILITY_H
#define UTILITY_H

#include <vector>
#include <iostream>
#include <string>
#include <unordered_set>
#include <type_traits>
#include <cstdio>
#include <cstdarg>

std::string ssprintf(const char* format, ...);

template <typename M, typename K>
auto mapFind(const M& map,
             const K& key,
             const typename M::mapped_type& defaultValue = typename M::mapped_type{})
    -> typename M::mapped_type
{
    using V = typename M::mapped_type;

    auto it = map.find(key);
    return (it != map.end()) ? it->second : defaultValue;
}

template <typename M, typename V>
auto mapFindKey(const M& map, const V& value)
    -> typename M::key_type
{
    for (const auto& [key, val] : map) {
        if (val == value) {
            return key;
        }
    }
    return typename M::key_type{}; // default key (0 for uint16_t)
}

template <typename T>
void printVector(const std::vector<T>& vec, const std::string& title = "Vector") {
    std::cout << title << " = [";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i];
        if (i != vec.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n" << std::flush;
}

template <typename T>
std::vector<T> removeDuplicatesPreserveOrder(const std::vector<T>& inputVec) {
    std::vector<T> uniqueVec;
    std::unordered_set<T> seenElements;

    for (const T& element : inputVec) {
        if (seenElements.find(element) == seenElements.end()) { // If element not seen before
            uniqueVec.push_back(element);
            seenElements.insert(element);
        }
    }
    return uniqueVec;
}


// std::vector<float_t> toFloatVector(const std::vector<std::string>& strVec) {
//     std::vector<float_t> result;
//     result.reserve(strVec.size());
//     for (const auto& s : strVec) {
//         const char* cstr = s.c_str();
//         char* endptr = nullptr;
//         errno = 0;
//         float val = std::strtof(cstr, &endptr);
//         if (errno != 0 || endptr == cstr || *endptr != '\0') val = 0.0f;
//         result.push_back(val);
//     }
//     return result;
// }

template<typename T>
std::vector<T> toNumericVector(const std::vector<std::string>& strVec) {
    static_assert(std::is_arithmetic<T>::value, "Template parameter must be a numeric type");

    std::vector<T> result;
    result.reserve(strVec.size());

    for (const auto& s : strVec) {
        const char* cstr = s.c_str();
        char* endptr = nullptr;
        errno = 0;
        T val{0};

        if constexpr (std::is_floating_point<T>::value) {
            if constexpr (std::is_same<T, float>::value) {
                val = std::strtof(cstr, &endptr);
            } else if constexpr (std::is_same<T, double>::value) {
                val = std::strtod(cstr, &endptr);
            } else if constexpr (std::is_same<T, long double>::value) {
                val = std::strtold(cstr, &endptr);
            }
        } else if constexpr (std::is_integral<T>::value) {
            val = static_cast<T>(std::strtoll(cstr, &endptr, 10));
        }

        // If conversion fails, set value to 0
        if (errno != 0 || endptr == cstr || *endptr != '\0') {
            val = 0;
        }

        result.push_back(val);
    }

    return result;
}


template <typename T,
         typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
T sto_generic(const std::string& s, size_t* pos = nullptr) {
    if constexpr (std::is_same<T, int>::value)
        return std::stoi(s, pos);
    else if constexpr (std::is_same<T, long>::value)
        return std::stol(s, pos);
    else if constexpr (std::is_same<T, long long>::value)
        return std::stoll(s, pos);
    else if constexpr (std::is_same<T, unsigned long>::value)
        return std::stoul(s, pos);
    else
        return static_cast<T>(std::stoull(s, pos));
}

template <typename T,
         typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
T sto_generic(const std::string& s, size_t* pos = nullptr) {
    if constexpr (std::is_same<T, float>::value)
        return std::stof(s, pos);
    else if constexpr (std::is_same<T, double>::value)
        return std::stod(s, pos);
    else
        return std::stold(s, pos);
}


#endif // UTILITY_H
