#include "testing.h"


namespace json {
    void testDouble() {
        std::vector<std::string> testCases = {
            "123.321", "123.321e10", "1.0", "2E2", "2e+2", "3E-3", "-3E+3",
            "3.14159265358979323", "10000000000000000000000000e0", "10000000000000000000000000.0",
            "1e25", "1e1000", "1e-1000", "0.00000000000000000000000001",
            "0.00000000000000000000000000000000001e35",
            "1000000000000000000000000000000000000e-35",
            "-2.7182818284591E+120",
            "2.7182818284591e-120"
        };
        auto precision = std::cout.precision(20);
        for (auto& testCase : testCases) {
            std::cout << testCase << " = " << json::parse(testCase).getDouble() << std::endl;
        }
        std::cout.precision(precision);
    }


    void testInteger() {
        std::vector<std::string> testCases = { "0", "123", "9223372036854775807", "-100200" };
        for (auto& testCase : testCases) {
            std::cout << testCase
                << " = "
                << json::parse(testCase).getInt()
                << std::endl;
        }
    }


    void testAbsurdlyLongFloat() {
        std::cout << "parsing megabyte long numbers:\n";
        std::string s(1024 * 1024, '1'); // megabyte of '1'
        s.replace(s.end() - 10, s.end(), "e-1048565");
        std::cout << json::parse(s).getDouble() << std::endl;
        std::string s2(1024 * 1024, '2');
        s2[1] = '.';
        std::cout << json::parse(s2).getDouble() << std::endl;
        std::string s3(1024 * 1024, '3');
        s3[1024 * 1024 - 10] = '.';
        std::cout << json::parse(s3).getDouble() << std::endl;
        std::string s4(1024 * 1024, '4');
        s4[1] = 'e';
        std::cout << json::parse(s4).getDouble() << std::endl;
    }


    class JsonGenerator {
        std::default_random_engine generator;
        std::uniform_int_distribution<int> asciiDistribution;
        std::uniform_int_distribution<int> unicodeDistribution;
        std::uniform_int_distribution<int> digitDistribution;
        std::uniform_int_distribution<int> zeroNineDistribution;

    public:
        JsonGenerator() : generator(123456789), asciiDistribution(32, 126),
            unicodeDistribution(0, 0x10FFFF), digitDistribution('0', '9'),
            zeroNineDistribution(0, 9)
        {}

        void genString(std::string& s) {
            s.push_back('"');
            int count = 2 * zeroNineDistribution(generator);
            for (int i = 0; i < count; i++) {
                if (zeroNineDistribution(generator) < 2) {
                    // one in five will be unicode character
                    int u = unicodeDistribution(generator);
                    auto hexu = [](std::string& s, int x) {
                        s.push_back('\\');
                        s.push_back('u');
                        for (int i = 12; i >= 0; i -= 4) {
                            s.push_back("0123456789abcdef"[(x >> i) & 15]);
                        }
                    };
                    if (u >= 0xD800 && u <= 0xDFFF) {
                        u += 0x800;
                    }
                    if (u < 0xD7FF || (u >= 0xE000 && u <= 0xFFFF)) {
                        // single escape
                        hexu(s, u);
                    } else {
                        // high surrogate
                        hexu(s, (u >> 10) + 0xD800);
                        // low surrogate
                        hexu(s, (u & 0x3FF) + 0xDC00);
                    }
                }
                else {
                    char c = asciiDistribution(generator);
                    if (c != '\\' && c != '"')
                        s.push_back(c);
                    else {
                        s.push_back('\\');
                        s.push_back('\\');
                    }
                }
            }
            s.push_back('"');
        }

        void genInt(std::string& s) {
            int count = zeroNineDistribution(generator);
            char c = digitDistribution(generator);
            if (c == '0') {
                s.push_back('1');
            } else {
                s.push_back(c);
            }
            for (int i = 0; i < count; i++) {
                s.push_back(digitDistribution(generator));
            }
        }

        void genDouble(std::string& s) {
            if (zeroNineDistribution(generator) > 1) {
                genInt(s);
            } else {
                s.push_back('0');
            }
            int r = zeroNineDistribution(generator);
            if (r <= 6) {
                s.push_back('.');
                genInt(s);
            }
            if (r >= 3) {
                s.push_back('e');
                if (r == 4 || r == 7) {
                    s.push_back('+');
                }
                if (r == 5 || r == 8) {
                    s.push_back('-');
                }
                s.push_back(digitDistribution(generator));
                s.push_back(digitDistribution(generator));
            }
        }

