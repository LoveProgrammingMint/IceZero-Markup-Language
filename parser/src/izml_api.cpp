#include "izml_parser_c_api.h"
#include "izml_ast.hpp"
#include "izml_lexer.hpp"
#include "izml_parser.hpp"
#include "izml_sema.hpp"

using namespace izml;

namespace
{
    Document* to_document(izml_document* handle)
    {
        return reinterpret_cast<Document*>(handle);
    }

    const Document* to_document(const izml_document* handle)
    {
        return reinterpret_cast<const Document*>(handle);
    }

    const Node* to_node(const izml_node* handle)
    {
        return reinterpret_cast<const Node*>(handle);
    }

    Node* to_node(izml_node* handle)
    {
        return reinterpret_cast<Node*>(handle);
    }

    Document* build_document(const String& source, const String& file)
    {
        Document* document = new Document();
        document->file = file;
        try
        {
            Lexer lexer(source, document->file, document->diagnostics);
            std::vector<Token> tokens = lexer.run();
            if (document->ok())
            {
                Parser parser(*document, std::move(tokens), source);
                parser.run();
            }
            if (document->ok())
            {
                Sema sema(*document);
                sema.run();
            }
        }
        catch (...)
        {
            document->report(0, 0, 0, "internal parser failure");
        }
        for (const Diagnostic& diagnostic : document->diagnostics)
        {
            if (diagnostic.severity == 0)
            {
                document->lastError = diagnostic.formatted();
                break;
            }
        }
        return document;
    }

    const char* node_text(const Node* node)
    {
        return node != nullptr ? node->text.c_str() : "";
    }
}

const char* IZML_CALL izml_version(void)
{
    return "0.1.0";
}

izml_document* IZML_CALL izml_parse_text(const char* utf8_text, const char* file_name)
{
    if (utf8_text == nullptr)
    {
        return nullptr;
    }
    return reinterpret_cast<izml_document*>(build_document(String(utf8_text), file_name != nullptr ? String(file_name) : String("<memory>")));
}

