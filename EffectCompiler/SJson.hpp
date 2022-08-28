#pragma once
#include <iostream>
#include <exception>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include <tuple>
#include <functional>
#include <variant>
#include <initializer_list>
#include <cassert>
#include <type_traits>
#include <sstream>
#include <iomanip>

#include "SRefl.hpp"

#define NOMINMAX

namespace std
{
    template<typename Ty>
    constexpr const Ty& get(void* ptr)
    {
        return *reinterpret_cast<Ty*>(ptr);
    }
}

namespace SJson
{
    template<typename T>
    struct foobar : std::false_type
    {
    };

    // Type declarations
    // 
    class JsonNode;
    class JsonConvert;
    struct JsonToken;
    struct JsonFormatOption;

    enum class ValueType : uint8_t
    {
        Null,           // Null value: null
        Object,         // Object value: unordered set of name - value pairs
        Array,          // Array value: ordered array of values
        String,         // String value: "hello world"
        Boolean,        // Boolean value: true/false
        Integer,        // Integer value: long long int
        Float,          // Float value: floating points 3.14159

        __COUNT
    };

    enum class TokenType : uint8_t
    {
        Null,           // Token: "null"
        True,			// Token: "true"
        False,			// Token: "false"
        Integer,        // Token: (+|-)?[0-9]+ 
        Float,			// Token: (+|-)?[0-9]+.[0-9]+ / (+|-)?[0-9]+.[0-9]+e(+|-)?[0-9]+
        String,			// Token: "\".*\""
        LeftBrace,		// Token: "{"
        RightBrace,		// Token: "}"
        LeftBracket,	// Token: "["  
        RightBracket,   // Token: "]"
        Comma,			// Token: ","
        Colon,			// Token: ":"
        Comment,		// Token: "#"
        EndOfFile,		// Token: EOF
        Unknown,		// error token

        __COUNT
    };

    using array_type = std::vector<JsonNode>;
    using object_type = std::map<std::string, JsonNode>;
    using array_type_init = std::initializer_list<JsonNode>;
    using object_type_init = std::initializer_list<std::pair<const std::string, JsonNode>>;

    using JsonValue = std::variant<
        std::monostate,
        int64_t,
        double,
        bool,
        std::string,
        array_type,
        object_type
    >;

    template <typename T, typename U>
    using t_enable_if_same_type = std::enable_if_t<std::is_same<std::decay_t<T>, U>::value, nullptr_t>;

    template <typename T>
    using t_enable_if_integral_type = std::enable_if_t<std::is_integral<std::decay_t<T>>::value && !std::is_same<std::decay_t<T>, bool>::value, nullptr_t>;

    template <typename T>
    using t_enable_if_floating_type = std::enable_if_t<std::is_floating_point<std::decay_t<T>>::value, nullptr_t>;

    /**
     * @brief
     * @param type
     * @return
    */
    std::string GetValueTypeName(ValueType type);

    /**
     * @brief
     * @param list
     * @return
    */
    JsonNode object(std::initializer_list<std::pair<const std::string, JsonNode>> list);

    /**
     * @brief
     * @param list
     * @return
    */
    JsonNode array(std::initializer_list<JsonNode> list);


    /**
     * @brief
     * @param tokens
     * @param index
     * @param newIndex
     * @return
    */
    JsonNode parse(const std::vector<JsonToken>& tokens, int index, int& newIndex);

    /**
     * @brief 
     * @param type 
     * @return 
    */
    std::string GetTokenFromType(TokenType type);

    /**
     * @brief Get the formatted std::string
     * @tparam ...Args
     * @param format The same format as printf
     * @param ...args The same arguments as arguments of printf
     * @return Formatted string
    */
    template<typename ... Args>
    inline std::string string_format(const std::string& format, Args&& ... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
        if (size_s <= 0) { throw std::exception("Error during formatting."); }
        auto size = static_cast<size_t>(size_s);
        auto buf = std::make_unique<char[]>(size);
        std::snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }


    struct JsonToken
    {
        std::string						Value;
        TokenType						Token;
        int								Position;
        std::shared_ptr<std::string>	OriginalText;
    };


    class sjson_error_base : public std::exception
    {
    public:
        explicit sjson_error_base(const std::string& reason, const JsonToken& token)
        {
            int left = std::max(token.Position - 15, 0);
            int right = std::min(token.Position + 15, (int)token.OriginalText->size() - 1);
            std::string view = token.OriginalText->substr(left, right - left + 1);
            std::string viewptr = "";
            int offset = token.Position - left;
            for (int i = 0; i < offset; i++)
            {
                viewptr.push_back(' ');
            }
            viewptr.push_back('^');
            m_msg = string_format("Error: %s\nAt: %d\n%s\n%s", reason.c_str(), token.Position, view.c_str(), viewptr.c_str());
        }
        ~sjson_error_base() override {}

        const char* what() const override
        {
            return m_msg.c_str();
        }
    private:
        std::string m_msg;
    };

