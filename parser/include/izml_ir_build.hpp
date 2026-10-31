#ifndef IZML_IR_BUILD_HPP
#define IZML_IR_BUILD_HPP

#include "izml_ir.hpp"
#include "izml_ast.hpp"
#include "izml_parser_c_api.h"

#include <utility>

namespace izmlr
{
    namespace detail
    {
        inline Value build_value(const izml::Node* node)
        {
            Value value;
            if (node == nullptr)
            {
                return value;
            }
            switch (node->kind)
            {
            case izml::NodeKind::Int:
                value.kind = ValueKind::Int;
                value.integer = node->intValue;
                break;
            case izml::NodeKind::Float:
                value.kind = ValueKind::Float;
                value.real = node->floatValue;
                break;
            case izml::NodeKind::Str:
                value.kind = ValueKind::String;
                value.text = node->text;
                break;
            case izml::NodeKind::Bool:
                value.kind = ValueKind::Bool;
                value.boolean = node->boolValue;
                break;
            case izml::NodeKind::Ref:
                value.kind = ValueKind::Ref;
                value.text = node->text;
                break;
            case izml::NodeKind::Object:
                value.kind = ValueKind::Object;
                for (const izml::Node* child : node->children)
                {
                    if (child->kind != izml::NodeKind::Entry)
                    {
                        continue;
                    }
                    Entry entry;
                    entry.name = child->name;
                    entry.value = build_value(child->value_child());
                    value.entries.push_back(std::move(entry));
                }
                break;
            case izml::NodeKind::Array:
                value.kind = ValueKind::Array;
                for (const izml::Node* child : node->children)
                {
                    value.items.push_back(build_value(child));
                }
                break;
            default:
                break;
            }
            return value;
        }

        inline std::vector<Entry> build_entries(const izml::Node* node)
        {
            std::vector<Entry> entries;
            if (node == nullptr)
            {
                return entries;
            }
            for (const izml::Node* child : node->children)
            {
                if (child->kind != izml::NodeKind::Entry)
                {
                    continue;
                }
                Entry entry;
                entry.name = child->name;
                entry.value = build_value(child->value_child());
                entries.push_back(std::move(entry));
            }
            return entries;
        }

        inline Field build_field(const izml::Node* node)
        {
            Field field;
            if (node == nullptr)
            {
                return field;
            }
            field.name = node->name;
            field.id = node->attr_int("id", 0);
            field.optional = node->flag("optional");
            field.optional_bit = node->attr_int("opt_bit", 0);
            field.kind = node->attr_or("kind", "");
            field.type_name = node->attr_or("type_name", "");
            field.type_tag = node->attr_int("type_tag", 0);
            field.element_kind = node->attr_or("element_kind", "");
            field.element_name = node->attr_or("element_name", "");
            field.element_tag = node->attr_int("element_tag", 0);
            field.size = node->attr_int("size", 0);
            return field;
        }

        inline Define build_define(const izml::Node* node)
        {
            Define define;
            if (node == nullptr)
            {
                return define;
            }
            define.name = node->name;
            define.qualified = node->attr_or("qualified", node->name);
            define.base_name = node->attr_or("base_name", "");
            define.type_tag = node->attr_int("type_tag", 0);
            define.nested = node->flag("nested");
            define.optional_count = node->attr_int("optional_count", 0);

            for (const izml::Node* child : node->children)
            {
                if (child->kind == izml::NodeKind::Field)
                {
                    define.fields.push_back(build_field(child));
                }
                else if (child->kind == izml::NodeKind::Define)
                {
                    define.nested_defines.push_back(build_define(child));
                }
            }
            for (const izml::Node* field : node->layout)
            {
                define.layout.push_back(build_field(field));
            }
            return define;
        }

        inline Enum build_enum(const izml::Node* node)
        {
            Enum value;
            if (node == nullptr)
            {
                return value;
            }
            value.name = node->name;
            value.base_kind = node->attr_or("base_kind", "byte");
            value.base_size = node->attr_int("base_size", 1);
            for (const izml::Node* child : node->children)
            {
                if (child->kind != izml::NodeKind::EnumMember)
                {
                    continue;
                }
                EnumMember member;
                member.name = child->name;
                member.value = child->intValue;
                value.members.push_back(std::move(member));
            }
            return value;
        }

        inline Using build_using(const izml::Node* node)
        {
            Using value;
            if (node == nullptr)
            {
                return value;
            }
            value.name = node->name;
            value.is_list = node->flag("is_list");
            value.type_name = node->attr_or("type_name", "");
            value.element_name = node->attr_or("element_name", "");
            value.type_tag = node->attr_int("type_tag", 0);
            value.element_tag = node->attr_int("element_tag", 0);
            value.root_index = value.is_list ? -1 : node->attr_int("root_index", -1);
            if (value.is_list)
            {
                for (const izml::Node* child : node->children)
                {
                    if (child->kind == izml::NodeKind::Ref)
                    {
                        value.references.push_back(child->text);
                    }
                }
            }
            else
            {
                value.entries = build_entries(node);
            }
            return value;
        }
    }

    inline bool build(izml_document* document, Program& program)
    {
        const izml::Document* source = reinterpret_cast<const izml::Document*>(document);
        if (source == nullptr || !source->ok())
        {
            return false;
        }
        program.file = source->file;
        program.version = source->version;
        program.root_count = static_cast<Int32>(source->rootCount);
        for (const izml::Node* node : source->enums)
        {
            program.enums.push_back(detail::build_enum(node));
        }
        for (const izml::Node* node : source->defines)
        {
            program.defines.push_back(detail::build_define(node));
        }
        for (const izml::Node* node : source->instances)
        {
            program.usings.push_back(detail::build_using(node));
        }
        return true;
    }
}

#endif
