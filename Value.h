#ifndef _VALUE_H_183719873
#define _VALUE_H_183719873

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <type_traits>

#include "exception.h"

namespace json {
class Value {
public:
    enum class Type {
        NIL, STRING, INT, DOUBLE, BOOL, OBJECT, ARRAY, UNPARSED
    };
private:
    union {
        int64_t i;
        double d;
        bool b;
        std::string* str;
        std::unordered_map<std::string, Value>* object;
        std::vector<Value>* array;
        const char* bounds[2];
    };
    void destroy();
protected:
    Type type;
public:
    Value();
    Value(const double d);
    Value(const bool b);
    Value(const std::string& s);
    Value(const char *s);
    Value(std::string* s);
    Value(std::unordered_map<std::string, Value>* object);
    Value(const std::unordered_map<std::string, Value>& object);
    Value(std::vector<Value>* array);
    Value(const std::vector<Value>& array);
    Value(const std::initializer_list<Value>& array);
    Value(std::nullptr_t);
    Value(const char* start, const char* end);
    Value(Type t);
    Value(const Value& other) noexcept;
    Value(Value&& other) noexcept;

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T> > >
    Value(const T& other) : type(Type::INT), i(other) {}

    ~Value();

    Value& operator=(const Value& other) noexcept;
    Value& operator=(Value&& other) noexcept;

    Value& operator[](const std::size_t index) const;
    Value& operator[](const std::string& key) const;
    Value& operator[](const char* key) const;

    bool operator!=(const Value& other) const;

    bool operator==(const Value& other) const;
    bool operator==(const double d) const;

    template <
        typename T,
        std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, int> = 0
    >
    bool operator==(const T& other) const {
        switch (type) {
        case Type::INT:
            return other == i;
        case Type::DOUBLE:
            return other == d;
        case Type::BOOL:
            return other == static_cast<T>(b);
        default:
            throw TypeError("Integer comparison can be used only on int, double or bool");
        }
    }

    template <
        typename T,
        std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0
    >
    bool operator==(const T& other) const {
        switch (type) {
        case Type::INT:
            if (i < 0) {
                return false;
            }
            return other == i;
        case Type::DOUBLE:
            return other == d;
        case Type::BOOL:
            return other == static_cast<T>(b);
        default:
            throw TypeError("Integer comparison can be used only on int, double or bool");
        }
    }

    template <typename T>
    bool operator!=(const T& other) const {
        if (type == Type::DOUBLE && std::isnan(d)) {
            return false;
        }
        return !(*this == other);
    }

    template <
        typename T,
        typename = std::enable_if_t<std::is_integral_v<T> >
    >
    operator T() const {
        switch (type) {
        case Type::INT:
            return static_cast<T>(i);
        case Type::DOUBLE:
            return static_cast<T>(d);
        case Type::BOOL:
            return static_cast<T>(b);
        default:
            throw TypeError("Only int, double or bool can be cast to integral value");
        }
    }

    bool castBool() const;
    int64_t castInt() const;
    double castDouble() const;
    std::string castString() const;

    std::string toJson() const;
    std::string toJson2() const;
    void toJson(std::string& s) const;
    void toJson(char** memory, std::size_t& size, std::size_t& used) const;
    void toJsonIterative(std::string& s) const;

    Type getType() const;

    template <typename T>
    constexpr static Type detectType(T value);

    std::string& getString();
    std::vector<Value>& getArray();
    std::unordered_map<std::string, Value>& getObject();
    int64_t& getInt();
    double& getDouble();
    bool& getBool();

