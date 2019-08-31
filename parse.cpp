#include "parse.h"


class Stream {
private:
    const char* streamPtr;

public:
    Stream(const std::string& s) : streamPtr(s.c_str()) {}
    Stream(const char* s) : streamPtr(s) {}

    /**
    * Skip all ASCII whitespace characters
    */
    char skipSpace() {
        while (isspace(*streamPtr)) streamPtr++;
        return *streamPtr;
    }

    /**
    * Skip to the end of string. If the string is not properly
    * closed with '"', skip up to the closing '\0' character.
    */
    char skipString() {
        while (*streamPtr != '"') {
            if (*streamPtr == '\\')
                streamPtr++;
            if (*streamPtr == '\0')
                break;
            streamPtr++;
        }
        return *streamPtr;
    }

    char skipNumber() {
        if (*streamPtr == '-') {
            streamPtr++;
        }
        while (isdigit(*streamPtr)) {
            streamPtr++;
        }
        if (*streamPtr != '.' && *streamPtr != 'e' && *streamPtr != 'E')
            return *streamPtr;
        if (*streamPtr == '.') {
            streamPtr++;
            while (isdigit(*streamPtr)) {
                streamPtr++;
            }
        }
        if (*streamPtr == 'e' || *streamPtr == 'E') {
            streamPtr++;
            if (*streamPtr == '+' || *streamPtr == '-') {
                streamPtr++;
            }
            while (isdigit(*streamPtr)) {
                streamPtr++;
            }
        }
        return *streamPtr;
    }

    const char* streamPointer() const {
        return streamPtr;
    }

    /**
    * Returns byte value at current position and advances a byte
    * forward, if the byte is not '\0'
    */
    char getChar() {
        char value = *streamPtr;
        if (value) streamPtr++;
        return value;
    }

    void rewindChar() {
        streamPtr--;
    }
};

