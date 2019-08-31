#ifndef _PARSE_H_183719873
#define _PARSE_H_183719873

#include <cctype>
#include <string>

#include "value.h"

namespace json {
Value parseObject(const std::string& str);
Value parse(const std::string& str);
Value parseNonrecursive(const std::string& str);
}

#endif // _PARSE_H_183719873