    class lexical_error : public sjson_error_base
    {
    public:
        lexical_error(const JsonToken& token)
            : sjson_error_base("Invalid token", token) {}
    };

    class expect_token_error : public sjson_error_base
    {
    public:
        expect_token_error(TokenType expect, TokenType actual, const JsonToken& token)
            : sjson_error_base(string_format("Expected to have %s but got %s instead",
                GetTokenFromType(expect).c_str(), GetTokenFromType(actual).c_str()), token)
        {
        }
    };

    class keys_not_string : public sjson_error_base
    {
    public:
    public:
        keys_not_string(const JsonToken& token)
            : sjson_error_base("Keys is not string", token)
        {
        }
    };

    class invalid_escape_char : public sjson_error_base
    {
    public:
    public:
        invalid_escape_char(const JsonToken& token)
            : sjson_error_base("Invalid escape character", token)
        {
        }
    };

    class invalid_eof : public sjson_error_base
    {
    public:
    public:
        invalid_eof(const JsonToken& token)
            : sjson_error_base("Unexpected EOF character", token)
        {
        }
    };

    class parse_match_failed : public sjson_error_base
    {
    public:
    public:
        parse_match_failed(const JsonToken& token)
            : sjson_error_base("Failed to match this token", token)
        {
        }
    };

    class root_not_singular_error : public sjson_error_base
    {
    public:
    public:
        root_not_singular_error(const JsonToken& token)
            : sjson_error_base("Root not singular", token)
        {
        }
    };


    class JsonConvert
    {
    public:
        static JsonNode Parse(const std::string& text);

        template<typename T>
        static std::string Serialize(const T& v, const JsonFormatOption& option);

        template<typename T>
        static T Deserialize(const std::string jsonStr);
    };

    struct JsonFormatOption
    {
        bool		Inline;				// Whether we should indent and make new-line when necessary
        bool		UseTab;				// True if we use tab to indent
        bool		KeysWithQuotes;		// True if we want keys to have quotes
    };

    const JsonFormatOption DefaultOption = { true, false, false };
    const JsonFormatOption InlineWithQuoteOption = { true, false, true };
    const JsonFormatOption DocumentOption = { false, false, true };

    // using JsonValue = void*;
    class JsonNode
    {
    public:
        JsonNode();
        template<typename T, t_enable_if_same_type<T, bool> = nullptr>
        JsonNode(T value);
        template<typename T, t_enable_if_integral_type<T> = nullptr>
        JsonNode(T value);
        template<typename T, t_enable_if_floating_type<T> = nullptr>
        JsonNode(T value);
        JsonNode(const char* value);
        JsonNode(const std::string& value);
        JsonNode(array_type_init list);
        JsonNode(object_type_init list);

        ~JsonNode();

        //template<typename T, t_enable_if_integral_type<T> = nullptr>
        //T Get() const;

        //template<typename T, t_enable_if_same_type<T, bool> = nullptr>
        //T Get() const;

        //template<typename T, t_enable_if_floating_type<T> = nullptr>
        //T Get() const;
        //
        //template<typename T, t_enable_if_same_type<T, std::string> = nullptr>
        //T Get() const;

        template<typename T>
        T Get() const;

        ValueType GetType() const { return m_type; }
        std::string ToString(const JsonFormatOption& format) const;

        void foreach(std::function<void(const JsonNode&)> action) const;
        void foreach_pairs(std::function<void(const std::string&, const JsonNode&)> action) const;

        void push_back(JsonNode&& node);
        void push_back(const JsonNode& node);

        bool Contains(const std::string& name) const;

        JsonNode& operator[](const std::string& name);
        const JsonNode& operator[](const std::string& name) const;
        JsonNode& operator[](size_t index);
        const JsonNode& operator[](size_t index) const;
    private:
        ValueType m_type{};
        JsonValue m_value{};

        std::string internal_tostring(const JsonFormatOption& format, int level) const;
    };


    inline JsonNode::~JsonNode()
    {
    }

    inline JsonNode::JsonNode()
        : m_type(ValueType::Null), m_value(std::monostate())
    {
    }

    template<typename T, t_enable_if_same_type<T, bool>>
    inline JsonNode::JsonNode(T value)
        : m_type(ValueType::Boolean), m_value(static_cast<bool>(value))
    {
    }

    template<typename T, t_enable_if_integral_type<T>>
    inline JsonNode::JsonNode(T value)
        : m_type(ValueType::Integer), m_value(static_cast<int64_t>(value))
    {
    }

    template<typename T, t_enable_if_floating_type<T>>
    inline JsonNode::JsonNode(T value)
        : m_type(ValueType::Float), m_value(static_cast<double>(value))
    {
    }


    inline JsonNode::JsonNode(const char* value)
        : m_type(ValueType::String), m_value(std::string(value))
    {
    }

    inline JsonNode::JsonNode(const std::string& value)
        : m_type(ValueType::String), m_value(value)
    {
    }

    inline JsonNode::JsonNode(array_type_init list)
        : m_type(ValueType::Array), m_value(array_type(list))
    {
    }

