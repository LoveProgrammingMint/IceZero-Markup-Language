#ifndef IZML_COMMON_HPP
#define IZML_COMMON_HPP

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using Int8 = std::int8_t;
using Int16 = std::int16_t;
using Int32 = std::int32_t;
using Int64 = std::int64_t;
using UInt8 = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;
using Byte = std::uint8_t;
using String = std::string;
using StringView = std::string_view;

namespace izml
{
    struct Diagnostic
    {
        Int32 severity = 0;
        Int32 line = 0;
        Int32 column = 0;
        String file;
        String message;

        String formatted() const
        {
            String result = file.empty() ? String("<izml>") : file;
            result += '(' + std::to_string(line) + ',' + std::to_string(column) + "): ";
            result += severity == 0 ? "error: " : (severity == 1 ? "warning: " : "note: ");
            result += message;
            return result;
        }
    };

    inline bool is_identifier_start(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    inline bool is_identifier_char(char c)
    {
        return is_identifier_start(c) || (c >= '0' && c <= '9');
    }

    inline bool is_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    inline bool is_upper_identifier(const String& text)
    {
        return !text.empty() && text[0] >= 'A' && text[0] <= 'Z';
    }
}

#endif