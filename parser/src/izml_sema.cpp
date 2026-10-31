#include "izml_sema.hpp"

namespace izml
{
    namespace
    {
        Int32 builtin_size(const String& name)
        {
            if (name == "bool" || name == "byte" || name == "sbyte") return 1;
            if (name == "short" || name == "ushort") return 2;
            if (name == "int" || name == "uint" || name == "float") return 4;
            if (name == "long" || name == "ulong" || name == "double") return 8;
            return 0;
        }

        bool is_builtin(const String& name)
        {
            static const char* names[] =
            {
                "bool", "byte", "sbyte", "short", "ushort", "int", "uint", "long", "ulong", "float", "double", "string"
            };
            for (const char* candidate : names)
            {
                if (name == candidate)
                {
                    return true;
                }
            }
            return false;
        }

        bool is_integer_builtin(const String& name)
        {
            return builtin_size(name) != 0 && name != "float" && name != "double" && name != "bool";
        }

        bool is_numeric_builtin(const String& name)
        {
            return builtin_size(name) != 0 && name != "bool";
        }
    }

    Sema::Sema(Document& document) : document_(document)
    {
    }

    void Sema::run()
    {
        if (document_.root == nullptr)
        {
            return;
        }
        collect(document_.root, "");
        validate_enums();
        for (size_t i = 0; i < document_.defines.size(); ++i)
        {
            document_.defines[i]->set("type_tag", std::to_string(static_cast<Int32>(i + 1)));
        }
        resolve_defines();
        if (!document_.ok())
        {
            return;
        }
        for (Node* instance : document_.instances)
        {
            check_using(instance);
        }
        UInt32 rootCount = 0;
        for (Node* instance : document_.instances)
        {
            if (instance->flag("is_list"))
            {
                instance->set("root_index", "");
                continue;
            }
            instance->set("root_index", std::to_string(rootCount));
            ++rootCount;
        }
        document_.rootCount = rootCount;
    }

    void Sema::collect(Node* parent, const String& prefix)
    {
        for (Node* child : parent->children)
        {
            if (child->kind == NodeKind::Enum)
            {
                if (enums_.count(child->name) != 0)
                {
                    report(child, "duplicate enum '" + child->name + "'");
                }
                else
                {
                    enums_[child->name] = child;
                    document_.enums.push_back(child);
                }
                continue;
            }
            if (child->kind == NodeKind::Define)
            {
                const String qualified = prefix.empty() ? child->name : prefix + "." + child->name;
                child->set("qualified", qualified);
                if (defines_.count(qualified) != 0)
                {
                    report(child, "duplicate define '" + qualified + "'");
                }
                else
                {
                    defines_[qualified] = child;
                }
                if (prefix.empty())
                {
                    document_.defines.push_back(child);
                }
                else
                {
                    child->set("nested", "1");
                    child->set("type_tag", "0");
                }
                collect(child, qualified);
                continue;
            }
            if (child->kind == NodeKind::Using)
            {
                if (instances_.count(child->name) != 0)
                {
                    report(child, "duplicate instance '" + child->name + "'");
                }
                else
                {
                    instances_[child->name] = child;
                    document_.instances.push_back(child);
                }
            }
        }
    }

    void Sema::validate_enums()
    {
        for (Node* node : document_.enums)
        {
            String base = node->attr_or("base_type", "byte");
            if (!is_integer_builtin(base))
            {
                report(node, "enum '" + node->name + "' has invalid underlying type '" + base + "'");
                base = "byte";
            }
            node->set("base_kind", base);
            node->set("base_size", std::to_string(builtin_size(base)));
            std::unordered_set<String> members;
            for (Node* member : node->children)
            {
                if (member->kind != NodeKind::EnumMember)
                {
                    continue;
                }
                if (members.count(member->name) != 0)
                {
                    report(member, "duplicate enum member '" + member->name + "'");
                }
                members.insert(member->name);
            }
        }
    }

    void Sema::resolve_defines()
    {
        for (Node* define : document_.defines)
        {
            resolve_define(define);
        }
        for (const auto& pair : defines_)
        {
            resolve_define(pair.second);
        }
    }

    Node* Sema::resolve_base(Node* define) const
    {
        const String* base = define->attr("base");
        if (base == nullptr || base->empty())
        {
            return nullptr;
        }
        String context;
        if (define->parent != nullptr && define->parent->kind == NodeKind::Define)
        {
            context = define->parent->attr_or("qualified", "");
        }
        return lookup_define(*base, context);
    }