namespace json {
    Value eagerParseString(Stream& stream) {
        // std::cout << "parse string\n";
        int i;			// index in unicode decoding loop
        uint32_t u;		// resulting unicode point decoded from \uXXXX sequences
        char c = stream.skipSpace();
        if (c != '"')
            throw SyntaxError("String must start with double quote");
        stream.getChar();		// skip the '"' character we already know of
        c = stream.getChar();
        std::string s;
        while (c != '"' && c != '\0') {
            if (c == '\\') {
                c = stream.getChar();
                switch (c) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case 'f':
                    s.push_back('\f');
                    break;
                case 'b':
                    s.push_back('\b');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                case '/':
                    s.push_back('/');
                    break;
                case 'u':
                    // Unicode decoding phase
                    u = 0;
                    do {
                        if (u != 0) {
                            if (stream.getChar() != '\\')
                                throw UnicodeError("Invalid unicode sequence, expected '\\'");
                            if (stream.getChar() != 'u')
                                throw UnicodeError("Invalid unicode sequence, expected 'u'");
                        }
                        for (i = 0; i < 4; i++) {
                            c = stream.getChar();
                            if (isdigit(c)) {
                                c -= '0';
                            } else if (c >= 'a' && c <= 'f') {
                                c -= 'a' - 10;
                            } else if (c >= 'A' && c <= 'F') {
                                c -= 'A' - 10;
                            } else {
                                throw SyntaxError("Expected charecter 0-9, a-f or A-F");
                            }
                            u = (u << 4) | c;
                        }
                    } while (u >= 0xD800 && u <= 0xDBFF); // if character must be encoded by two \uXXXX sequences
                    if (u >= 0xDC00 && u <= 0xDBFF) {
                        throw UnicodeError("Encountered low surrogate, but no preceding high surrogate");
                    }
                    if (u > 0xFFFF) {
                        u -= 0xD800DC00UL;
                        if ((u & 0xFFFF) > 0x3FF)
                            throw UnicodeError("Invalid low surrogate value");
                        u = ((u & 0xFFFF0000UL) >> 6) | (u & 0xFFFF) | 0x10000;
                    }
                    if (u <= 0x7F) {
                        s.push_back((char)u);
                    } else if (u <= 0x7FF) {
                        s.push_back(0xC0 | (u >> 6));
                        s.push_back(0x80 | (u & 0x3F));
                    } else if (u <= 0xFFFF) {
                        s.push_back(0xE0 | (u >> 12));
                        s.push_back(0x80 | ((u >> 6) & 0x3F));
                        s.push_back(0x80 | (u & 0x3F));
                    } else {
                        s.push_back(0xF0 | (u >> 18));
                        s.push_back(0x80 | ((u >> 12) & 0x3F));
                        s.push_back(0x80 | ((u >> 6) & 0x3F));
                        s.push_back(0x80 | (u & 0x3F));
                    }
                    // end of unicode sequence decoding
                    break;
                default:
                    throw SyntaxError("Invalid escape sequence");
                } // end of escape character switch ()
            } else {
                if (c >= 0 && c < 32)
                    throw SyntaxError("Characters with values 0-31 must be escaped within string");
                s.push_back(c);
            }
            c = stream.getChar();
        }
        if (c != '"')
            throw SyntaxError("String must end with unescaped '\"' character");
        return s;
    }

    Value parseNumber(Stream& stream) {
        /*
        Parse a number, either floating point or integer.

        If the result is supposed to be integer, parses integer or
        throws error if integer does not fit into int64_t.

        If the result is supposed to be double (value includes '.' or 'e'/'E'
        characters), parses double. Parser takes great care about rounding,
        so the values should typically be the closest possible double
        representations. It should parse correctly even absurdly large
        inputs (theoretically up to ~100MB, but test only tries 1MB float)

        Very large floating point values might be parsed to infinity,
        but does not parse "inf", "infinity", "NaN" or other as a valid
        input (JSON does not specify such values).
        */
// largest number that will surely handle appending a digit without overflow
#define WONT_OVERFLOW ((UINT64_MAX - 9) / 10) 

        bool sign = false;		// minus sign was present
        bool point = false;		// decimal point was present
        bool e = false;			// e/E character was present
        bool e_sign = false;	// minus sign of exponent was present
        int32_t e_modifier = 0;	// correction for parsed exponent
        int32_t exponent = 0;	// parsed value of exponent (after 'e/E' character)
        uint64_t significand = 0;

        char c = stream.getChar();
        // optional minus character
        if (c == '-') {
            sign = true;
            c = stream.getChar();
        }

        if (!isdigit(c))
            throw SyntaxError("There should be digit");

        if (c == '0') {
            // single zero...
            c = stream.getChar();
        } else {
            // ...or a group of digits
            while (isdigit(c)) {
                if (significand <= WONT_OVERFLOW) {
                    significand *= 10;
                    significand += c - '0';
                } else {
                    // result would not fit into 64bit integer. If result turns out
                    // to be float, just adjust exponent. If it turns out to
                    // be int, throw an error.
                    e_modifier++;
                }
                c = stream.getChar();
            }
        }

        // there may be a decimal point and at least one digit. Or no decimal point.
        if (c == '.') {
            point = true;
            c = stream.getChar();
            if (!isdigit(c))
                throw SyntaxError("There must be at least one digit after decimal point");
            while (isdigit(c)) {
                if (significand <= WONT_OVERFLOW) {
                    significand *= 10;
                    significand += c - '0';
                    e_modifier--;
                } // else { just drop those decimal places, 64-bit double can't hold it anyway }
                c = stream.getChar();
            }
        }

        // there may be exponent, +/- and some digits
        if (c == 'e' || c == 'E') {
            e = true;
            c = stream.getChar();
            if (c == '-' || c == '+') {
                e_sign = (c == '-');
                c = stream.getChar();
            }
            if (!isdigit(c))
                throw SyntaxError("There must be digits in exponent part (after e or E)");
            while (isdigit(c)) {
                // parse whole exponent, but don't let it overflow.
                // IEEE754 double can't express exponent larger than 1023,
                // but for very long numbers (think kilobytes, megabytes) 
                // the exponent might be balanced with sheer number of specified
                // digits, i.e. "0.0 ... thousand zeroes ... 01e1000" is the same
                // value as "0.001"
                if (exponent < 100000000) {
                    exponent *= 10;
                    exponent += c - '0';
                }
                c = stream.getChar();
            }
            if (e_sign)
                exponent = -exponent;
        }

        if (c) {
            // one extra character was read, unless reading
            // got to the end of string (\0 character) which
            // did not allow reading past it and therefore should not be rewinded
            stream.rewindChar();
        }

        // number will be floating point
        if (point || e) {
            double result = static_cast<double>(significand);
            if (sign)
                result = -result;
            return result * pow(10, exponent + e_modifier);
            // number will be integer
        } else {
            int64_t signed_significand = significand;
            if (signed_significand < 0 || e_modifier != 0)
                throw SyntaxError("Integer can't be saved as 64-bit signed int");
            if (sign)
                signed_significand = -signed_significand;
            return signed_significand;
        }
#undef WONT_OVERFLOW
    }

    Value parseArray(Stream& stream);
    Value parseObject(Stream& stream);

    Value parseValue(Stream& stream) {
        // std::cout << "parse value\n";
        char c = stream.skipSpace();
        switch (c) {
        case 't':
            stream.getChar(); // skip the 't'
            if (stream.getChar() != 'r') throw SyntaxError("Expected 'true'");
            if (stream.getChar() != 'u') throw SyntaxError("Expected 'true'");
            if (stream.getChar() != 'e') throw SyntaxError("Expected 'true'");
            // std::cout << "parsed true\n";
            return Value(true);
        case 'f':
            stream.getChar(); // skip the 'f'
            if (stream.getChar() != 'a') throw SyntaxError("Expected 'false'");
            if (stream.getChar() != 'l') throw SyntaxError("Expected 'false'");
            if (stream.getChar() != 's') throw SyntaxError("Expected 'false'");
            if (stream.getChar() != 'e') throw SyntaxError("Expected 'false'");
            // std::cout << "parsed false\n";
            return Value(false);
        case 'n':
            stream.getChar(); // skip the 'n'
            if (stream.getChar() != 'u') throw SyntaxError("Expected 'null'");
            if (stream.getChar() != 'l') throw SyntaxError("Expected 'null'");
            if (stream.getChar() != 'l') throw SyntaxError("Expected 'null'");
            // std::cout << "parsed null\n";
            return Value();
        case '"':
            return eagerParseString(stream);
        case '{':
            return parseObject(stream);
        case '[':
            return parseArray(stream);
        default:
            if (c == '-' || isdigit(c))
                return parseNumber(stream);
            throw SyntaxError("Not a JSON value");
        }
    }

    Value parseValueNonrecursive(Stream& stream) {
        // std::cout << "parse value\n";
        // TODO: placement new operations, custom allocators
        Value result;
        Value* current = &result;
        std::vector<Value*> stack;
        do {
            char c = stream.skipSpace();
            switch (c) {
            case 't':
                stream.getChar(); // skip the 't'
                if (stream.getChar() != 'r') throw SyntaxError("Expected 'true'");
                if (stream.getChar() != 'u') throw SyntaxError("Expected 'true'");
                if (stream.getChar() != 'e') throw SyntaxError("Expected 'true'");
                // std::cout << "parsed true\n";
                *current = true;
                break;
            case 'f':
                stream.getChar(); // skip the 'f'
                if (stream.getChar() != 'a') throw SyntaxError("Expected 'false'");
                if (stream.getChar() != 'l') throw SyntaxError("Expected 'false'");
                if (stream.getChar() != 's') throw SyntaxError("Expected 'false'");
                if (stream.getChar() != 'e') throw SyntaxError("Expected 'false'");
                // std::cout << "parsed false\n";
                *current = false;
                break;
            case 'n':
                stream.getChar(); // skip the 'n'
                if (stream.getChar() != 'u') throw SyntaxError("Expected 'null'");
                if (stream.getChar() != 'l') throw SyntaxError("Expected 'null'");
                if (stream.getChar() != 'l') throw SyntaxError("Expected 'null'");
                // std::cout << "parsed null\n";
                *current = nullptr;
                break;
            case '"':
                *current = eagerParseString(stream);
                break;
            case '[':
                stream.getChar();
                c = stream.skipSpace();
                *current = Value(Value::Type::ARRAY);
                if (c == ']') {
                    stream.getChar(); // skip the ']'
                } else {
                    std::vector<Value>& val = current->getArray();
                    val.emplace_back(nullptr);
                    stack.push_back(current);
                    current = &val.back();
                }
                break;
            case '{':
                stream.getChar();
                c = stream.skipSpace();
                *current = Value(Value::Type::OBJECT);
                if (c == '}') {
                    stream.getChar(); // skip the '}'
                } else {
                    stack.push_back(current);
                    Value key = eagerParseString(stream);
                    current = &(*current)[key.getString()];
                    c = stream.skipSpace();
                    if (c != ':') {
                        throw SyntaxError("Key must be followed by ':' character");
                    }
                    c = stream.getChar();
                }
                break;
            case ',':
                if (stack.empty()) {
                    throw SyntaxError("',' character not allowed in this context");
                }
                stream.getChar();
                // TODO: save stack.back() to extra variable
                if (stack.back()->getType() == Value::Type::ARRAY) {
                    auto& array = stack.back()->getArray();
                    array.push_back(nullptr);
                    current = &array.back();
                } else {
                    Value key = eagerParseString(stream);
                    c = stream.skipSpace();
                    if (c != ':') {
                        throw SyntaxError("Key must be followed by ':' character");
                    }
                    stream.getChar(); // skip the ':' character
                    current = &((*stack.back())[key.getString()]);
                }
                break;
            case ']':
                if (stack.empty() || stack.back()->getType() != Value::Type::ARRAY) {
                    throw SyntaxError("']' character not allowed in this context");
                }
                stream.getChar();
                stack.pop_back();
                break;
            case '}':
                if (stack.empty() || stack.back()->getType() != Value::Type::OBJECT) {
                    throw SyntaxError("'}' character not allowed in this context");
                }
                stream.getChar();
                stack.pop_back();
                break;
            default:
                if (c == '-' || isdigit(c)) {
                    *current = parseNumber(stream);
                } else {
                    throw SyntaxError("Not a JSON value");
                }
            }
        } while (!stack.empty());
        return result;
    }

    Value parse(const std::string& s) {
        auto stream = Stream(s);
        Value v = parseValue(stream);
        char c = stream.skipSpace();
        if (c != '\0') {
            throw SyntaxError("Parsed a value, but data continues");
        }
        return v;
    }

    Value parseNonrecursive(const std::string& s) {
        auto stream = Stream(s);
        Value v = parseValueNonrecursive(stream);
        char c = stream.skipSpace();
        if (c != '\0')
            throw SyntaxError("Parsed a value, but data continues");
        return v;
    }

    Value parseObject(Stream& stream) {
        char c = stream.skipSpace();
        if (c != '{')
            throw SyntaxError("Object must start with '{'");
        auto object = new std::unordered_map<std::string, Value>();
        stream.getChar();
        c = stream.skipSpace();
        if (c == '}') {
            stream.getChar();
            return object;
        }
        while (c == '"') {
            Value name = eagerParseString(stream);
            c = stream.skipSpace();
            if (c != ':') {
                throw SyntaxError("Missing colon");
            }
            stream.getChar();
            c = stream.skipSpace();
            (*object)[name.getString()] = parseValue(stream);
            c = stream.skipSpace();
            if (c == ',') {
                stream.getChar();
                c = stream.skipSpace();
            }
        }
        if (c != '}')
            throw SyntaxError("Object must end with '}'");
        stream.getChar();
        return object;
    }

    Value parseObject(const std::string& s) {
        auto stream = Stream(s);
        return parseObject(stream);
    }

    Value parseArray(Stream& stream) {
        // std::cout << "parse array\n";
        char c = stream.skipSpace();
        if (c != '[')
            throw SyntaxError("Array must start with '[' character");
        stream.getChar();
        c = stream.skipSpace();
        auto array = new std::vector<Value>();
        while (c != ']') {
            array->push_back(parseValue(stream));
            c = stream.skipSpace();
            if (c == ',') {
                stream.getChar();
                c = stream.skipSpace();
            }
            else if (c != ']') {
                throw SyntaxError("Array must end with closing ']' character");
            }
        }
        stream.getChar();
        return array;
    }

    std::string Value::getTypeName() const {
        switch (type) {
        case Type::STRING:
            return "string";
        case Type::INT:
            return "int";
        case Type::DOUBLE:
            return "double";
        case Type::BOOL:
            return "bool";
        case Type::OBJECT:
            return "object";
        case Type::ARRAY:
            return "array";
        case Type::NIL:
            return "null";
        case Type::UNPARSED:
            return "unparsed";
        default:
            throw TypeError("Unknown type");
        }
    }

    Value lazyParseString(Stream& stream) {
        char c = stream.skipSpace();
        if (c != '"')
            throw SyntaxError("String must start with '\"' character");
        const char* start = stream.streamPointer();
        c = stream.skipString();
        if (c != '"')
            throw SyntaxError("String did not end with '\"' character");
        const char* end = stream.streamPointer();
        return Value(start, end);
    }

    Value lazyParseArray(Stream& stream);
    Value lazyParseValue(Stream& stream);

    Value lazyParseObject(Stream& stream) {
        char c = stream.skipSpace();
        if (c != '{')
            throw SyntaxError("Object must start with '{' character");
        auto object = new std::unordered_map<std::string, json::Value>();
        c = stream.skipSpace();
        if (c == '}') {
            stream.getChar();
            return Value(object);
        }
        while (c == '"') {
            Value name = eagerParseString(stream);
            c = stream.skipSpace();
            if (c != ':')
                throw SyntaxError("Missing colon");
            stream.getChar();
            c = stream.skipSpace();
            (*object)[name.getString()] = lazyParseValue(stream);
            c = stream.skipSpace();
            if (c == ',') {
                stream.getChar();
                c = stream.skipSpace();
            }
        }
        if (c != '}')
            throw SyntaxError("Object must end with '}'");
        stream.getChar();
        return object;
    }

    // TODO: implementovat
    Value lazyParseArray(Stream& stream) { return Value(std::vector<Value>()); }

    // TODO: implementovat
    Value lazyParseValue(Stream& stream) { return Value(); }
}
