#include "izml_ast.hpp"

namespace izml
{
    const char* node_kind_name(NodeKind kind)
    {
        switch (kind)
        {
        case NodeKind::Document: return "document";
        case NodeKind::Version: return "version";
        case NodeKind::Enum: return "enum";
        case NodeKind::EnumMember: return "enum-member";
        case NodeKind::Define: return "define";
        case NodeKind::Field: return "field";
        case NodeKind::Type: return "type";
        case NodeKind::Using: return "using";
        case NodeKind::Int: return "int";
        case NodeKind::Float: return "float";
        case NodeKind::Str: return "string";
        case NodeKind::Bool: return "bool";
        case NodeKind::Ref: return "ref";
        case NodeKind::Object: return "object";
        case NodeKind::Array: return "array";
        case NodeKind::Entry: return "entry";
        }
        return "unknown";
    }

    String render_type(const Node* type)
    {
        if (type == nullptr)
        {
            return String();
        }
        if (type->kind == NodeKind::Type)
        {
            return type->text;
        }
        return type->text.empty() ? type->name : type->text;
    }
}
