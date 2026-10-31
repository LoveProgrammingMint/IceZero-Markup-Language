#ifndef IZMLGENC_TEMPLATE_VALUE_HPP
#define IZMLGENC_TEMPLATE_VALUE_HPP

#include "izml_ir.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace izmlgenc
{
    using Int64 = std::int64_t;
    using String = std::string;

    enum class ValueKind
    {
        Null,
        Bool,
        Int,
        Double,
        String,
        List,
        Object
    };

    struct Value
    {
        ValueKind kind = ValueKind::Null;
        bool boolean = false;
        Int64 integer = 0;
        double real = 0.0;
        String text;
        std::vector<Value> items;
        std::map<String, Value> object;

        static Value null();
        static Value boolean_of(bool value);
        static Value int_of(Int64 value);
        static Value double_of(double value);
        static Value string_of(const String& value);
        static Value list_of(std::vector<Value> items);
        static Value object_of();

        Value& set(const String& key, Value value);
        const Value* find(const String& key) const;
        bool truthy() const;
        String as_text() const;
    };

    Value program_value(const izmlr::Program& program);
}

#endif
