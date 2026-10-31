#ifndef IZML_AST_HPP
#define IZML_AST_HPP

#include "izml_common.hpp"
#include "izml_parser_c_api.h"

namespace izml
{
    enum class NodeKind : Int32
    {
        Document = IZML_NODE_DOCUMENT,
        Version = IZML_NODE_VERSION,
        Enum = IZML_NODE_ENUM,
        EnumMember = IZML_NODE_ENUM_MEMBER,
        Define = IZML_NODE_DEFINE,
        Field = IZML_NODE_FIELD,
        Type = IZML_NODE_TYPE,
        Using = IZML_NODE_USING,
        Int = IZML_NODE_INT,
        Float = IZML_NODE_FLOAT,
        Str = IZML_NODE_STRING,
        Bool = IZML_NODE_BOOL,
        Ref = IZML_NODE_REF,
        Object = IZML_NODE_OBJECT,
        Array = IZML_NODE_ARRAY,
        Entry = IZML_NODE_ENTRY
    };

    struct Node;

    const char* node_kind_name(NodeKind kind);
    String render_type(const Node* type);

    struct Node
    {
        NodeKind kind = NodeKind::Document;
        String name;
        String text;
        Int64 intValue = 0;
        double floatValue = 0.0;
        bool boolValue = false;
        Int32 line = 1;
        Int32 column = 1;
        Node* parent = nullptr;
        std::vector<Node*> children;
        std::vector<Node*> layout;
        std::vector<std::pair<String, String>> attrs;

        bool flag(const String& key) const
        {
            const String* value = attr(key);
            return value != nullptr && *value == "1";
        }

        void set(const String& key, const String& value)
        {
            for (auto& pair : attrs)
            {
                if (pair.first == key)
                {
                    pair.second = value;
                    return;
                }
            }
            attrs.emplace_back(key, value);
        }

        const String* attr(const String& key) const
        {
            for (const auto& pair : attrs)
            {
                if (pair.first == key)
                {
                    return &pair.second;
                }
            }
            return nullptr;
        }

        String attr_or(const String& key, const String& fallback) const
        {
            const String* value = attr(key);
            return value != nullptr ? *value : fallback;
        }

        Int32 attr_int(const String& key, Int32 fallback) const
        {
            const String* value = attr(key);
            return value != nullptr && !value->empty() ? static_cast<Int32>(std::strtol(value->c_str(), nullptr, 10)) : fallback;
        }

        Node* add(Node* child)
        {
            child->parent = this;
            children.push_back(child);
            return child;
        }

        Node* first_child() const
        {
            return children.empty() ? nullptr : children.front();
        }

        Node* find_child(const String& key) const
        {
            for (Node* child : children)
            {
                if (child->name == key)
                {
                    return child;
                }
            }
            return nullptr;
        }

        Node* type_child() const
        {
            return !children.empty() && children.front()->kind == NodeKind::Type ? children.front() : nullptr;
        }

        Node* value_child() const
        {
            for (Node* child : children)
            {
                if (child->kind != NodeKind::Type)
                {
                    return child;
                }
            }
            return nullptr;
        }
    };

    struct Document
    {
        String file;
        String version;
        std::vector<Diagnostic> diagnostics;
        std::vector<std::unique_ptr<Node>> storage;
        Node* root = nullptr;
        std::vector<Node*> enums;
        std::vector<Node*> defines;
        std::vector<Node*> instances;
        UInt32 rootCount = 0;
        String lastError;

        Node* create(NodeKind kind)
        {
            storage.push_back(std::make_unique<Node>());
            Node* node = storage.back().get();
            node->kind = kind;
            return node;
        }

        bool ok() const
        {
            for (const Diagnostic& diagnostic : diagnostics)
            {
                if (diagnostic.severity == 0)
                {
                    return false;
                }
            }
            return true;
        }

        void report(Int32 severity, Int32 line, Int32 column, const String& message)
        {
            Diagnostic diagnostic;
            diagnostic.severity = severity;
            diagnostic.line = line;
            diagnostic.column = column;
            diagnostic.file = file;
            diagnostic.message = message;
            diagnostics.push_back(std::move(diagnostic));
        }
    };
}

#endif