        void genObject(std::string& s, int depth) {
            s.push_back('{');
            if (depth > 20) {
                s.push_back('}');
                return;
            }
            int count = 3 * zeroNineDistribution(generator);
            for (int i = 0; i < count; i++) {
                if (i != 0) {
                    s.push_back(',');
                }
                genString(s);
                if (zeroNineDistribution(generator) > 6) {
                    genSpace(s);
                }
                s.push_back(':');
                genAny(s, depth + 1);
            }
            s.push_back('}');
        }

        std::string genObject(int depth) {
            std::string s;
            genObject(s, depth);
            return s;
        }

        void genArray(std::string& s, int depth) {
            s.push_back('[');
            if (depth > 20) {
                s.push_back(']');
                return;
            }
            int count = 3 * zeroNineDistribution(generator);
            for (int i = 0; i < count; i++) {
                if (i != 0) {
                    s.push_back(',');
                }
                genAny(s, depth + 1);
            }
            s.push_back(']');
        }

        void genSpace(std::string& s) {
            int count = zeroNineDistribution(generator);
            char whitespaces[10] = { ' ', '\r', '\t', '\n', '\f', '\v', ' ', ' ', ' ', ' ' };
            for (int i = 0; i < count; i++) {
                s.push_back(whitespaces[zeroNineDistribution(generator)]);
            }
        }

        void genAny(std::string& s, int depth) {
            switch (zeroNineDistribution(generator)) {
            case 0:
                s.append("true");
                return;
            case 1:
                s.append("false");
                return;
            case 2:
                s.append("null");
                return;
            case 3:
                genString(s);
                return;
            case 4:
                genInt(s);
                return;
            case 5:
                genDouble(s);
                return;
            case 6:
                genObject(s, depth + 1);
                return;
            case 7:
                genArray(s, depth + 1);
                return;
            case 8:
                genSpace(s);
                genAny(s, depth + 1);
                return;
            default:
                genAny(s, depth + 1);
                genSpace(s);
                return;
            }
        }
    };


    /**
    * Generate few megabytes of JSON and have parser handle it.
    */
    void testGenerate() {
        JsonGenerator generator{};
        std::cout << "Generating random string...";
        std::string randomJson = generator.genObject(0);
        std::cout << " done" << std::endl;
        std::cout << randomJson.substr(0, 300) << "..." << std::endl;
        std::cout << "length: " << randomJson.size() << std::endl;
        auto start = std::chrono::system_clock::now();
        Value value = parse(randomJson);
        auto end = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsedSeconds = end - start;
        std::cout << std::setprecision(3);
        std::cout << "Decoding speed: "
            << (randomJson.size() / (elapsedSeconds.count() * 1e6))
            << "MB/s"
            << std::endl;
        std::cout << std::setprecision(6);

        start = std::chrono::system_clock::now();
        std::string randomJson2 = value.toJson2();
        end = std::chrono::system_clock::now();

        elapsedSeconds = end - start;
        std::cout << std::setprecision(3);
        std::cout << "Encoding speed: "
            << (randomJson2.size() / (elapsedSeconds.count() * 1e6))
            << "MB/s"
            << std::endl;
        std::cout << std::setprecision(6);

        Value longList{ Value::Type::ARRAY };
        std::vector<Value>& longListVector = longList.getArray();
        for (int i = 0; i < 1024 * 1024; ++i) {
            longListVector.emplace_back(i);
        }
        start = std::chrono::system_clock::now();
        randomJson2 = value.toJson2();
        end = std::chrono::system_clock::now();
        elapsedSeconds = end - start;
        std::cout << std::setprecision(3);
        std::cout << "Encoding speed for array of ints: "
            << (randomJson2.size() / (elapsedSeconds.count() * 1e6))
            << "MB/s"
            << std::endl;
        std::cout << std::setprecision(6);


        /*
        auto out = std::fstream("big.json", std::fstream::out);
        out << randomJson << std::endl;
        out.close();
        */
    }


