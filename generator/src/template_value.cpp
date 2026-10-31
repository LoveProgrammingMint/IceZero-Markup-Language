#include "template_value.hpp"

#include <set>
#include <sstream>

namespace izmlgenc
{
    Value Value::null()
    {
        return Value();
    }

    Value Value::boolean_of(bool value)
    {
        Value result;
        result.kind = ValueKind::Bool;
        result.boolean = value;
        return result;
    }

    Value Value::int_of(Int64 value)
    {
        Value result;
        result.kind = ValueKind::Int;
        result.integer = value;
        return result;
    }

    Value Value::double_of(double value)
    {
        Value result;
        result.kind = ValueKind::Double;
        result.real = value;
        return result;
    }

    Value Value::string_of(const String& value)
    {
        Value result;
        result.kind = ValueKind::String;
        result.text = value;
        return result;
    }

    Value Value::list_of(std::vector<Value> items)
    {
        Value result;
        result.kind = ValueKind::List;
        result.items = std::move(items);
        return result;
    }

    Value Value::object_of()
    {
        Value result;
        result.kind = ValueKind::Object;
        return result;
    }

    Value& Value::set(const String& key, Value value)
    {
        if (kind != ValueKind::Object)
        {
            kind = ValueKind::Object;
        }
        object[key] = std::move(value);
        return *this;
    }

    const Value* Value::find(const String& key) const
    {
        if (kind != ValueKind::Object)
        {
            return nullptr;
        }
        auto found = object.find(key);
        return found != object.end() ? &found->second : nullptr;
    }

    bool Value::truthy() const
    {
        switch (kind)
        {
        case ValueKind::Null: return false;
        case ValueKind::Bool: return boolean;
        case ValueKind::Int: return integer != 0;
        case ValueKind::Double: return real != 0.0;
        case ValueKind::String: return !text.empty();
        case ValueKind::List: return !items.empty();
        case ValueKind::Object: return !object.empty();
        }
        return false;
    }

    String Value::as_text() const
    {
        switch (kind)
        {
        case ValueKind::Null: return "";
        case ValueKind::Bool: return boolean ? "true" : "false";
        case ValueKind::Int: return std::to_string(integer);
        case ValueKind::Double:
        {
            std::ostringstream stream;
            stream.precision(17);
            stream << real;
            return stream.str();
        }
        case ValueKind::String: return text;
        case ValueKind::List:
        {
            String result;
            for (const Value& item : items)
            {
                if (!result.empty()) { result += ","; }
                result += item.as_text();
            }
            return result;
        }
        case ValueKind::Object:
        {
            String result;
            for (const auto& pair : object)
            {
                if (!result.empty()) { result += ","; }
                result += pair.first + "=" + pair.second.as_text();
            }
            return result;
        }
        }
        return "";
    }

    namespace
    {
        Value enum_value(const izmlr::Enum& value)
        {
            Value result = Value::object_of();
            result.set("name", Value::string_of(value.name));
            result.set("base_kind", Value::string_of(value.base_kind));
            result.set("base_size", Value::int_of(value.base_size));
            std::vector<Value> members;
            for (const izmlr::EnumMember& member : value.members)
            {
                Value item = Value::object_of();
                item.set("name", Value::string_of(member.name));
                item.set("value", Value::int_of(member.value));
                members.push_back(std::move(item));
            }
            result.set("members", Value::list_of(std::move(members)));
            return result;
        }

        const izmlr::Enum* find_enum(const izmlr::Program& program, const String& name)
        {
            return program.find_enum(name);
        }

        Value field_value(const izmlr::Program& program, const izmlr::Field& field)
        {
            Value result = Value::object_of();
            result.set("name", Value::string_of(field.name));
            result.set("id", Value::int_of(field.id));
            result.set("optional", Value::boolean_of(field.optional));
            result.set("optional_bit", Value::int_of(field.optional_bit));
            result.set("bitmap_byte", Value::int_of(field.optional_bit / 8));
            result.set("bitmap_bit", Value::int_of(field.optional_bit % 8));
            result.set("mask", Value::int_of(1 << (field.optional_bit % 8)));
            result.set("kind", Value::string_of(field.kind));
            result.set("type_name", Value::string_of(field.type_name));
            result.set("type_tag", Value::int_of(field.type_tag));
            result.set("element_kind", Value::string_of(field.element_kind));
            result.set("element_name", Value::string_of(field.element_name));
            result.set("element_tag", Value::int_of(field.element_tag));
            result.set("size", Value::int_of(field.size));
            const izmlr::Enum* enum_type = field.kind == "enum" ? find_enum(program, field.type_name) : nullptr;
            result.set("enum_base_kind", Value::string_of(enum_type != nullptr ? enum_type->base_kind : String("byte")));
            const izmlr::Enum* element_enum = field.element_kind == "enum" ? find_enum(program, field.element_name) : nullptr;
            result.set("element_enum_base_kind", Value::string_of(element_enum != nullptr ? element_enum->base_kind : String("byte")));
            return result;
        }