    bool Sema::resolve_define(Node* define)
    {
        if (resolved_.count(define) != 0)
        {
            return true;
        }
        if (visiting_.count(define) != 0)
        {
            report(define, "inheritance cycle involving '" + define->attr_or("qualified", define->name) + "'");
            return false;
        }
        visiting_.insert(define);

        Node* base = resolve_base(define);
        if (define->attr("base") != nullptr && !define->attr("base")->empty() && base == nullptr)
        {
            report(define, "unknown base define '" + *define->attr("base") + "'");
        }
        if (base != nullptr)
        {
            define->set("base_name", base->attr_or("qualified", base->name));
            resolve_define(base);
        }
        else
        {
            define->set("base_name", "");
        }

        const String context = define->attr_or("qualified", define->name);
        std::vector<Node*> own;
        for (Node* child : define->children)
        {
            if (child->kind == NodeKind::Field)
            {
                resolve_field(child, context);
                own.push_back(child);
            }
        }

        std::vector<Node*> effective;
        if (base != nullptr && resolved_.count(base) != 0)
        {
            effective = effective_[base];
        }
        effective.insert(effective.end(), own.begin(), own.end());

        std::unordered_set<String> ids;
        std::unordered_set<String> names;
        for (Node* field : effective)
        {
            const String* id = field->attr("id");
            if (id != nullptr && !id->empty())
            {
                if (ids.count(*id) != 0)
                {
                    report(field, "duplicate field id [" + *id + "] in '" + context + "'");
                }
                ids.insert(*id);
            }
            if (names.count(field->name) != 0)
            {
                report(field, "duplicate field name '" + field->name + "' in '" + context + "'");
            }
            names.insert(field->name);
        }

        Int32 optionalCount = 0;
        for (Node* field : effective)
        {
            const bool optional = field->flag("optional");
            field->set("optional", optional ? "1" : "0");
            field->set("opt_bit", optional ? std::to_string(optionalCount) : "");
            if (optional)
            {
                ++optionalCount;
            }
        }

        std::vector<Node*> layout;
        for (Node* field : effective)
        {
            if (field->attr_or("kind", "") != "list")
            {
                layout.push_back(field);
            }
        }
        for (Node* field : effective)
        {
            if (field->attr_or("kind", "") == "list")
            {
                layout.push_back(field);
            }
        }

        define->layout = layout;
        define->set("field_count", std::to_string(static_cast<Int32>(layout.size())));
        define->set("optional_count", std::to_string(optionalCount));
        define->set("has_optional", optionalCount != 0 ? "1" : "0");
        effective_[define] = effective;
        resolved_.insert(define);
        visiting_.erase(define);
        return true;
    }

    void Sema::resolve_field(Node* field, const String& context)
    {
        if (field->attr("id") == nullptr)
        {
            report(field, "field '" + field->name + "' is missing [id]");
        }
        if (!field->flag("optional"))
        {
            field->set("optional", "0");
        }
        Node* type = field->type_child();
        if (type == nullptr)
        {
            report(field, "field '" + field->name + "' has no type");
            return;
        }
        SemaType info;
        if (!resolve_type(type, context, info))
        {
            return;
        }
        field->set("kind", info.kind);
        field->set("type_name", info.name);
        if (info.kind == "list")
        {
            field->set("element_kind", info.elementKind);
            field->set("element_name", info.elementName);
            field->set("type_tag", "0");
            field->set("element_tag", std::to_string(info.elementTag));
            field->set("size", "0");
            return;
        }
        field->set("type_tag", std::to_string(info.typeTag));
        Int32 size = 0;
        if (info.kind == "enum")
        {
            auto found = enums_.find(info.name);
            if (found != enums_.end())
            {
                size = found->second->attr_int("base_size", 1);
            }
        }
        else if (info.kind != "define" && info.kind != "string")
        {
            size = builtin_size(info.kind);
        }
        field->set("size", std::to_string(size));
    }

    bool Sema::resolve_type(Node* type, const String& context, SemaType& out)
    {
        if (type->flag("is_list"))
        {
            Node* element = type->first_child();
            if (element == nullptr)
            {
                report(type, "list type requires an element type");
                return false;
            }
            SemaType inner;
            if (!resolve_type(element, context, inner))
            {
                return false;
            }
            out.kind = "list";
            out.name = inner.name;
            out.elementKind = inner.kind;
            out.elementName = inner.name;
            out.elementTag = inner.typeTag;
            type->set("kind", "list");
            type->set("element_kind", inner.kind);
            type->set("element_name", inner.name);
            type->set("element_tag", std::to_string(inner.typeTag));
            return true;
        }
        return resolve_atom(type, context, out);
    }