    void testCreate() {
        using json::Value;
        Value();
        // Value("test");
        Value(1.0);
        Value({ { "key", Value(3.0) },{ "key2", Value(1) } });
        Value({ Value(), Value(), Value() });
        Value(int8_t(222));
        Value(uint8_t(222));
        Value(int16_t(222));
        Value(uint16_t(222));
        Value(int32_t(222));
        Value(uint32_t(222));
        Value(222);
        Value(222U);
        Value(222L);
        Value(222UL);
        Value(222LL);
        Value(222ULL);
        Value(true);
        Value(1.0f);

        Value v1 = 0;
        Value v2 = 0U;
        Value v3 = 0L;
        Value v4 = 0ULL;
        Value v5 = 0LL;
        Value v6 = true;
        Value v7 = 0.0f;
        Value v8 = 0.0;
        Value v9 = "test";
        Value v10 = std::vector<Value>{ 0, 1.0f, 0.0, "test" };
        Value v11 = std::unordered_map<std::string, Value>{
            {"a", 1},
            {"b", "test"}
        };

        int8_t zero8 = 0;
        uint8_t zero8u = 0;
        int16_t zero16 = 0;
        uint16_t zero16u = 0;
        int32_t zero32 = 0;
        uint32_t zero32u = 0;
        if (!(v1 == zero8 && v1 == zero16 && v1 == zero32 && v1 == 0 && v1 == 0L && v1 == 0LL)) {
            std::cout << "failed comparison to signed zero" << std::endl;
        }
        if (!(v1 == zero8u && v1 == zero16u && v1 == zero32u && v1 == 0U && v1 == 0UL && v1 == 0ULL)) {
            std::cout << "failed comparison to unsigned zero" << std::endl;
        }
    }

    void testSubscript() {
        Value array = std::vector<Value>{ "test", 2, 2.0 };
        Value object = std::unordered_map<std::string, Value>{
            { "key1", "value1" },
            { "key2", "value2" }
        };

        array[1] = 5.0;
        array[2] = "test";

        if (array[1] != 5.0) {
            std::cout << "wrong array access or comparison" << std::endl;
        }
        if (array[2] != "test") {
            std::cout << "wrong array access or comparison" << std::endl;
        }

        if (object["key1"] != "value1") {
            std::cout << "wrong object access or comparison" << std::endl;
        }
        if (object["key2"] != "value2") {
            std::cout << "wrong object access or comparison" << std::endl;
        }
        if (object["nonexistent"] != Value()) {
            std::cout << "wrong object access to nonexistent key" << std::endl;
        }
    }

    void testValidCases() {
        auto emptyMap = std::unordered_map<std::string, json::Value>();
        json::Value emptyObject(emptyMap);

        auto emptyVector = std::vector<json::Value>();
        json::Value emptyArray(emptyVector);

        std::string emptyStr;
        json::Value emptyString(emptyStr);

        std::vector<std::pair<std::string, json::Value>> validCases{
            { "[]", emptyArray },
            { "\"\"", emptyString },
            { "123", 123 },
            { "-123", -123 },
            { "1.0", 1.0 },
            { "-1.0", -1.0 },
            { "1e3", 1e3 },
            { "-1e3", -1e3 },
            { "1e+3", 1e+3 },
            { "1e-3", 1e-3 },
            { "1.5e-3", 1.5e-3 },
            { "-1e-3", -1e-3 },
            { "123.321", 123.321 },
            { "123.321E3", 123.321E3 },
            { "11.11e11", 11.11e11 },
            {
                "{\"a\":[]}",
                std::unordered_map<std::string, json::Value>{
                    { "a", emptyArray }
                }
            },
            { "true", true },
            { "false", false },
            { "null", json::Value() },
            { "\"\\r\\t\\n\"", "\r\t\n" },
            { "\"\\u000aaaaa\"", "\naaaa" },
            { "\"\\\\\"", "\\" },
            { "[ ]", emptyArray },
            { "[123.321]", std::vector<json::Value>{ 123.321 } },
            { "[123]", std::vector<json::Value>{ 123 } },
            { "[true]", std::vector<json::Value>{ true } },
            { "{}", emptyObject },
            {
                "{ \"a\" : true, \"b\": [] }",
                std::unordered_map<std::string, json::Value>{
                    { "a", json::Value(true)},
                    { "b", emptyArray }
                }
            },
            {
                "{\"a\": [ \"abc\", \"def\" ]}",
                std::unordered_map<std::string, json::Value>{
                    {
                        "a",
                        std::vector<json::Value>{"abc", "def"}
                    }
                }
            },
            {
                "[[], [ [ ]], [[[] ]], true,true, 1.3e11,false, null, \"\"]",
                std::vector<json::Value>{
                    emptyVector,
                    { emptyVector },
                    { { emptyVector } },
                    true,
                    true,
                    1.3e11,
                    false,
                    json::Value(),
                    ""
                }
            },
            { "\"\\ud801\\uDc37\"", "\xf0\x90\x90\xb7" }
        };
        for (auto& pair : validCases) {
            std::cout << "Test: " << pair.first << ": ";
            json::Value value = json::parseNonrecursive(pair.first);
            if (value == pair.second) {
                std::cout << "success\n";
            } else {
                std::cout << "failure. Expected type: "
                    << pair.second.getTypeName()
                    << " got: "
                    << value.getTypeName()
                    << std::endl;
                if (pair.second.getType() == json::Value::Type::STRING) {
                    std::cout << "content: "
                        << pair.first
                        << " "
                        << value.getString()
                        << std::endl;
                    auto s = value.getString();
                    std::cout << std::hex;
                    for (char c : s) {
                        std::cout << "\\x" << (int)c;
                    }
                    std::cout << std::endl << std::dec;
                }
            }
        }
    }