izml_document* IZML_CALL izml_parse_file(const char* path)
{
    if (path == nullptr)
    {
        return nullptr;
    }
    const String file(path);
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
    {
        Document* document = new Document();
        document->file = file;
        document->report(0, 0, 0, "unable to open file '" + file + "'");
        document->lastError = document->diagnostics.front().formatted();
        return reinterpret_cast<izml_document*>(document);
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    String source = buffer.str();
    if (source.size() >= 3 && static_cast<UInt8>(source[0]) == 0xEF && static_cast<UInt8>(source[1]) == 0xBB && static_cast<UInt8>(source[2]) == 0xBF)
    {
        source.erase(0, 3);
    }
    return reinterpret_cast<izml_document*>(build_document(source, file));
}

void IZML_CALL izml_document_destroy(izml_document* document)
{
    delete to_document(document);
}

const char* IZML_CALL izml_last_error(const izml_document* document)
{
    const Document* target = to_document(document);
    return target != nullptr ? target->lastError.c_str() : "";
}

int32_t IZML_CALL izml_document_ok(const izml_document* document)
{
    const Document* target = to_document(document);
    return target != nullptr && target->ok() ? 1 : 0;
}

int32_t IZML_CALL izml_diagnostic_count(const izml_document* document)
{
    const Document* target = to_document(document);
    return target != nullptr ? static_cast<int32_t>(target->diagnostics.size()) : 0;
}

int32_t IZML_CALL izml_diagnostic_get(const izml_document* document, int32_t index, izml_diagnostic* out)
{
    const Document* target = to_document(document);
    if (target == nullptr || index < 0 || static_cast<size_t>(index) >= target->diagnostics.size())
    {
        return 0;
    }
    const Diagnostic& diagnostic = target->diagnostics[static_cast<size_t>(index)];
    if (out != nullptr)
    {
        out->severity = diagnostic.severity;
        out->line = diagnostic.line;
        out->column = diagnostic.column;
        out->message = diagnostic.message.c_str();
        out->file = diagnostic.file.c_str();
    }
    return 1;
}

const char* IZML_CALL izml_document_version(const izml_document* document)
{
    const Document* target = to_document(document);
    return target != nullptr ? target->version.c_str() : "";
}

const char* IZML_CALL izml_document_file(const izml_document* document)
{
    const Document* target = to_document(document);
    return target != nullptr ? target->file.c_str() : "";
}

izml_node* IZML_CALL izml_document_root(izml_document* document)
{
    Document* target = to_document(document);
    return target != nullptr ? reinterpret_cast<izml_node*>(target->root) : nullptr;
}

int32_t IZML_CALL izml_document_root_count(const izml_document* document)
{
    const Document* target = to_document(document);
    return target != nullptr ? static_cast<int32_t>(target->rootCount) : 0;
}

int32_t IZML_CALL izml_enum_count(const izml_document* document)
{
    const Document* target = to_document(document);
    return target != nullptr ? static_cast<int32_t>(target->enums.size()) : 0;
}

izml_node* IZML_CALL izml_enum_at(izml_document* document, int32_t index)
{
    Document* target = to_document(document);
    if (target == nullptr || index < 0 || static_cast<size_t>(index) >= target->enums.size())
    {
        return nullptr;
    }
    return reinterpret_cast<izml_node*>(target->enums[static_cast<size_t>(index)]);
}

izml_node* IZML_CALL izml_find_enum(izml_document* document, const char* name)
{
    Document* target = to_document(document);
    if (target == nullptr || name == nullptr)
    {
        return nullptr;
    }
    for (Node* node : target->enums)
    {
        if (node->name == name)
        {
            return reinterpret_cast<izml_node*>(node);
        }
    }
    return nullptr;
}

int32_t IZML_CALL izml_define_count(const izml_document* document)
{
    const Document* target = to_document(document);
    return target != nullptr ? static_cast<int32_t>(target->defines.size()) : 0;
}

izml_node* IZML_CALL izml_define_at(izml_document* document, int32_t index)
{
    Document* target = to_document(document);
    if (target == nullptr || index < 0 || static_cast<size_t>(index) >= target->defines.size())
    {
        return nullptr;
    }
    return reinterpret_cast<izml_node*>(target->defines[static_cast<size_t>(index)]);
}

izml_node* IZML_CALL izml_find_define(izml_document* document, const char* qualified_name)
{
    Document* target = to_document(document);
    if (target == nullptr || qualified_name == nullptr || *qualified_name == '\0')
    {
        return nullptr;
    }
    const String wanted(qualified_name);
    std::vector<Node*> pending(target->defines.begin(), target->defines.end());
    while (!pending.empty())
    {
        Node* define = pending.back();
        pending.pop_back();
        if (define->attr_or("qualified", define->name) == wanted || define->name == wanted)
        {
            return reinterpret_cast<izml_node*>(define);
        }
        for (Node* child : define->children)
        {
            if (child->kind == NodeKind::Define)
            {
                pending.push_back(child);
            }
        }
    }
    return nullptr;
}

int32_t IZML_CALL izml_define_layout_count(const izml_node* define)
{
    const Node* target = to_node(define);
    return target != nullptr ? static_cast<int32_t>(target->layout.size()) : 0;
}

izml_node* IZML_CALL izml_define_layout_field(const izml_node* define, int32_t index)
{
    const Node* target = to_node(define);
    if (target == nullptr || index < 0 || static_cast<size_t>(index) >= target->layout.size())
    {
        return nullptr;
    }
    return reinterpret_cast<izml_node*>(target->layout[static_cast<size_t>(index)]);
}

int32_t IZML_CALL izml_using_count(const izml_document* document)
{
    const Document* target = to_document(document);
    return target != nullptr ? static_cast<int32_t>(target->instances.size()) : 0;
}

izml_node* IZML_CALL izml_using_at(izml_document* document, int32_t index)
{
    Document* target = to_document(document);
    if (target == nullptr || index < 0 || static_cast<size_t>(index) >= target->instances.size())
    {
        return nullptr;
    }
    return reinterpret_cast<izml_node*>(target->instances[static_cast<size_t>(index)]);
}

int32_t IZML_CALL izml_node_kind(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr ? static_cast<int32_t>(target->kind) : -1;
}

const char* IZML_CALL izml_node_name(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr ? target->name.c_str() : "";
}

const char* IZML_CALL izml_node_text(const izml_node* node)
{
    return node_text(to_node(node));
}

const char* IZML_CALL izml_node_type_text(const izml_node* node)
{
    const Node* target = to_node(node);
    if (target == nullptr)
    {
        return "";
    }
    if (target->kind == NodeKind::Type)
    {
        return target->text.c_str();
    }
    static thread_local String buffer;
    buffer = render_type(target->type_child());
    return buffer.c_str();
}

int64_t IZML_CALL izml_node_int(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr ? target->intValue : 0;
}

double IZML_CALL izml_node_float(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr ? target->floatValue : 0.0;
}

int32_t IZML_CALL izml_node_bool(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr && target->boolValue ? 1 : 0;
}

int32_t IZML_CALL izml_node_line(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr ? target->line : 0;
}

int32_t IZML_CALL izml_node_column(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr ? target->column : 0;
}

const char* IZML_CALL izml_node_attr(const izml_node* node, const char* key)
{
    const Node* target = to_node(node);
    if (target == nullptr || key == nullptr)
    {
        return "";
    }
    const String* value = target->attr(key);
    return value != nullptr ? value->c_str() : "";
}

int32_t IZML_CALL izml_node_attr_int(const izml_node* node, const char* key, int32_t fallback)
{
    const Node* target = to_node(node);
    if (target == nullptr || key == nullptr)
    {
        return fallback;
    }
    const String* value = target->attr(key);
    if (value == nullptr || value->empty())
    {
        return fallback;
    }
    return static_cast<int32_t>(std::strtol(value->c_str(), nullptr, 10));
}

izml_node* IZML_CALL izml_node_parent(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr ? reinterpret_cast<izml_node*>(target->parent) : nullptr;
}

int32_t IZML_CALL izml_node_child_count(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr ? static_cast<int32_t>(target->children.size()) : 0;
}

izml_node* IZML_CALL izml_node_child(const izml_node* node, int32_t index)
{
    const Node* target = to_node(node);
    if (target == nullptr || index < 0 || static_cast<size_t>(index) >= target->children.size())
    {
        return nullptr;
    }
    return reinterpret_cast<izml_node*>(target->children[static_cast<size_t>(index)]);
}

izml_node* IZML_CALL izml_node_find_child(const izml_node* node, const char* name)
{
    const Node* target = to_node(node);
    if (target == nullptr || name == nullptr)
    {
        return nullptr;
    }
    return reinterpret_cast<izml_node*>(target->find_child(name));
}

izml_node* IZML_CALL izml_node_type(const izml_node* node)
{
    const Node* target = to_node(node);
    return target != nullptr ? reinterpret_cast<izml_node*>(target->type_child()) : nullptr;
}
