#ifndef IZML_IR_HPP
#define IZML_IR_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace izmlr
{
    using Int32 = std::int32_t;
    using Int64 = std::int64_t;
    using UInt32 = std::uint32_t;
    using String = std::string;

    enum class ValueKind
    {
        Int,
        Float,
        String,
        Bool,
        Ref,
        Object,
        Array
    };

    struct Entry;

    struct Value
    {
        ValueKind kind = ValueKind::Int;
        Int64 integer = 0;
        double real = 0.0;
        bool boolean = false;
        String text;
        std::vector<Entry> entries;
        std::vector<Value> items;
    };

    struct Entry
    {
        String name;
        Value value;
    };

    struct EnumMember
    {
        String name;
        Int64 value = 0;
    };

    struct Enum
    {
        String name;
        String base_kind = "byte";
        Int32 base_size = 1;
        std::vector<EnumMember> members;
    };

    struct Field
    {
        String name;
        Int32 id = 0;
        bool optional = false;
        Int32 optional_bit = 0;
        String kind;
        String type_name;
        Int32 type_tag = 0;
        String element_kind;
        String element_name;
        Int32 element_tag = 0;
        Int32 size = 0;
    };

    struct Define
    {
        String name;
        String qualified;
        String base_name;
        Int32 type_tag = 0;
        bool nested = false;
        Int32 optional_count = 0;
        std::vector<Field> fields;
        std::vector<Field> layout;
        std::vector<Define> nested_defines;
    };

    struct Using
    {
        String name;
        bool is_list = false;
        String type_name;
        String element_name;
        Int32 type_tag = 0;
        Int32 element_tag = 0;
        Int32 root_index = -1;
        std::vector<String> references;
        std::vector<Entry> entries;
    };

    struct Program
    {
        String file;
        String version;
        Int32 root_count = 0;
        std::vector<Enum> enums;
        std::vector<Define> defines;
        std::vector<Using> usings;

        const Define* find_define(const String& name) const;
        const Enum* find_enum(const String& name) const;
    };

    inline const Define* find_define_in(const std::vector<Define>& defines, const String& name)
    {
        for (const Define& define : defines)
        {
            if (define.qualified == name || define.name == name)
            {
                return &define;
            }
            const Define* nested = find_define_in(define.nested_defines, name);
            if (nested != nullptr)
            {
                return nested;
            }
        }
        return nullptr;
    }

    inline const Define* Program::find_define(const String& name) const
    {
        return find_define_in(defines, name);
    }

    inline const Enum* Program::find_enum(const String& name) const
    {
        for (const Enum& value : enums)
        {
            if (value.name == name)
            {
                return &value;
            }
        }
        return nullptr;
    }
}

#endif