    void testInvalidCases() {
        std::vector<std::string> invalidCases{
            "00.5",
            "01",
            "\"abc",
            "{\"abc\" }",
            "{\"abc\": }",
            "[true",
            "[true,",
            "][",
            "turnip",
            "falsef",
            "-",
            "-1.",
            "-1..e",
            "1e-",
            "3.0E",
            "",
            "\"\\\"",
            "\"\\uD8DD\"",
            "9223372036854775808",
            ".34",
            "\"\\uabc\"",
            "\"\\uABC\"",
            "\"\\u000\""
        };
        for (auto& s : invalidCases) {
            std::cout << "Test: " << s << ": ";
            try {
                json::Value v = json::parse(s);
                std::cout << "parsed garbage. Type: " << v.getTypeName() << std::endl;
            } catch (json::SyntaxError& error) {
                std::cout << "success: " << error.what() << std::endl;
            } catch (json::UnicodeError& error) {
                std::cout << "success: " << error.what() << std::endl;
            }
        }
    }


    void testToJson() {
        const Value v = std::unordered_map<std::string, Value>{
            { "key", "value\r\t\n\f\b\x12\"\\" },
            { "key2", 111 },
            { "key3", true },
            { "key4", 222.222 },
            { "key5", std::vector<Value>{1, 2.0, false, "x" } },
            {
                "key6",
                std::unordered_map<std::string, Value>{
                    { "k", 3.14159265358979323846 },
                    { "k2", 2.7182818284591e120 }
                }
            },
            { "key7", std::vector<Value>{} },
            { "key8", std::unordered_map<std::string, Value>{} },
            { "key9", -111 },
            { "key10", -123456789 }
        };
        std::string s = v.toJson2();
        std::cout << "toJson: " << s << std::endl;
        const Value v2 = parse(s);
        std::cout << "serialized and parsed are equal: "
            << (v == v2 ? "true" : "false")
            << std::endl;
    }

    void testBinaryIntOps() {
        std::cout << "Testing binary int operators...";
        int64_t i33 = 33;
        Value val33 = i33;
        int64_t i = 22;
#define TEST_BINARY_OP(op)               \
    do {                                 \
        Value result = val33 op i;       \
        if (!(result == (i33 op i))) {   \
            std::cout << "binary '" #op "' operator implemented incorrectly" << std::endl;        \
            std::cout << "expected: " << (i33 op i) << ", got: " << result.getInt() << std::endl; \
        }                                \
    } while (0);
        TEST_BINARY_OP(&);
        TEST_BINARY_OP(|);
        TEST_BINARY_OP(^);
        TEST_BINARY_OP(%);
        TEST_BINARY_OP(>>);
        TEST_BINARY_OP(<<);
        TEST_BINARY_OP(+);
        TEST_BINARY_OP(-);
#undef TEST_BINARY_OP
        std::cout << " done." << std::endl;
    }

    void testModifyingIntOps() {
        std::cout << "Testing modifying int operators...";
        Value v = 123;
        if (v++ != 123) std::cout << "postfix ++ failed" << std::endl;
        if (v != 124) std::cout << "postfix ++ failed" << std::endl;
        if (++v != 125) std::cout << "prefix ++ failed" << std::endl;
        if (v != 125) std::cout << "prefix ++ failed" << std::endl;
        if (v-- != 125) std::cout << "postfix -- failed" << std::endl;
        if (v != 124) std::cout << "postfix -- failed" << std::endl;
        if (--v != 123) std::cout << "prefix -- failed" << std::endl;
        if (v != 123) std::cout << "prefix -- failed" << std::endl;
        if ((v += 2) != 125) std::cout << "+= failed" << std::endl;
        if ((v -= 2) != 123) std::cout << "-= failed" << std::endl;
        if ((v *= 2) != 246) std::cout << "*= failed" << std::endl;
        if ((v /= 2) != 123) std::cout << "/= failed" << std::endl;
        std::cout << " done." << std::endl;
    }

    void test() {
        std::cout << "Starting test:" << std::endl;
        testCreate();
        testSubscript();
        testValidCases();
        testInvalidCases();
        testDouble();
        testInteger();
        testAbsurdlyLongFloat();
        testToJson();
        testBinaryIntOps();
        testModifyingIntOps();
        testGenerate();
        std::cout << "tests done." << std::endl;
    }
}