        Value define_value(const izmlr::Program& program, const izmlr::Define& define)
        {
            Value result = Value::object_of();
            result.set("name", Value::string_of(define.name));
            result.set("qualified", Value::string_of(define.qualified));
            result.set("base_name", Value::string_of(define.base_name));
            result.set("type_tag", Value::int_of(define.type_tag));
            result.set("nested", Value::boolean_of(define.nested));
            result.set("optional_count", Value::int_of(define.optional_count));
            result.set("bitmap_bytes", Value::int_of((define.optional_count + 7) / 8));
            result.set("has_nested", Value::boolean_of(!define.nested_defines.empty()));

            std::vector<Value> fields;
            for (const izmlr::Field& field : define.fields)
            {
                fields.push_back(field_value(program, field));
            }
            result.set("fields", Value::list_of(std::move(fields)));

            std::vector<Value> layout;
            for (const izmlr::Field& field : define.layout)
            {
                layout.push_back(field_value(program, field));
            }
            result.set("layout", Value::list_of(std::move(layout)));

            std::vector<Value> nested;
            for (const izmlr::Define& child : define.nested_defines)
            {
                nested.push_back(define_value(program, child));
            }
            result.set("nested_defines", Value::list_of(std::move(nested)));
            return result;
        }

        Value using_value(const izmlr::Using& using_value)
        {
            Value result = Value::object_of();
            result.set("name", Value::string_of(using_value.name));
            result.set("is_list", Value::boolean_of(using_value.is_list));
            result.set("type_name", Value::string_of(using_value.type_name));
            result.set("element_name", Value::string_of(using_value.element_name));
            result.set("type_tag", Value::int_of(using_value.type_tag));
            result.set("element_tag", Value::int_of(using_value.element_tag));
            result.set("root_index", Value::int_of(using_value.root_index));
            return result;
        }

        void collect_top_level(const izmlr::Program& program,
                               const izmlr::Define& define,
                               std::set<String>& seen,
                               std::vector<Value>& out)
        {
            if (seen.count(define.qualified) != 0)
            {
                return;
            }
            if (!define.base_name.empty())
            {
                const izmlr::Define* base = program.find_define(define.base_name);
                if (base != nullptr && !base->nested && seen.count(base->qualified) == 0)
                {
                    collect_top_level(program, *base, seen, out);
                }
            }
            seen.insert(define.qualified);
            out.push_back(define_value(program, define));
        }
    }

    Value program_value(const izmlr::Program& program)
    {
        Value result = Value::object_of();
        result.set("file", Value::string_of(program.file));
        result.set("version", Value::string_of(program.version));
        result.set("root_count", Value::int_of(program.root_count));

        std::vector<Value> enums;
        for (const izmlr::Enum& value : program.enums)
        {
            enums.push_back(enum_value(value));
        }
        result.set("enums", Value::list_of(std::move(enums)));

        std::set<String> seen;
        std::vector<Value> top;
        for (const izmlr::Define& define : program.defines)
        {
            collect_top_level(program, define, seen, top);
        }
        result.set("top_level_defines", Value::list_of(std::move(top)));

        std::vector<Value> defines;
        for (const izmlr::Define& define : program.defines)
        {
            defines.push_back(define_value(program, define));
        }
        result.set("defines", Value::list_of(std::move(defines)));

        std::vector<Value> layouts;
        for (const izmlr::Define& define : program.defines)
        {
            if (!define.layout.empty())
            {
                layouts.push_back(define_value(program, define));
            }
        }
        result.set("layout_defines", Value::list_of(std::move(layouts)));

        std::vector<Value> usings;
        std::vector<Value> instances;
        std::vector<Value> list_views;
        for (const izmlr::Using& using_value_ : program.usings)
        {
            Value value = using_value(using_value_);
            usings.push_back(value);
            if (using_value_.is_list) { list_views.push_back(value); }
            else { instances.push_back(value); }
        }
        const Int64 instance_count = static_cast<Int64>(instances.size());
        const Int64 list_view_count = static_cast<Int64>(list_views.size());
        result.set("usings", Value::list_of(std::move(usings)));
        result.set("instances", Value::list_of(std::move(instances)));
        result.set("list_views", Value::list_of(std::move(list_views)));
        result.set("instance_count", Value::int_of(instance_count));
        result.set("list_view_count", Value::int_of(list_view_count));
        return result;
    }
}
