#ifndef IZML_SEMA_HPP
#define IZML_SEMA_HPP

#include "izml_ast.hpp"

namespace izml
{
    struct SemaType
    {
        String kind;
        String name;
        String elementKind;
        String elementName;
        Int32 typeTag = 0;
        Int32 elementTag = 0;
    };

    class Sema
    {
    public:
        explicit Sema(Document& document);

        void run();

        bool derives(Node* type, Node* base) const;

    private:
        void collect(Node* parent, const String& prefix);
        void validate_enums();
        void resolve_defines();
        bool resolve_define(Node* define);
        Node* resolve_base(Node* define) const;
        void resolve_field(Node* field, const String& context);
        bool resolve_type(Node* type, const String& context, SemaType& out);
        bool resolve_atom(Node* type, const String& context, SemaType& out);
        Node* lookup_define(const String& name, const String& context) const;
        Node* find_field(Node* define, const String& name) const;
        std::vector<Node*> effective_fields(Node* define) const;
        void check_using(Node* instance);
        void check_object(Node* object, Node* define, const String& context);
        void check_value(Node* value, Node* field);
        void check_element(Node* value, const String& kind, const String& name);
        bool enum_member(Node* enumNode, const String& text, Int64& out) const;
        void report(const Node* node, const String& message);

        Document& document_;
        std::unordered_map<String, Node*> enums_;
        std::unordered_map<String, Node*> defines_;
        std::unordered_map<String, Node*> instances_;
        std::unordered_map<Node*, std::vector<Node*>> effective_;
        std::unordered_set<Node*> resolved_;
        std::unordered_set<Node*> visiting_;
    };
}

#endif
