#ifndef IZML_PARSER_IMPL_HPP
#define IZML_PARSER_IMPL_HPP

#include "izml_ast.hpp"
#include "izml_lexer.hpp"

namespace izml
{
    class Parser
    {
    public:
        Parser(Document& document, std::vector<Token> tokens, const String& source);

        void run();

    private:
        const Token& peek(size_t lookahead = 0) const;
        TokenKind kind_at(size_t lookahead = 0) const;
        bool check(TokenKind kind) const;
        bool match(TokenKind kind);
        Token take();
        void advance();
        bool expect(TokenKind kind, const char* what);
        bool take_identifier(String& out);
        void skip_separators();
        void error(const Token& token, const String& message);

        void parse_document();
        void parse_version(Node* root);
        void parse_declaration(Node* parent);
        void parse_enum(Node* parent);
        void parse_define(Node* parent, bool nested);
        void parse_field(Node* owner, Int32 id, bool optional);
        Node* parse_type();
        String parse_identifier();
        String parse_qualified_name();
        void parse_using(Node* parent);
        void parse_using_list(Node* instance);
        void parse_object(Node* instance);
        void parse_entry(Node* container);
        Node* parse_value();
        Node* literal_from_token(const Token& token);
        Node* parse_object_value();
        Node* parse_array_value();

        Document& document_;
        std::vector<Token> tokens_;
        const String& source_;
        size_t cursor_ = 0;
    };
}

#endif