    inline JsonNode::JsonNode(object_type_init list)
        : m_type(ValueType::Object), m_value(object_type(list))
    {
    }

    //template<typename T>
    //inline T JsonNode::Get() const
    //{
    //	static_assert(false, "Cannot get an un-supportted type");
    //}

    template<typename T>
    inline T JsonNode::Get() const
    {
        if constexpr (std::is_same<std::decay_t<T>, bool>::value)
        {
            assert(m_type == ValueType::Boolean);
            return std::get<bool>(m_value);
        }
        else if constexpr (std::is_integral<std::decay_t<T>>::value && !std::is_same<std::decay_t<T>, bool>::value)
        {
            assert(m_type == ValueType::Integer);
            return static_cast<T>(std::get<int64_t>(m_value));
        }
        else if constexpr (std::is_floating_point<std::decay_t<T>>::value)
        {
            assert(m_type == ValueType::Float || m_type == ValueType::Integer);
            if (m_type == ValueType::Integer)
            {
                return static_cast<T>(std::get<int64_t>(m_value));
            }
            return static_cast<T>(std::get<double>(m_value));
        }
        else if constexpr (std::is_same<std::decay_t<T>, array_type>::value)
        {
            assert(m_type == ValueType::Array);
            return static_cast<T>(std::get<array_type>(m_value));
        }
        else
        {
            static_assert(foobar<T>::value, "Cannot get an un-supportted type");
        }
    }

    inline bool JsonNode::Contains(const std::string& name) const
    {
        assert(m_type == ValueType::Object);
        auto& map = std::get<object_type>(m_value);
        auto it = map.find(name);
        return it != map.end();
    }

    template<>
    inline std::string JsonNode::Get() const
    {
        assert(m_type == ValueType::String);
        return std::get<std::string>(m_value);
    }


    inline std::string JsonNode::ToString(const JsonFormatOption& format) const
    {
        return internal_tostring(format, 0);
    }

    inline void JsonNode::foreach(std::function<void(const JsonNode&)> action) const
    {
        assert(m_type == ValueType::Array);
        for (auto& element : std::get<array_type>(m_value))
        {
            action(element);
        }
    }

    inline void JsonNode::foreach_pairs(std::function<void(const std::string&, const JsonNode&)> action) const
    {
        assert(m_type == ValueType::Object);
        for (auto& pair : std::get<object_type>(m_value))
        {
            action(pair.first, pair.second);
        }
    }

    inline void JsonNode::push_back(JsonNode&& node)
    {
        if (m_type == ValueType::Null)
        {
            m_type = ValueType::Array;
            m_value = array_type();
        }
        assert(m_type == ValueType::Array);
        std::get<array_type>(m_value).push_back(std::move(node));
    }

    inline void JsonNode::push_back(const JsonNode& node)
    {
        if (m_type == ValueType::Null)
        {
            m_type = ValueType::Array;
            m_value = array_type();
        }
        assert(m_type == ValueType::Array);
        std::get<array_type>(m_value).push_back(node);
    }

    inline JsonNode& JsonNode::operator[](const std::string& name)
    {
        if (m_type == ValueType::Null)
        {
            m_type = ValueType::Object;
            m_value = object_type();
        }
        assert(m_type == ValueType::Object);
        return std::get<object_type>(m_value)[name];
    }

    inline const JsonNode& JsonNode::operator[](const std::string& name) const
    {
        assert(m_type == ValueType::Object);
        auto& map = std::get<object_type>(m_value);
        auto it = map.find(name);
        if (it == map.end())
        {
            throw std::logic_error("Given key does not exist");
        }
        return it->second;
    }

    inline JsonNode& JsonNode::operator[](size_t index)
    {
        assert(m_type == ValueType::Array);
        return std::get<array_type>(m_value)[index];
    }

    inline const JsonNode& JsonNode::operator[](size_t index) const
    {
        assert(m_type == ValueType::Array);
        return std::get<array_type>(m_value)[index];
    }


