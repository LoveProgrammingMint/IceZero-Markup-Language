#ifndef IZML_PARSER_C_API_H
#define IZML_PARSER_C_API_H

#include <stdint.h>

#ifdef _WIN32
#  ifdef IZMLPARSER_EXPORTS
#    define IZML_API __declspec(dllexport)
#  else
#    define IZML_API __declspec(dllimport)
#  endif
#  define IZML_CALL __cdecl
#else
#  define IZML_API
#  define IZML_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct izml_document izml_document;
typedef struct izml_node izml_node;

typedef enum izml_node_kind_tag
{
    IZML_NODE_DOCUMENT = 0,
    IZML_NODE_VERSION = 1,
    IZML_NODE_ENUM = 2,
    IZML_NODE_ENUM_MEMBER = 3,
    IZML_NODE_DEFINE = 4,
    IZML_NODE_FIELD = 5,
    IZML_NODE_TYPE = 6,
    IZML_NODE_USING = 7,
    IZML_NODE_INT = 8,
    IZML_NODE_FLOAT = 9,
    IZML_NODE_STRING = 10,
    IZML_NODE_BOOL = 11,
    IZML_NODE_REF = 12,
    IZML_NODE_OBJECT = 13,
    IZML_NODE_ARRAY = 14,
    IZML_NODE_ENTRY = 15
} izml_node_kind_t;

typedef enum izml_severity
{
    IZML_SEVERITY_ERROR = 0,
    IZML_SEVERITY_WARNING = 1,
    IZML_SEVERITY_NOTE = 2
} izml_severity;

typedef struct izml_diagnostic
{
    int32_t severity;
    int32_t line;
    int32_t column;
    const char* message;
    const char* file;
} izml_diagnostic;

IZML_API const char* IZML_CALL izml_version(void);

IZML_API izml_document* IZML_CALL izml_parse_text(const char* utf8_text, const char* file_name);
IZML_API izml_document* IZML_CALL izml_parse_file(const char* path);
IZML_API void IZML_CALL izml_document_destroy(izml_document* document);

IZML_API const char* IZML_CALL izml_last_error(const izml_document* document);
IZML_API int32_t IZML_CALL izml_document_ok(const izml_document* document);
IZML_API int32_t IZML_CALL izml_diagnostic_count(const izml_document* document);
IZML_API int32_t IZML_CALL izml_diagnostic_get(const izml_document* document, int32_t index, izml_diagnostic* out);

IZML_API const char* IZML_CALL izml_document_version(const izml_document* document);
IZML_API const char* IZML_CALL izml_document_file(const izml_document* document);
IZML_API izml_node* IZML_CALL izml_document_root(izml_document* document);
IZML_API int32_t IZML_CALL izml_document_root_count(const izml_document* document);

IZML_API int32_t IZML_CALL izml_enum_count(const izml_document* document);
IZML_API izml_node* IZML_CALL izml_enum_at(izml_document* document, int32_t index);
IZML_API izml_node* IZML_CALL izml_find_enum(izml_document* document, const char* name);

IZML_API int32_t IZML_CALL izml_define_count(const izml_document* document);
IZML_API izml_node* IZML_CALL izml_define_at(izml_document* document, int32_t index);
IZML_API izml_node* IZML_CALL izml_find_define(izml_document* document, const char* qualified_name);
IZML_API int32_t IZML_CALL izml_define_layout_count(const izml_node* define);
IZML_API izml_node* IZML_CALL izml_define_layout_field(const izml_node* define, int32_t index);

IZML_API int32_t IZML_CALL izml_using_count(const izml_document* document);
IZML_API izml_node* IZML_CALL izml_using_at(izml_document* document, int32_t index);

IZML_API int32_t IZML_CALL izml_node_kind(const izml_node* node);
IZML_API const char* IZML_CALL izml_node_name(const izml_node* node);
IZML_API const char* IZML_CALL izml_node_text(const izml_node* node);
IZML_API const char* IZML_CALL izml_node_type_text(const izml_node* node);
IZML_API int64_t IZML_CALL izml_node_int(const izml_node* node);
IZML_API double IZML_CALL izml_node_float(const izml_node* node);
IZML_API int32_t IZML_CALL izml_node_bool(const izml_node* node);
IZML_API int32_t IZML_CALL izml_node_line(const izml_node* node);
IZML_API int32_t IZML_CALL izml_node_column(const izml_node* node);
IZML_API const char* IZML_CALL izml_node_attr(const izml_node* node, const char* key);
IZML_API int32_t IZML_CALL izml_node_attr_int(const izml_node* node, const char* key, int32_t fallback);

IZML_API izml_node* IZML_CALL izml_node_parent(const izml_node* node);
IZML_API int32_t IZML_CALL izml_node_child_count(const izml_node* node);
IZML_API izml_node* IZML_CALL izml_node_child(const izml_node* node, int32_t index);
IZML_API izml_node* IZML_CALL izml_node_find_child(const izml_node* node, const char* name);
IZML_API izml_node* IZML_CALL izml_node_type(const izml_node* node);

#ifdef __cplusplus
}
#endif

#endif