    bool Sema::resolve_atom(Node* type, const String& context, SemaType& out)
    {
        const String name = type->text;
        if (is_builtin(name))
        {
            out.kind = name;
            out.name = name;
            type->set("kind", name);
            type->set("size", std::to_string(builtin_size(name)));
            return true;
        }
        if (Node* define = lookup_define(name, context))
        {
            out.kind = "define";
            out.name = define->attr_or("qualified", define->name);
            out.typeTag = define->attr_int("type_tag", 0);
            type->set("kind", "define");
            type->set("type_name", out.name);
            type->set("type_tag", std::to_string(out.typeTag));
            type->set("size", "0");
            return true;
        }
        auto found = enums_.find(name);
        if (found != enums_.end())
        {
            out.kind = "enum";
            out.name = name;
            type->set("kind", "enum");
            type->set("type_name", name);
            type->set("size", found->second->attr_or("base_size", "1"));
            return true;
        }
        report(type, "unknown type '" + name + "'");
        return false;
    }

    Node* Sema::lookup_define(const String& name, const String& context) const
    {
        if (name.empty())
        {
            return nullptr;
        }
        auto direct = defines_.find(name);
        if (direct != defines_.end())
        {
            return direct->second;
        }
        String scope = context;
        while (!scope.empty())
        {
            auto found = defines_.find(scope + "." + name);
            if (found != defines_.end())
            {
                return found->second;
            }
            const size_t dot = scope.find_last_of('.');
            if (dot == String::npos)
            {
                break;
            }
            scope.erase(dot);
        }
        return nullptr;
    }

    std::vector<Node*> Sema::effective_fields(Node* define) const
    {
        auto found = effective_.find(define);
        return found != effective_.end() ? found->second : std::vector<Node*>();
    }

    Node* Sema::find_field(Node* define, const String& name) const
    {
        for (Node* field : effective_fields(define))
        {
            if (field->name == name)
            {
                return field;
            }
        }
        return nullptr;
    }

    bool Sema::derives(Node* type, Node* base) const
    {
        Node* current = type;
        while (current != nullptr)
        {
            if (current == base)
            {
                return true;
            }
            const String baseName = current->attr_or("base_name", "");
            if (baseName.empty())
            {
                return false;
            }
            auto found = defines_.find(baseName);
            if (found == defines_.end())
            {
                return false;
            }
            current = found->second;
        }
        return false;
    }

    void Sema::check_using(Node* instance)
    {
        if (instance->flag("is_list"))
        {
            const String elementName = instance->attr_or("element_name", "");
            Node* element = lookup_define(elementName, "");
            if (element == nullptr)
            {
                report(instance, "unknown type '" + elementName + "'");
                return;
            }
            instance->set("element_name", element->attr_or("qualified", element->name));
            instance->set("element_tag", element->attr_or("type_tag", "0"));
            std::size_t current = 0;
            for (std::size_t i = 0; i < document_.instances.size(); ++i)
            {
                if (document_.instances[i] == instance)
                {
                    current = i;
                    break;
                }
            }
            for (Node* ref : instance->children)
            {
                if (ref->kind != NodeKind::Ref)
                {
                    continue;
                }
                auto found = instances_.find(ref->text);
                if (found == instances_.end())
                {
                    report(ref, "unknown instance '" + ref->text + "'");
                    continue;
                }
                Node* target = found->second;
                std::size_t position = 0;
                for (std::size_t i = 0; i < document_.instances.size(); ++i)
                {
                    if (document_.instances[i] == target)
                    {
                        position = i;
                        break;
                    }
                }
                if (position >= current)
                {
                    report(ref, "instance '" + ref->text + "' must be declared before the list");
                }
                if (target->flag("is_list"))
                {
                    report(ref, "instance '" + ref->text + "' is a list and cannot be a list element");
                    continue;
                }
                Node* targetType = lookup_define(target->attr_or("type_name", ""), "");
                if (targetType == nullptr)
                {
                    continue;
                }
                if (!derives(targetType, element))
                {
                    report(ref, "instance '" + ref->text + "' of type '" + targetType->name + "' is not compatible with '" + element->name + "'");
                }
            }
            return;
        }

        const String typeName = instance->attr_or("type_name", "");
        Node* define = lookup_define(typeName, "");
        if (define == nullptr)
        {
            report(instance, "unknown type '" + typeName + "'");
            return;
        }
        instance->set("type_name", define->attr_or("qualified", define->name));
        instance->set("type_tag", define->attr_or("type_tag", "0"));
        check_object(instance, define, define->attr_or("qualified", define->name));
    }