    inline std::string JsonNode::internal_tostring(const JsonFormatOption& format, int level) const
    {
        auto indent = [&](std::string& str, int L) {
            if (!format.Inline)
            {
                for (int i = 0; i < L + 1; i++)
                {
                    if (format.UseTab)
                    {
                        str.push_back('\t');
                    }
                    else
                    {
                        str.append("  ");
                    }
                }
            }
        };
        switch (m_type)
        {
        case SJson::ValueType::Null:
            return "null";
        case SJson::ValueType::Object:
        {
            std::string objList = "{";
            if (!format.Inline)
            {
                objList.push_back('\n');
            }
            auto& dict = std::get<object_type>(m_value);
            int count = dict.size();
            for (auto& node : dict)
            {
                indent(objList, level);
                if (format.KeysWithQuotes)
                {
                    objList.push_back('\"');
                    objList.append(node.first);
                    objList.push_back('\"');
                }
                else
                {
                    objList.append(node.first);
                }
                objList.append(": ");
                objList.append(node.second.internal_tostring(format, level + 1));
                if (--count > 0)
                {
                    objList.append(", ");
                }
                if (!format.Inline)
                {
                    objList.push_back('\n');
                }
            }
            indent(objList, level - 1);
            objList.push_back('}');
            return objList;
        }
        break;
        case SJson::ValueType::Array:
        {
            std::string arrayStr = "[";
            if (!format.Inline)
            {
                arrayStr.push_back('\n');
            }
            auto& dict = std::get<array_type>(m_value);
            int count = dict.size();
            for (auto& node : dict)
            {
                indent(arrayStr, level);
                arrayStr.append(node.internal_tostring(format, level + 1));
                if (--count > 0)
                {
                    arrayStr.append(", ");
                }
                if (!format.Inline)
                {
                    arrayStr.push_back('\n');
                }
            }
            indent(arrayStr, level - 1);
            arrayStr.push_back(']');
            return arrayStr;
        }
        break;
        case SJson::ValueType::String:
        {
            std::string arrayStr = "\"";
            arrayStr.append(std::get<std::string>(m_value));
            arrayStr.push_back('\"');
            return arrayStr;
        }
        break;
        case SJson::ValueType::Boolean:
        {
            auto boolValue = std::get<bool>(m_value);
            return boolValue ? "true" : "false";
        }
        break;
        case SJson::ValueType::Integer:
        {
            auto intValue = std::get<int64_t>(m_value);
            return std::to_string(intValue);
        }
        break;
        case SJson::ValueType::Float:
        {
            // High precision double
            auto floatValue = std::get<double>(m_value);
            std::stringstream ss;
            ss << std::fixed;
            ss << std::setprecision(std::numeric_limits<double>::digits10 + 2);
            ss << floatValue;
            return ss.str();
        }
        break;
        default:
            break;
        }
        return "";
    }


