#include "utility.h"


std::string ssprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    // Copy to compute required size
    va_list args_copy;
    va_copy(args_copy, args);
    int size = std::vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        va_end(args);
        throw std::runtime_error("Error during string formatting");
    }

    std::vector<char> buffer(size + 1);
    std::vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);

    return std::string(buffer.data(), size);
}
