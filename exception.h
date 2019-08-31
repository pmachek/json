#ifndef _EXCEPTION_H_183719873
#define _EXCEPTION_H_183719873

#include <stdexcept>


namespace json {
class SyntaxError : public std::runtime_error {
    std::size_t streamPosition;
    char* formatted;

public:
    SyntaxError(const char* msg);
    SyntaxError(const char* msg, std::size_t streamPosition);

    virtual const char* what();

    virtual ~SyntaxError();
};

struct TypeError : public std::runtime_error {
    TypeError(const char* msg);
};

struct ValueError : public std::runtime_error {
    ValueError(const char* msg);
};

struct UnicodeError : public std::runtime_error {
    UnicodeError(const char* msg);
};
}

#endif // _EXCEPTION_H_183719873