    inline bool is_white_space(const char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    inline int try_lex_keyword(const std::string& text, int index, const std::string& word)
    {
        auto len = text.size();
        auto lenWord = word.size();
        for (int i = 0; i < lenWord; i++)
        {
            if (index + i >= len || text[index + i] != word[i]) return -1;
        }
        return index + lenWord - 1;
    }

    inline int lex_remove_whole_line(const std::string& text, int index)
    {
        auto len = text.size();
        int i = index;
        while (i < len && text[i] != '\n')
        {
            i++;
        }
        return i;
    }

    inline int try_lex_string(const std::string& text, int index)
    {
        auto len = text.size();
        int i = index + 1;
        while (i < len && text[i] != '\"')
        {
            i++;
        }
        if (i == len)
        {
            //throw parse_error("Unexpected EOF while parsing string", JsonToken{ "", TokenType::String, i, });
            return -1;
        }
        return i;
    }


    struct DFAState
    {
        std::string					Name;
        std::function<int(char c)>	T;
        bool						Accept;
    };

    inline int try_lex_number(const std::string& text, int index, bool& integer)
    {
        static std::vector<DFAState> dfa = {
            DFAState{ "Init",[](char c)
            {
                if (isdigit(c)) return 2;
                else if (c == '+' || c == '-') return 1;
                else if (c == '.') return 3;
                return -1;
            }, false},
            DFAState{ "Num 0",[](char c)
            {
                if (isdigit(c)) return 2;
                else if (c == '.') return 3;
                return -1;
            }, false},
            DFAState{ "Num 1",[](char c)
            {
                if (isdigit(c)) return 2;
                else if (c == '.') return 4;
                else if (c == 'e' || c == 'E') return 5;
                return -1;
            }, true},
            DFAState{ "Dot 0",[](char c)
            {
                if (isdigit(c)) return 4;
                return -1;
            }, false},
            DFAState{ "Dot 1",[](char c)
            {
                if (isdigit(c)) return 4;
                if (c == 'e' || c == 'E') return 5;
                return -1;
            }, true},
            DFAState{ "Exp 0",[](char c)
            {
                if (isdigit(c)) return 7;
                if (c == '+' || c == '-') return 6;
                return -1;
            }, false},
            DFAState{ "Exp 0.5",[](char c)
            {
                if (isdigit(c)) return 7;
                return -1;
            }, false},
            DFAState{ "Exp 1",[](char c)
            {
                if (isdigit(c)) return 7;
                return -1;
            }, true},
        };

        auto len = text.size();
        int dfaPtr = 0;
        int i = index;
        integer = true;
        for (; i < len; i++)
        {
            auto& curState = dfa[dfaPtr];
            int nxt = curState.T(text[i]);
            if (nxt == -1)
            {
                if (curState.Accept)
                {
                    return i - 1;
                }
                else
                {
                    return -1;
                }
            }
            if (nxt == 4 || nxt == 7)
            {
                integer = false;
            }
            dfaPtr = nxt;
        }
        if (!dfa[dfaPtr].Accept) return -1;
        return i - 1;
    }

    inline int try_lex(const std::string& text, int index, JsonToken& token)
    {
        int c = text[index];
        int newIndex = -1;
        token.Position = index;
        switch (c)
        {
        case 'n':
        {
            if ((newIndex = try_lex_keyword(text, index, "null")) != -1)
            {
                token.Value = "";
                token.Token = TokenType::Null;
            }
        }
        break;
        case 't':
        {
            if ((newIndex = try_lex_keyword(text, index, "true")) != -1)
            {
                token.Value = "";
                token.Token = TokenType::True;
            }
        }
        break;
        case 'f':
        {
            if ((newIndex = try_lex_keyword(text, index, "false")) != -1)
            {
                token.Value = "";
                token.Token = TokenType::False;
            }
        }
        break;
        case '{':
        {
            newIndex = index;
            token.Value = c;
            token.Token = TokenType::LeftBrace;
        }
        break;
        case '}':
        {
            newIndex = index;
            token.Value = c;
            token.Token = TokenType::RightBrace;
        }
        break;
        case '[':
        {
            newIndex = index;
            token.Value = c;
            token.Token = TokenType::LeftBracket;
        }
        break;
        case ']':
        {
            newIndex = index;
            token.Value = c;
            token.Token = TokenType::RightBracket;
        }
        break;
        case ',':
        {
            newIndex = index;
            token.Value = c;
            token.Token = TokenType::Comma;
        }
        break;
        case ':':
        {
            newIndex = index;
            token.Value = c;
            token.Token = TokenType::Colon;
        }
        break;
        case '#':
        {
            newIndex = lex_remove_whole_line(text, index);
            token.Value = c;
            token.Token = TokenType::Comment;
        }
        break;
        case '\"':
        {
            if ((newIndex = try_lex_string(text, index)) != -1)
            {
                token.Value = text.substr(index + 1, newIndex - index - 1);
                token.Token = TokenType::String;
            }
        }
        break;
        default:
        {
            bool integer;
            if ((newIndex = try_lex_number(text, index, integer)) != -1)
            {
                token.Value = text.substr(index, newIndex - index + 1);
                token.Token = integer ? TokenType::Integer : TokenType::Float;
            }
        }
        break;
        }
        return newIndex;
    }

    inline std::vector<JsonToken> lex(const std::string& text)
    {
        std::vector<JsonToken> tokenList;
        auto originalText = std::make_shared<std::string>(text);
        int length = text.size();
        for (int i = 0; i < length; i++)
        {
            if (is_white_space(text[i]))
            {
                continue;
            }
            if (i == length) break;
            JsonToken token;
            int newIndex = try_lex(text, i, token);
            if (newIndex != -1)
            {
                i = newIndex;
                if (token.Token == TokenType::Comment)
                {
                    continue;
                }
                else
                {
                    token.OriginalText = originalText;
                    tokenList.push_back(token);
                }
            }
            else
            {
                throw lexical_error(JsonToken{ "", TokenType::Unknown, i, originalText });
            }
        }
        tokenList.push_back(JsonToken{ "", TokenType::EndOfFile, length, originalText });
        return tokenList;
    }

    inline int expect(const JsonToken& token, TokenType type, int index)
    {
        if (token.Token != type)
        {
            throw expect_token_error(type, token.Token, token);
        }
        return index + 1;
    }

    using maplist = std::vector<std::pair<const std::string, JsonNode>>;
    using m_pair = std::pair<const std::string, JsonNode>;
    using arrlist = std::vector<JsonNode>;

    inline maplist parse_object(const std::vector<JsonToken>& tokens,
        int index, int& newIndex)
    {

        maplist list;
        int curIndex = index;
        if (tokens[curIndex].Token != TokenType::RightBrace)
        {
            while (true)
            {
                auto& keyToken = tokens[curIndex++];
                if (keyToken.Token == TokenType::EndOfFile)
                {
                    throw invalid_eof(keyToken);
                }
                if (keyToken.Token != TokenType::String)
                {
                    throw keys_not_string(keyToken);
                }
                const std::string key = keyToken.Value;

                curIndex = expect(tokens[curIndex], TokenType::Colon, curIndex);
                int nxtIndex;
                auto value = parse(tokens, curIndex, nxtIndex);
                curIndex = nxtIndex;

                list.push_back(m_pair{ key, value });

                if (tokens[curIndex].Token == TokenType::Comma)
                {
                    curIndex++;
                }
                else
                {
                    break;
                }
            }
            newIndex = expect(tokens[curIndex], TokenType::RightBrace, curIndex);
            return list;
        }
        else
        {
            newIndex = index + 1;
            return list;
        }
    }

    inline arrlist parse_array(const std::vector<JsonToken>& tokens,
        int index, int& newIndex)
    {
        arrlist list;
        int curIndex = index;
        if (tokens[curIndex].Token != TokenType::RightBracket)
        {
            while (true)
            {
                int nxtIndex;
                list.push_back(parse(tokens, curIndex, nxtIndex));
                curIndex = nxtIndex;
                if (tokens[curIndex].Token == TokenType::Comma)
                {
                    curIndex++;
                }
                else
                {
                    break;
                }
            }
            newIndex = expect(tokens[curIndex], TokenType::RightBracket, curIndex);
            return list;
        }
        else
        {
            newIndex = index + 1;
            return list;
        }
    }

    inline std::string remove_escapes(const std::string& str, const JsonToken& token)
    {
        std::string result;
        int len = str.size();
        for (int i = 0; i < len; i++)
        {
            if (str[i] != '\\')
            {
                result.push_back(str[i]);
                continue;
            }
            if (str[i] == '\\')
            {
                if (i == len - 1)
                {
                    throw invalid_escape_char(token);
                }
                switch (str[i + 1])
                {
                case 'n':
                    result.push_back('\n');
                    i++;
                    break;
                case 'r':
                    result.push_back('\r');
                    i++;
                    break;
                case 't':
                    result.push_back('\t');
                    i++;
                    break;
                case '\\':
                    result.push_back('\\');
                    i++;
                    break;
                default:
                    throw invalid_escape_char(token);
                    break;
                }
            }
        }
        return result;
    }

    inline JsonNode parse(const std::vector<JsonToken>& tokens, int index, int& newIndex)
    {
        auto& token = tokens[index];
        switch (token.Token)
        {
        case TokenType::Null:
        {
            newIndex = index + 1;
            return JsonNode();
        }
        case TokenType::Integer:
        {
            newIndex = index + 1;
            return JsonNode(std::stoll(token.Value, nullptr, 10));
        }
        case TokenType::Float:
        {
            newIndex = index + 1;
            return JsonNode(std::stod(token.Value, nullptr));
        }
        case TokenType::True:
        {
            newIndex = index + 1;
            return JsonNode(true);
        }
        case TokenType::False:
        {
            newIndex = index + 1;
            return JsonNode(false);
        }
        case TokenType::String:
        {
            newIndex = index + 1;
            return JsonNode(remove_escapes(token.Value, token));
        }
        case TokenType::LeftBrace:
        {
            JsonNode node = JsonNode(object_type_init());
            auto&& obj = parse_object(tokens, index + 1, newIndex);
            for (auto& pair : obj)
            {
                node[pair.first] = pair.second;
            }
            return node;
        }
        case TokenType::LeftBracket:
        {
            JsonNode node = JsonNode(array_type_init());
            auto&& arr = parse_array(tokens, index + 1, newIndex);
            for (auto& element : arr)
            {
                node.push_back(element);
            }
            return node;
        }
        case TokenType::EndOfFile:
        {
            throw invalid_eof(token);
        }
        default:
            break;
        }
        throw parse_match_failed(token);
    }

    inline JsonNode JsonConvert::Parse(const std::string& text)
    {
        auto tokens = lex(text);
        int index = 0;
        auto node = parse(tokens, 0, index);
        if (tokens[index].Token != TokenType::EndOfFile)
        {
            throw root_not_singular_error(tokens[index]);
        }
        return node;
    }

    inline std::string GetValueTypeName(ValueType type)
    {
        switch (type)
        {
        case SJson::ValueType::Null:
            return "Null";
        case SJson::ValueType::Object:
            return "Object";
        case SJson::ValueType::Array:
            return "Array";
        case SJson::ValueType::String:
            return "String";
        case SJson::ValueType::Boolean:
            return "Boolean";
        case SJson::ValueType::Integer:
            return "Integer";
        case SJson::ValueType::Float:
            return "Float";
        case SJson::ValueType::__COUNT:
        default:
            break;
        }
        return "ERROR TYPE";
    }

    inline std::string GetTokenFromType(TokenType type)
    {
        switch (type )
        {
        case SJson::TokenType::Null: return "null";
        case SJson::TokenType::True: return "true";
        case SJson::TokenType::False: return "false";
        case SJson::TokenType::Integer: return "number";
        case SJson::TokenType::Float: return "number";
        case SJson::TokenType::String: return "string";
        case SJson::TokenType::LeftBrace: return "'{'";
        case SJson::TokenType::RightBrace: return "'}'";
        case SJson::TokenType::LeftBracket: return "'['";
        case SJson::TokenType::RightBracket: return "']'";
        case SJson::TokenType::Comma: return "','";
        case SJson::TokenType::Colon: return "':'";
        case SJson::TokenType::Comment: return "'#'";
        case SJson::TokenType::EndOfFile: return "EOF";
        default:
            break;
        }
        return "Unkown";
    }

    inline JsonNode object(std::initializer_list<std::pair<const std::string, JsonNode>> list)
    {
        return JsonNode(list);
    }

    inline JsonNode array(std::initializer_list<JsonNode> list)
    {
        return JsonNode(list);
    }

    //
    // Serialization
    // 序列化
    // 

    template<typename T> struct is_vector : public std::false_type {};

    template<typename T, typename A>
    struct is_vector<std::vector<T, A>> : public std::true_type {};


    template<typename T> struct is_map : public std::false_type {};

    template<typename K, typename V, typename Cmp, typename Alloc>
    struct is_map<std::map<K, V, Cmp, Alloc>> : public std::true_type {};

    template<typename, typename = void>
    struct is_type_complete : public std::false_type {};

    template<typename T>
    struct is_type_complete
        <T, std::void_t<decltype(sizeof(T))>> : public std::true_type
    {
    };

//    template<typename T, typename = void>
//    struct is_parent_defined : public std::false_type {};
//
//    template<typename T>
//    struct is_parent_defined<T, std::void_t<decltype(T::parents)>> : std::true_type {};
//
//    template<typename T, typename = void>
//    struct has_properties : public std::false_type {};
//
//    template<typename T>
//    struct has_properties<T, std::void_t<decltype(T::properties())>> : std::true_type {};
//
//#define PROPERTY(member) SJson::property(&Self_Type::member, #member)
//#define BASECLASS(type) SJson::classref<type>(#type)
//#define PLACE(...) std::make_tuple(__VA_ARGS__)
//#define PROPERTIES(Type, ...) constexpr static auto properties()\
//{\
//    using Self_Type = Type;\
//    return PLACE(__VA_ARGS__);\
//}\
//
//#define ENUM_KEY(key) case ENUM_TYPE::key: return #key;
//#define GENERATE_ENUM(T, FIELDS)\
//template<>\
//struct SJson::enum_mapper<SType>\
//{\
//    static const char* const map(T v)\
//    {\
//        using ENUM_TYPE = T;\
//        switch (v)\
//        {\
//        FIELDS(ENUM_KEY)\
//        default:\
//            break;\
//        }\
//        throw std::logic_error("Invalid enum type");\
//    }\
//};
//
//    template<typename Class, typename T>
//    struct PropertyImpl
//    {
//        constexpr PropertyImpl(T Class::* aMember, const char* aName) : member{ aMember }, name{ aName } {}
//
//        using Type = T;
//
//        T Class::* member;
//        const char* name;
//    };
//
//    template<typename Class, typename T>
//    constexpr auto property(T Class::* member, const char* name)
//    {
//        return PropertyImpl<Class, T>{member, name};
//    }
//
//    template<typename Class>
//    struct ClassImpl
//    {
//        using Type = Class;
//
//        const char* name;
//    };
//
//    template<typename Class>
//    constexpr auto classref(const char* name)
//    {
//        return ClassImpl<Class>{name};
//    }
//
//    template <typename T, T... S, typename F>
//    constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f)
//    {
//        (static_cast<void>(f(std::integral_constant<T, S>{})), ...);
//    }
//
//    template <typename E>
//    struct enum_mapper
//    {
//        static const char* const map(E v);
//    };

    /**
     * @brief If is a string, we can convert it to a JSON string value
     * @param v
     * @return
    */
    inline SJson::JsonNode serialize(const std::string& v)
    {
        return SJson::JsonNode(v);
    }

    template<typename T, std::enable_if_t<std::is_fundamental<T>::value, nullptr_t> = nullptr>
    constexpr JsonNode serialize(const T& v)
    {
        // If is primitive types, we can convert it directly to a JSON value
        return SJson::JsonNode(v);
    }

    template<typename T, std::enable_if_t<is_vector<T>::value, nullptr_t> = nullptr>
    constexpr JsonNode serialize(const T& v)
    {
        // If is a vector of elements, we can convert them to a JSON array
        SJson::JsonNode node = SJson::array({});
        for (auto& e : v)
        {
            node.push_back(serialize(e));
        }
        return node;
    }

    template<typename T, std::enable_if_t<std::is_enum<T>::value, nullptr_t> = nullptr>
    constexpr JsonNode serialize(const T& v)
    {
        // If is a vector of elements, we can convert them to a JSON array
        return SJson::JsonNode(SRefl::EnumInfo<T>::enum_to_string(v));
    }

    template<typename K, typename V>
    constexpr SJson::JsonNode serialize(const std::pair<K, V>& v)
    {
        SJson::JsonNode node = SJson::object({});
        node["key"] = serialize(v.first);
        node["value"] = serialize(v.second);
        return node;
    }

    template<bool _To_Obj = false, typename K, typename V, typename Cmp, typename Alloc>
    constexpr SJson::JsonNode serialize(const std::map<K, V, Cmp, Alloc>& v)
    {
        if constexpr (_To_Obj && std::is_same<std::decay_t<K>, std::string>::value)
        {
            SJson::JsonNode node = SJson::object({});
            for (auto& pair : v)
            {
                node[pair.first] = serialize(pair.second);
            }
            return node;
        }
        else
        {
            SJson::JsonNode node = SJson::array({});
            for (auto& pair : v)
            {
                node.push_back(serialize(pair));
            }
            return node;
        }
    }

    /**
     * @brief If it is a tuple of elements, we can convert it to a JSON array
     * @tparam ...Ts
     * @param v
     * @return
    */
    template<typename... Ts>
    constexpr SJson::JsonNode serialize(const std::tuple<Ts...>& v)
    {
        SJson::JsonNode json = SJson::array({});
        constexpr auto numTypes = sizeof...(Ts);
        SRefl::for_sequence(std::make_index_sequence<numTypes>{}, [&](auto i) {
            json.push_back(serialize(std::get<i>(v)));
            });
        return json;
    }

    template<typename T, std::enable_if_t<SRefl::has_fields_v<T>, nullptr_t> = nullptr>
    constexpr JsonNode serialize(const T& v)
    {
        SJson::JsonNode json;
        if constexpr (SRefl::has_fields_v<T>)
        {
            SRefl::ForEachField<T>([&json, &v](auto field) {
                json[field.Name] = serialize(v.*(field.MemberPtr));
                });
        }
        if constexpr (SRefl::has_bases_v<T>)
        {
            SRefl::ForEachBase<T>([&json, &v](auto baseRef) {
                std::string key = "$";
                key.append(baseRef.Name);
                json[key] = serialize<decltype(baseRef)::_Type>(v);
                });
        }
        //// We first get the number of properties
        //constexpr auto nbProperties = std::tuple_size<decltype(T::properties())>::value;

        //// We iterate on the index sequence of size `nbProperties`
        //for_sequence(std::make_index_sequence<nbProperties>{}, [&](auto i) {
        //    // get the property
        //    constexpr auto property = std::get<i>(T::properties());

        //    // set the value to the member
        //    json[property.name] = serialize(v.*(property.member));
        //    });

        //if constexpr (is_parent_defined<T>::value)
        //{
        //    constexpr auto nParents = std::tuple_size<decltype(T::parents())>::value;
        //    for_sequence(std::make_index_sequence<nParents>{}, [&json, &v](auto i) {
        //        constexpr auto classRef = std::get<i>(T::parents());

        //        std::string key = "$";
        //        key.append(classRef.name);
        //        json[key] = serialize(static_cast<const decltype(classRef)::Type&>(v));
        //    });
        //}
        return json;
    }

    //
    // De-serialization
    // 反序列化
    // 

    /**
     * @brief If is primitive types, we can directly get value from JSON
     * @tparam T
     * @param v
     * @return
    */
    template<typename T>
    inline T de_serialize(const JsonNode& node)
    {
        if constexpr (std::is_fundamental<T>::value)
        {
            return node.Get<T>();
        }
        else if constexpr (is_vector<T>::value)
        {
            T vec{};
            node.foreach([&](const JsonNode& e) {
                vec.push_back(de_serialize<typename T::value_type>(e));
            });
            return vec;
        }
        else if constexpr (is_map<T>::value)
        {
            T mapp{};
            node.foreach([&](const JsonNode& e) {
                auto key = de_serialize<typename T::key_type>(e["key"]);
                auto value = de_serialize<typename T::mapped_type>(e["value"]);
                mapp[key] = value;
            });
            return mapp;
        }
        else if constexpr (std::is_enum<T>::value)
        {
            return SRefl::EnumInfo<T>::string_to_enum(node.Get<std::string>());
        }
        else if constexpr (SRefl::has_fields_v<T>)
        {
            T result{};
            SRefl::ForEachField<T>([&result, &node](auto field) {
                result.*(field.MemberPtr) = de_serialize<decltype(field)::_Type>(node[field.Name]);
                });
            //// We first get the number of properties
            //constexpr auto nbProperties = std::tuple_size<decltype(T::properties())>::value;

            //// We iterate on the index sequence of size `nbProperties`
            //for_sequence(std::make_index_sequence<nbProperties>{}, [&](auto i) {
            //    // get the property
            //    constexpr auto prop = std::get<i>(T::properties());

            //    // set the value to the member
            //    result.*(prop.member) = de_serialize<decltype(prop)::Type>(node[prop.name]);
            //    });

            if constexpr (SRefl::has_bases_v<T>)
            {
                SRefl::ForEachBase<T>([&result, &node](auto baseRef) {
                    using baseType = typename decltype(baseRef)::_Type;
                    baseType& base = result;
                    std::string key = "$";
                    key.append(baseRef.Name);
                    base = de_serialize<baseType>(node[key]);
                    });
                //constexpr auto nParents = std::tuple_size<decltype(T::parents())>::value;
                //for_sequence(std::make_index_sequence<nParents>{}, [&](auto i) {
                //    constexpr auto classRef = std::get<i>(T::parents());

                //    using baseType = typename decltype(classRef)::Type;
                //    baseType& base = result;
                //    std::string key = "$";
                //    key.append(classRef.name);
                //    base = de_serialize<baseType>(node[key]);
                //    });
            }
            return result;
        }
        else
        {
            static_assert(foobar<T>::value, "Cannot deserialize this type");
        }
    }

    template<>
    inline std::string de_serialize(const JsonNode& node)
    {
        return node.Get<std::string>();
    }

    template<typename T>
    inline std::string JsonConvert::Serialize(const T& v, const JsonFormatOption& option)
    {
        return serialize(v).ToString(option);
    }

    template<typename T>
    inline T JsonConvert::Deserialize(const std::string jsonStr)
    {
        return de_serialize<T>(Parse(jsonStr));
    }
}
