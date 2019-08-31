#include "value.h"
#include <cstring>

#ifndef DBL_DECIMAL_DIG
#define DBL_DECIMAL_DIG 17
#endif


namespace json {
Value::Value() : type(Type::NIL), i(0) {}
Value::Value(const double d) : type(Type::DOUBLE), d(d) {}
Value::Value(const bool b) : type(Type::BOOL), b(b) {}
Value::Value(const std::string& s) : type(Type::STRING), str(new std::string(s)) {}
Value::Value(const char* s) : type(Type::STRING), str(new std::string(s)) {}
Value::Value(std::string* s) : type(Type::STRING), str(s) {}
Value::Value(std::unordered_map<std::string, Value>* object) : type(Type::OBJECT), object(object) {}
Value::Value(const std::unordered_map<std::string, Value>& object) : type(Type::OBJECT), object(new std::unordered_map<std::string, Value>(object)) {}
Value::Value(std::vector<Value>* array) : type(Type::ARRAY), array(array) {}
Value::Value(const std::vector<Value>& array) : type(Type::ARRAY), array(new std::vector<Value>(array)) {}
Value::Value(const std::initializer_list<Value>& array) : type(Type::ARRAY), array(new std::vector<Value>(array)) {}
Value::Value(std::nullptr_t) : type(Type::NIL), i(0) {}
Value::Value(const char* start, const char* end) : type(Type::UNPARSED), bounds{ start, end } {}

Value::Value(Type t) : type(t), i(0) {
    switch (t) {
    case Type::ARRAY:
        array = new std::vector<Value>();
        break;
    case Type::OBJECT:
        object = new std::unordered_map<std::string, Value>();
        break;
    case Type::STRING:
        str = new std::string();
        break;
    default:
        // nothing to do here, zero initialization was enough
        break;
    }
}

Value::Value(Value&& other) noexcept : type(Type::NIL), i(0) {
    // std::cout << "Move constructor\n";
    this->type = other.type;
    switch (other.type) {
    case Type::STRING:
        str = other.str;
        other.str = nullptr;
        break;
    case Type::INT:
        i = other.i;
        break;
    case Type::DOUBLE:
        d = other.d;
        break;
    case Type::BOOL:
        b = other.b;
        break;
    case Type::OBJECT:
        object = other.object;
        other.object = nullptr;
        break;
    case Type::ARRAY:
        array = other.array;
        other.array = nullptr;
        break;
    case Type::NIL:
        break;
    case Type::UNPARSED:
        bounds[0] = other.bounds[0];
        bounds[1] = other.bounds[1];
        break;
    }
}

Value::Value(const Value& other) noexcept : type(Type::NIL), i(0) {
    // std::cout << "Copy constructor\n";
    this->type = other.type;
    switch (other.type) {
    case Type::STRING:
        str = new std::string(*other.str);
        break;
    case Type::INT:
        i = other.i;
        break;
    case Type::DOUBLE:
        d = other.d;
        break;
    case Type::BOOL:
        b = other.b;
        break;
    case Type::OBJECT:
        object = new std::unordered_map<std::string, Value>(*other.object);
        break;
    case Type::ARRAY:
        array = new std::vector<Value>(*other.array);
        break;
    case Type::NIL:
        break;
    case Type::UNPARSED:
        bounds[0] = other.bounds[0];
        bounds[1] = other.bounds[1];
        break;
    }
}

Value& Value::operator=(Value&& other) noexcept {
    // std::cout << "Reassign by move\n";
    destroy();
    this->type = other.type;

    switch (other.type) {
    case Type::STRING:
        str = other.str;;
        other.str = nullptr;
        break;
    case Type::INT:
        i = other.i;
        break;
    case Type::DOUBLE:
        d = other.d;
        break;
    case Type::BOOL:
        b = other.b;
        break;
    case Type::OBJECT:
        object = other.object;
        other.object = nullptr;
        break;
    case Type::ARRAY:
        array = other.array;
        other.array = nullptr;
        break;
    case Type::NIL:
        i = 0;
        break;
    case Type::UNPARSED:
        bounds[0] = other.bounds[0];
        bounds[1] = other.bounds[1];
        break;
    }

    return *this;
}

Value& Value::operator=(const Value& other) noexcept {
    // std::cout << "Reassign by copy\n";
    destroy();
    this->type = other.type;

    switch (other.type) {
    case Type::STRING:
        str = new std::string(*other.str);
        break;
    case Type::INT:
        i = other.i;
        break;
    case Type::DOUBLE:
        d = other.d;
        break;
    case Type::BOOL:
        b = other.b;
        break;
    case Type::OBJECT:
        object = new std::unordered_map<std::string, Value>(*other.object);
        break;
    case Type::ARRAY:
        array = new std::vector<Value>(*other.array);
        break;
    case Type::NIL:
        i = 0;
        break;
    case Type::UNPARSED:
        bounds[0] = other.bounds[0];
        bounds[1] = other.bounds[1];
        break;
    }

    return *this;
}

Value::~Value() {
    // std::cout << "Destructor (" << getTypeName() << ")\n";
    destroy();
}

Value& Value::operator[](const std::size_t index) const {
    if (type != Type::ARRAY)
        throw TypeError("Integer indexing can be used only on array");
    return (*array)[index];
}

Value& Value::operator[](const std::string& key) const {
    if (type != Type::OBJECT)
        throw TypeError("String indexing can be used only on object");
    return (*object)[key];
}

Value& Value::operator[](const char* key) const {
    if (type != Type::OBJECT)
        throw TypeError("String indexing can be used only on object");
    return (*object)[key];
}

bool Value::operator==(const Value& other) const {
    switch (this->type) {
    case Type::NIL:
        // compare NIL as nullptr
        if (other.type == Type::NIL)
            return true;
        if (other.type == Type::ARRAY || other.type == Type::OBJECT || other.type == Type::STRING)
            return false;
        throw TypeError("Can't compare null to int, float or bool");
    case Type::INT:
        if (other.type == Type::INT)
            return i == other.i;
        if (other.type == Type::DOUBLE)
            return static_cast<double>(i) == other.d;
        if (other.type == Type::BOOL)
            return i == (other.b ? 1.0 : 0.0);
        throw TypeError("Can't compare int to anything but int, float or bool");
    case Type::STRING:
        if (other.type == Type::NIL)
            return false;
        if (other.type == Type::STRING)
            return *str == *other.str;
        throw TypeError("Can't compare string to anything but string");
    case Type::DOUBLE:
        if (other.type == Type::DOUBLE)
            return d == other.d;
        if (other.type == Type::INT)
            return d == static_cast<double>(other.i);
        if (other.type == Type::BOOL)
            return other.b ? d == 1.0 : d == 0.0;
        throw TypeError("Can't compare float to anything but int, float or bool");
    case Type::BOOL:
        if (other.type == Type::DOUBLE)
            return b ? other.d == 1.0 : other.d == 0.0;
        if (other.type == Type::INT)
            return (int64_t)b == other.i;
        if (other.type == Type::BOOL)
            return b == other.b;
        throw TypeError("Can't compare bool to anything but int, float or bool");
    case Type::ARRAY:
        if (other.type == Type::NIL)
            return false;
        if (other.type == Type::ARRAY)
            return *array == *other.array;
        throw TypeError("Can't compare array to anything but array or null");
    case Type::OBJECT:
        if (other.type == Type::NIL)
            return false;
        if (other.type == Type::OBJECT)
            return *object == *other.object;
        throw TypeError("Can't compare object to anything but object or null");
    default:
        throw TypeError("Invalid type in comparison");
    }
}

bool Value::operator==(const double d) const {
    switch (type) {
    case Type::INT:
        return static_cast<double>(i) == d;
    case Type::DOUBLE:
        return this->d == d;
    case Type::BOOL:
        return d == (b ? 1.0 : 0.0);
    default:
        throw TypeError("Double comparison can be used only on int, double or bool");
    }
}

bool Value::operator!=(const Value& other) const {
    if (type == Type::DOUBLE && std::isnan(d)) {
        return false;
    }
    if (other.type == Type::DOUBLE && std::isnan(other.d)) {
        return false;
    }
    return !(*this == other);
}

void Value::destroy() {
    switch (type) {
    case Type::STRING:
        // str.~basic_string();
        delete str;
        break;
    case Type::ARRAY:
        // array.~vector();
        delete array;
        break;
    case Type::OBJECT:
        // object.~unordered_map();
        delete object;
        break;
    default:
        break;
    }
}

bool Value::castBool() const {
    // works the same way as Python bool conversion
    switch (type) {
    case Type::NIL:
        return false;
    case Type::INT:
        return i;
    case Type::DOUBLE:
        return d;
    case Type::STRING:
        return !str->empty();
    case Type::ARRAY:
        return !array->empty();
    case Type::OBJECT:
        return !object->empty();
    case Type::BOOL:
        return b;
    default:
        throw TypeError("Can't convert to bool");
    }
}

int64_t Value::castInt() const {
    switch (type) {
    case Type::BOOL:
        return b;
    case Type::INT:
        return i;
    case Type::DOUBLE:
        return (int64_t)d;
    default:
        throw TypeError("Can't convert to int");
    }
}

double Value::castDouble() const {
    switch (type) {
    case Type::BOOL:
        return b;
    case Type::INT:
        return (double)i;
    case Type::DOUBLE:
        return d;
    default:
        throw TypeError("Can't convert to double");
    }
}

std::string Value::castString() const {
    switch (type) {
    case Type::BOOL:
        return b ? "true" : "false";
    case Type::INT:
        return std::to_string(i);
    case Type::DOUBLE:
        return std::to_string(d);
    case Type::NIL:
        return "null";
    case Type::ARRAY:
        return toJson();
    case Type::OBJECT:
        return toJson();
    default:
        throw TypeError("Can't convert to string");
    }
}

std::string Value::toJson() const {
    std::string s;
    toJson(s);
    return s;
}

// TODO:
// - options of unicode escaping - always escape, do not escape
// - options of output formatting
//		- no extra spaces
//		- pretty printing
//		- formatting object, that will decide on indentation
//			based on object depth, key and value lengths,
//			number of elements in an array, maximum key length,
//			maximum value length, etc
// - Infinity encoding options
// - NaN encoding options
// - improve speed
void Value::toJson(std::string& s) const {
    char buffer[32];
    // whether element is first element of array or object.
    // can't be defined in 'case'
    bool first = true;

    switch (type) {
    case Type::NIL:
        s.append("null");
        break;
    case Type::INT:
            std::snprintf(buffer, 32, "%lli", static_cast<long long int>(i));
        s.append(buffer);
        break;
    case Type::DOUBLE:
        if (std::isnan(d)) {
            throw ValueError("No way to serialize NaN value");
        } else if (std::isinf(d)) {
            // TODO: better infinity handling
            if (d > 0.0) {
                s.append("1e1000");
            } else {
                s.append("-1e1000");
            }
        } else {
            std::snprintf(buffer, 32, "%.*g", DBL_DECIMAL_DIG, d);
            s.append(buffer);
        }
        break;
    case Type::STRING:
        s.push_back('"');
        for (char c : *str) {
            switch (c) {
            case '\\':
                s.push_back('\\');
                s.push_back('\\');
                break;
            case '"':
                s.push_back('\\');
                s.push_back('"');
                break;
            case '\n':
                s.push_back('\\');
                s.push_back('n');
                break;
            case '\t':
                s.push_back('\\');
                s.push_back('t');
                break;
            case '\r':
                s.push_back('\\');
                s.push_back('r');
                break;
            case '\f':
                s.push_back('\\');
                s.push_back('f');
                break;
            case '\b':
                s.push_back('\\');
                s.push_back('b');
                break;
            default:
                if (c >= 0 && c < 32) {
                    buffer[0] = '\\';
                    buffer[1] = 'u';
                    buffer[2] = '0';
                    buffer[3] = '0';
                    buffer[4] = '0' + (c >> 4);
                    buffer[5] = ((c & 15) < 10) ? ('0' + (c & 15)) : ('a' - 10 + (c & 15));
                    buffer[6] = '\0';
                    s.append(buffer);
                } else {
                    s.push_back(c);
                }
            }
        }
        s.push_back('"');
        break;
    case Type::BOOL:
        s.append(b ? "true" : "false");
        break;
    case Type::ARRAY:
        s.push_back('[');
        for (const Value& val : *array) {
            if (!first) s.push_back(',');
            first = false;
            val.toJson(s);
        }
        s.push_back(']');
        break;
    case Type::OBJECT:
        s.push_back('{');
        for (const auto& item : *object) {
            if (!first) s.push_back(',');
            first = false;
            Value(item.first).toJson(s);
            s.push_back(':');
            item.second.toJson(s);
        }
        s.push_back('}');
        break;
    case Type::UNPARSED:
        // TODO: this case
        break;
    }
}

std::string Value::toJson2() const {
    char* ptr;
    std::size_t size = 0;
    std::size_t used = 0;
    toJson(&ptr, size, used);
    std::string s(ptr, used);
    free(ptr);
    return s;
}

// TODO:
// - options of unicode escaping - always escape, do not escape
// - options of output formatting
//		- no extra spaces
//		- pretty printing
//		- formatting object, that will decide on indentation
//			based on object depth, key and value lengths,
//			number of elements in an array, maximum key length,
//			maximum value length, etc
// - Infinity encoding options
// - NaN encoding options
// - improve speed
void Value::toJson(char** memory, std::size_t& size, std::size_t& used) const {
    if (size == 0) {
        size = 4096;
        used = 0;
        *memory = static_cast<char*>(malloc(size));
    }
    if (size - used < 32) {
        size *= 2;
        *memory = static_cast<char*>(realloc(*memory, size));
    }
    // whether element is first element of array or object.
    // can't be defined in 'case'
    bool first = true;

    switch (type) {
    case Type::NIL:
        std::memcpy(*memory + used, "null", 4);
        used += 4;
        break;
    case Type::INT: {
        int64_t v = i;
        if (i == 0) {
            (*memory)[used++] = '0';
            break;
        } else if (i < 0) {
            (*memory)[used++] = '-';
            v = -i;
        }
        char buffer[24];
        char* bufferEnd = buffer + 23;
        // TODO: handle i == std::numeric_limits<int64_t>::min()
        while (v != 0) {
            (*bufferEnd--) = '0' + (v % 10);
            v /= 10;
        }
        auto numberSize = 23 - (bufferEnd - buffer);
        std::memcpy(*memory + used, bufferEnd + 1, numberSize);
        used += numberSize;
        break;
    }
    case Type::DOUBLE:
        if (std::isnan(d)) {
            throw ValueError("No way to serialize NaN value");
        } else if (std::isinf(d)) {
            // TODO: better infinity handling
            if (d > 0.0) {
                std::memcpy(*memory + used, "1e1000", 6);
                used += 6;
            } else {
                std::memcpy(*memory + used, "-1e1000", 7);
                used += 7;
            }
        } else {
            used += std::snprintf(*memory + used, 32, "%.*g", DBL_DECIMAL_DIG, d);
        }
        break;
    case Type::STRING:
        (*memory)[used++] = '"';
        for (char c : *str) {
            switch (c) {
            case '\\':
                (*memory)[used++] = '\\';
                (*memory)[used++] = '\\';
                break;
            case '"':
                (*memory)[used++] = '\\';
                (*memory)[used++] = '"';
                break;
            case '\n':
                (*memory)[used++] = '\\';
                (*memory)[used++] = 'n';
                break;
            case '\t':
                (*memory)[used++] = '\\';
                (*memory)[used++] = 't';
                break;
            case '\r':
                (*memory)[used++] = '\\';
                (*memory)[used++] = 'r';
                break;
            case '\f':
                (*memory)[used++] = '\\';
                (*memory)[used++] = 'f';
                break;
            case '\b':
                (*memory)[used++] = '\\';
                (*memory)[used++] = 'b';
                break;
            default:
                if (c >= 0 && c < 32) {
                    std::memcpy(*memory + used, "\\u00", 4);
                    used += 4;
                    (*memory)[used++] = '0' + (c >> 4);
                    (*memory)[used++] = ((c & 15) < 10) ? ('0' + (c & 15)) : ('a' - 10 + (c & 15));
                } else {
                    (*memory)[used++] = c;
                }
            }
            if (size - used < 32) {
                size *= 2;
                *memory = static_cast<char*>(realloc(*memory, size));
            }
        }
        (*memory)[used++] = '"';
        break;
    case Type::BOOL:
        if (b) {
            std::memcpy(*memory + used, "true", 4);
            used += 4;
        } else {
            std::memcpy(*memory + used, "false", 5);
            used += 5;
        }
        break;
    case Type::ARRAY:
        (*memory)[used++] = '[';
        for (const Value& val : *array) {
            if (!first) (*memory)[used++] = ',';
            first = false;
            val.toJson(memory, size, used);
        }
        (*memory)[used++] = ']';
        break;
    case Type::OBJECT:
        (*memory)[used++] = '{';
        for (const auto& item : *object) {
            if (!first) (*memory)[used++] = ',';
            first = false;
            Value(item.first).toJson(memory, size, used);
            (*memory)[used++] = ':';
            item.second.toJson(memory, size, used);
        }
        (*memory)[used++] = '}';
        break;
    case Type::UNPARSED:
        // TODO: this case
        break;
    }
}

void Value::toJsonIterative(std::string& s) const {
    throw TypeError("Not implemented yet.");
    struct IterationState {
        union {
            std::vector<Value>::const_iterator vecIt;
            std::unordered_map<std::string, Value>::const_iterator mapIt;
        };
        union {
            std::vector<Value>::const_iterator vecEnd;
            std::unordered_map<std::string, Value>::const_iterator mapEnd;
        };
        bool isArrayIt;
    };

    char buffer[32];
    // whether element is first element of array or object.
    // can't be defined in 'case'
    bool first = true;

    std::vector<IterationState> stack;
    const Value* value = this;

    while (stack.size()) {
        switch (value->type) {
        case Type::NIL:
            s.append("null");
            break;
        case Type::INT:
            std::snprintf(buffer, 32, "%lli", static_cast<long long int>(value->i));
            s.append(buffer);
            break;
        case Type::DOUBLE:
            if (std::isnan(value->d)) {
                throw ValueError("No way to serialize NaN value");
            } else if (std::isinf(value->d)) {
                // TODO: better infinity handling
                if (value->d > 0.0) {
                    s.append("1e1000");
                } else {
                    s.append("-1e1000");
                }
            } else {
                std::snprintf(buffer, 32, "%.*g", DBL_DECIMAL_DIG, value->d);
                s.append(buffer);
            }
            break;
        case Type::STRING:
            s.push_back('"');
            for (char c : *value->str) {
                switch (c) {
                case '\\':
                    s.push_back('\\');
                    s.push_back('\\');
                    break;
                case '"':
                    s.push_back('\\');
                    s.push_back('"');
                    break;
                case '\n':
                    s.push_back('\\');
                    s.push_back('n');
                    break;
                case '\t':
                    s.push_back('\\');
                    s.push_back('t');
                    break;
                case '\r':
                    s.push_back('\\');
                    s.push_back('r');
                    break;
                case '\f':
                    s.push_back('\\');
                    s.push_back('f');
                    break;
                case '\b':
                    s.push_back('\\');
                    s.push_back('b');
                    break;
                default:
                    if (c >= 0 && c < 32) {
                        buffer[0] = '\\';
                        buffer[1] = 'u';
                        buffer[2] = '0';
                        buffer[3] = '0';
                        buffer[4] = '0' + (c >> 4);
                        buffer[5] = ((c & 15) < 10) ? ('0' + (c & 15)) : ('a' - 10 + (c & 15));
                        buffer[6] = '\0';
                        s.append(buffer);
                    }
                    else {
                        s.push_back(c);
                    }
                }
            }
            s.push_back('"');
            break;
        case Type::BOOL:
            s.append(value->b ? "true" : "false");
            break;
        case Type::ARRAY:
            s.push_back('[');
            for (const Value& val : *(value->array)) {
                if (!first) s.push_back(',');
                first = false;
                val.toJson(s);
            }
            s.push_back(']');
            break;
        case Type::OBJECT:
            s.push_back('{');
            for (const auto& item : *(value->object)) {
                if (!first) s.push_back(',');
                first = false;
                Value(item.first).toJson(s);
                s.push_back(':');
                item.second.toJson(s);
            }
            s.push_back('}');
            break;
        case Type::UNPARSED:
            // TODO: this case
            break;
        }
    }
}

Value::Type Value::getType() const {
    return type;
}

template <>
Value::Type Value::detectType(int) { return Type::INT; }

template <>
Value::Type Value::detectType(int64_t) { return Type::INT; }

template <>
Value::Type Value::detectType(double) { return Type::DOUBLE; }

template <>
Value::Type Value::detectType(float) { return Type::DOUBLE; }

template <>
Value::Type Value::detectType(const std::string&) { return Type::STRING; }

template <>
Value::Type Value::detectType(const std::vector<Value>*) { return Type::ARRAY; }

template <>
Value::Type Value::detectType(const std::vector<Value>&) { return Type::ARRAY; }

template <>
Value::Type Value::detectType(const std::unordered_map<std::string, Value>*) { return Type::OBJECT; }

template <>
Value::Type Value::detectType(const std::unordered_map<std::string, Value>&) { return Type::OBJECT; }

template <>
Value::Type Value::detectType(bool) { return Type::BOOL; }

template <>
Value::Type Value::detectType(std::nullptr_t) { return Type::NIL; }

/*
template <typename T>
constexpr Value::Type Value::detectType(T t) {
static_assert(false, "No type conversion to json::Value type");
return Type::NIL;
}
*/

std::string& Value::getString() {
    if (type != Type::STRING)
        throw TypeError("Value is not string");
    return *str;
}

std::vector<Value>& Value::getArray() {
    if (type != Type::ARRAY)
        throw TypeError("Value is not array");
    return *array;
}

std::unordered_map<std::string, Value>& Value::getObject() {
    if (type != Type::OBJECT)
        throw TypeError("Value is not object");
    return *object;
}

int64_t& Value::getInt() {
    if (type != Type::INT)
        throw TypeError("Value is not integer");
    return i;
}

double& Value::getDouble() {
    if (type != Type::DOUBLE)
        throw TypeError("Value is not float");
    return d;
}

bool& Value::getBool() {
    if (type != Type::BOOL)
        throw TypeError("Value is not bool");
    return b;
}
}