    std::string getTypeName() const;

#define DEF_BINARY_INT_OPERATOR(opSymbol)                                                     \
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T> > >               \
    Value operator opSymbol(const T& other) const {                                           \
        switch (type) {                                                                       \
        case Type::INT:                                                                       \
            return Value(i opSymbol static_cast<int64_t>(other));                             \
        case Type::BOOL:                                                                      \
            return Value(static_cast<T>(b) opSymbol other);                                   \
        default:                                                                              \
            throw TypeError("Binary operator '" #opSymbol "' can be used only on integer or bool");   \
        }                                                                                     \
    }
    DEF_BINARY_INT_OPERATOR(&)
    DEF_BINARY_INT_OPERATOR(|)
    DEF_BINARY_INT_OPERATOR(^)
    DEF_BINARY_INT_OPERATOR(%)
    DEF_BINARY_INT_OPERATOR(>>)
    DEF_BINARY_INT_OPERATOR(<<)
#undef DEF_BINARY_INT_OPERATOR

#define DEF_BINARY_NUM_OPERATOR(opSymbol)                                                   \
    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value > >      \
    Value operator opSymbol(const T& other) const {                                         \
        switch (type) {                                                                     \
        case Type::INT:                                                                     \
            /* if constexpr (std::is_integral_v<T>) */                                      \
            if (std::is_integral_v<T>) {                                                    \
                return Value(i opSymbol static_cast<int64_t>(other));                       \
            } else {                                                                        \
                return Value(static_cast<double>(i) opSymbol static_cast<double>(other));   \
            }                                                                               \
        case Type::DOUBLE:                                                                  \
            return Value(d opSymbol static_cast<double>(other));                            \
        case Type::BOOL:                                                                    \
            return Value(static_cast<T>(b) opSymbol other);                                 \
        default:                                                                            \
            throw TypeError("Binary operator '" #opSymbol "' can be used only on integer, float or bool");  \
        }                                                                                   \
    }
    DEF_BINARY_NUM_OPERATOR(+)
    DEF_BINARY_NUM_OPERATOR(-)
    DEF_BINARY_NUM_OPERATOR(*)
    DEF_BINARY_NUM_OPERATOR(/)
#undef DEF_BINARY_NUM_OPERATOR

#pragma warning(push)
#pragma warning(disable : 4804)
#define DEF_UNARY_OPERATOR(opSymbol)     \
    Value operator opSymbol() const {    \
        switch (type) {                  \
        case Type::INT:                  \
            return Value(opSymbol i);    \
        case Type::BOOL:                 \
            return Value(opSymbol b);    \
        case Type::DOUBLE:               \
            return Value(opSymbol d);    \
        default:                         \
            throw TypeError("Unary '" #opSymbol "' can be used only on integer or bool"); \
        }                                \
    }
    DEF_UNARY_OPERATOR(!)
    DEF_UNARY_OPERATOR(-)
#undef DEF_UNARY_OPERATOR
#pragma warning(pop)

    Value operator ~() const {
        switch (type) {
        case Type::INT:
            return Value(~i);
        case Type::BOOL:
            return Value(~static_cast<int64_t>(b));
        default:
            throw TypeError("Unary '~' can be used only on integer or bool");
        }
    }

#define DEF_INT_ASSIGNMENT_OPERATOR(opSymbol)                                    \
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T> > >  \
    Value& operator opSymbol##=(const T& other) {                                \
        switch (type) {                                                          \
        case Type::INT:                                                          \
            i opSymbol##= other;                                                 \
            break;                                                               \
        case Type::BOOL:                                                         \
            b opSymbol##= other;                                                 \
            break;                                                               \
        default:                                                                 \
            throw TypeError("Binary operator '" #opSymbol "' can be used only on integer or bool");   \
        }                                                                        \
        return *this;                                                            \
    }
    DEF_INT_ASSIGNMENT_OPERATOR(&)
    DEF_INT_ASSIGNMENT_OPERATOR(|)
    DEF_INT_ASSIGNMENT_OPERATOR(^)
    DEF_INT_ASSIGNMENT_OPERATOR(%)
    DEF_INT_ASSIGNMENT_OPERATOR(>>)
    DEF_INT_ASSIGNMENT_OPERATOR(<<)
#undef DEF_INT_ASSIGNMENT_OPERATOR

#define DEF_NUM_ASSIGNMENT_OPERATOR(opSymbol)                                     \
    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value > > \
    Value& operator opSymbol##=(const T& other) {                                 \
        switch (type) {                                                           \
        case Type::INT:                                                           \
            i opSymbol##= other;                                                  \
            break;                                                                \
        case Type::DOUBLE:                                                        \
            d opSymbol##= static_cast<double>(other);                             \
            break;                                                                \
        default:                                                                  \
            throw TypeError("Binary operator '" #opSymbol "' can be used only on integer or double");  \
        }                                                                         \
        return *this;                                                             \
    }
    DEF_NUM_ASSIGNMENT_OPERATOR(+)
    DEF_NUM_ASSIGNMENT_OPERATOR(-)
    DEF_NUM_ASSIGNMENT_OPERATOR(*)
    DEF_NUM_ASSIGNMENT_OPERATOR(/)
#undef DEF_NUM_ASSIGNMENT_OPERATOR

#define DEF_INCREMENT_DECREMENT(opSymbol) \
    Value& operator opSymbol() { \
        if (type != Type::INT) { \
            throw TypeError("Unary operator '" #opSymbol "' can be used only on integer"); \
        }                        \
        opSymbol i;              \
        return *this;            \
    }                            \
    Value operator opSymbol(int) { \
        if (type != Type::INT) {   \
            throw TypeError("Unary operator '" #opSymbol "' can be used only on integer"); \
        }                          \
        int64_t oldI = i;          \
        i opSymbol;                \
        return Value(oldI);        \
    }
    DEF_INCREMENT_DECREMENT(++)
    DEF_INCREMENT_DECREMENT(--)
};
}

#endif // _VALUE_H_183719873