    void Sema::check_object(Node* object, Node* define, const String& context)
    {
        std::vector<Node*> fields = effective_fields(define);
        std::unordered_set<String> seen;
        for (Node* child : object->children)
        {
            if (child->kind != NodeKind::Entry)
            {
                continue;
            }
            Node* field = find_field(define, child->name);
            if (field == nullptr)
            {
                report(child, "unknown field '" + child->name + "' in '" + context + "'");
                continue;
            }
            if (seen.count(child->name) != 0)
            {
                report(child, "duplicate value for field '" + child->name + "'");
                continue;
            }
            seen.insert(child->name);
            Node* value = child->value_child();
            if (value == nullptr)
            {
                report(child, "field '" + child->name + "' has no value");
                continue;
            }
            check_value(value, field);
        }
        for (Node* field : fields)
        {
            if (!field->flag("optional") && seen.count(field->name) == 0)
            {
                report(object, "missing required field '" + field->name + "' in '" + context + "'");
            }
        }
    }

    void Sema::check_value(Node* value, Node* field)
    {
        const String kind = field->attr_or("kind", "");
        if (kind == "list")
        {
            if (value->kind != NodeKind::Array)
            {
                report(value, "field '" + field->name + "' expects a list");
                return;
            }
            const String elementKind = field->attr_or("element_kind", "");
            const String elementName = field->attr_or("element_name", "");
            for (Node* element : value->children)
            {
                check_element(element, elementKind, elementName);
            }
            return;
        }
        check_element(value, kind, field->attr_or("type_name", ""));
    }

    void Sema::check_element(Node* value, const String& kind, const String& name)
    {
        if (kind == "bool")
        {
            if (value->kind != NodeKind::Bool)
            {
                report(value, "expected a bool value");
            }
            return;
        }
        if (kind == "float" || kind == "double")
        {
            if (value->kind != NodeKind::Int && value->kind != NodeKind::Float)
            {
                report(value, "expected a numeric value");
            }
            return;
        }
        if (is_numeric_builtin(kind))
        {
            if (value->kind != NodeKind::Int)
            {
                report(value, "expected an integer value");
            }
            return;
        }
        if (kind == "string")
        {
            if (value->kind != NodeKind::Str)
            {
                report(value, "expected a string value");
            }
            return;
        }
        if (kind == "enum")
        {
            if (value->kind != NodeKind::Ref)
            {
                report(value, "expected an enum member");
                return;
            }
            Node* enumNode = nullptr;
            auto found = enums_.find(name);
            if (found != enums_.end())
            {
                enumNode = found->second;
            }
            if (enumNode == nullptr)
            {
                report(value, "unknown enum '" + name + "'");
                return;
            }
            Int64 memberValue = 0;
            if (!enum_member(enumNode, value->text, memberValue))
            {
                report(value, "unknown enum member '" + value->text + "' in '" + name + "'");
            }
            return;
        }
        if (kind == "define")
        {
            if (value->kind != NodeKind::Object)
            {
                report(value, "expected an object value");
                return;
            }
            Node* define = lookup_define(name, "");
            if (define == nullptr)
            {
                report(value, "unknown define '" + name + "'");
                return;
            }
            check_object(value, define, define->attr_or("qualified", define->name));
            return;
        }
        report(value, "unsupported field type '" + kind + "'");
    }

    bool Sema::enum_member(Node* enumNode, const String& text, Int64& out) const
    {
        String member = text;
        const size_t dot = text.find_last_of('.');
        if (dot != String::npos)
        {
            if (text.substr(0, dot) != enumNode->name)
            {
                return false;
            }
            member = text.substr(dot + 1);
        }
        for (Node* child : enumNode->children)
        {
            if (child->kind == NodeKind::EnumMember && child->name == member)
            {
                out = child->intValue;
                return true;
            }
        }
        return false;
    }

    void Sema::report(const Node* node, const String& message)
    {
        const Int32 line = node != nullptr ? node->line : 0;
        const Int32 column = node != nullptr ? node->column : 0;
        document_.report(0, line, column, message);
    }
}
