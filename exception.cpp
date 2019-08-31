#include "exception.h"
#include <cstring>


namespace json {
SyntaxError::SyntaxError(const char* msg) :
    std::runtime_error(msg), streamPosition(-1), formatted(nullptr)
{}

SyntaxError::SyntaxError(const char* msg, std::size_t streamPosition) :
    std::runtime_error(msg), streamPosition(streamPosition), formatted(nullptr)
{}

const char* SyntaxError::what() {
    const char* msg = std::runtime_error::what();
    if (streamPosition == -1) {
        return msg;
    }
    std::size_t allocLen = std::strlen(msg) + 40;
    formatted = new char[allocLen];
    std::snprintf(formatted, allocLen, msg, streamPosition);
    return formatted;
}

SyntaxError::~SyntaxError() {
    if (streamPosition != -1) delete[] formatted;
}

TypeError::TypeError(const char* msg) : std::runtime_error(msg) {}

ValueError::ValueError(const char* msg) : std::runtime_error(msg) {}

UnicodeError::UnicodeError(const char* msg) : std::runtime_error(msg) {}
}
