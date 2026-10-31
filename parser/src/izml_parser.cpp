#include "izml_parser.hpp"

namespace izml
{
    Parser::Parser(Document& document, std::vector<Token> tokens, const String& source)
        : document_(document), tokens_(std::move(tokens)), source_(source)
    {
    }

    const Token& Parser::peek(size_t lookahead) const
    {
        const size_t position = cursor_ + lookahead;
        return position < tokens_.size() ? tokens_[position] : tokens_.back();
    }

    TokenKind Parser::kind_at(size_t lookahead) const
    {
        return peek(lookahead).kind;
    }

    bool Parser::check(TokenKind kind) const
    {
        return kind_at(0) == kind;
    }

    bool Parser::match(TokenKind kind)
    {
        if (!check(kind))
        {
            return false;
        }
        advance();
        return true;
    }

    Token Parser::take()
    {
        Token token = peek();
        advance();
        return token;
    }

    void Parser::advance()
    {
        if (cursor_ < tokens_.size())
        {
            ++cursor_;
        }
    }

    bool Parser::expect(TokenKind kind, const char* what)
    {
        if (!check(kind))
        {
            error(peek(), String("expected ") + what + ", found " + token_kind_name(kind_at(0)));
            return false;
        }
        advance();
        return true;
    }

    bool Parser::take_identifier(String& out)
    {
        if (!check(TokenKind::Identifier))
        {
            error(peek(), String("expected identifier, found ") + token_kind_name(kind_at(0)));
            return false;
        }
        out = take().text;
        return true;
    }

    void Parser::skip_separators()
    {
        while (check(TokenKind::Newline) || check(TokenKind::Comma) || check(TokenKind::Semicolon))
        {
            advance();
        }
    }

    void Parser::error(const Token& token, const String& message)
    {
        document_.report(0, token.line, token.column, message);
    }

    void Parser::run()
    {
        parse_document();
    }

    void Parser::parse_document()
    {
        document_.root = document_.create(NodeKind::Document);
        document_.root->line = 1;
        document_.root->column = 1;
        parse_version(document_.root);
        while (!check(TokenKind::End))
        {
            skip_separators();
            if (check(TokenKind::End))
            {
                break;
            }
            parse_declaration(document_.root);
        }
    }

    void Parser::parse_version(Node* root)
    {
        skip_separators();
        Node* version = document_.create(NodeKind::Version);
        version->line = peek().line;
        version->column = peek().column;
        root->add(version);
        if (!expect(TokenKind::At, "'@'"))
        {
            return;
        }
        String name;
        if (!take_identifier(name))
        {
            return;
        }
        if (name != "izml")
        {
            error(peek(), "expected 'izml' after '@'");
        }
        if (!expect(TokenKind::Version, "version literal"))
        {
            return;
        }
        const String low = tokens_[cursor_ - 1].text;
        if (!expect(TokenKind::DotDot, "'..'"))
        {
            return;
        }
        if (!expect(TokenKind::Version, "version literal"))
        {
            return;
        }
        const String high = tokens_[cursor_ - 1].text;
        document_.version = low + ".." + high;
        version->text = document_.version;
        skip_separators();
    }

    void Parser::parse_declaration(Node* parent)
    {
        if (check(TokenKind::KwEnum))
        {
            parse_enum(parent);
            return;
        }
        if (check(TokenKind::KwDefine))
        {
            parse_define(parent, false);
            return;
        }
        if (check(TokenKind::KwUsing))
        {
            parse_using(parent);
            return;
        }
        error(peek(), String("expected 'enum', 'define' or 'using', found ") + token_kind_name(kind_at(0)));
        while (!check(TokenKind::End) && !check(TokenKind::Newline) && !check(TokenKind::RBrace))
        {
            advance();
        }
    }

    void Parser::parse_enum(Node* parent)
    {
        take();
        Node* node = document_.create(NodeKind::Enum);
        node->line = peek().line;
        node->column = peek().column;
        if (!take_identifier(node->name))
        {
            node->name = "<error>";
        }
        if (!expect(TokenKind::Colon, "':'"))
        {
            node->set("base_type", "byte");
        }
        else
        {
            String base;
            if (take_identifier(base))
            {
                node->set("base_type", base);
            }
        }
        parent->add(node);
        if (!expect(TokenKind::LBrace, "'{'"))
        {
            return;
        }
        while (!check(TokenKind::End) && !check(TokenKind::RBrace))
        {
            skip_separators();
            if (check(TokenKind::RBrace) || check(TokenKind::End))
            {
                break;
            }
            Node* member = document_.create(NodeKind::EnumMember);
            member->line = peek().line;
            member->column = peek().column;
            if (!take_identifier(member->name))
            {
                member->name = "<error>";
            }
            if (!expect(TokenKind::Assign, "'='"))
            {
                node->add(member);
                return;
            }
            bool negative = match(TokenKind::Minus);
            if (!expect(TokenKind::Int, "integer literal"))
            {
                node->add(member);
                return;
            }
            member->intValue = (negative ? -1 : 1) * tokens_[cursor_ - 1].intValue;
            node->add(member);
        }
        expect(TokenKind::RBrace, "'}'");
        skip_separators();
    }

    void Parser::parse_define(Node* parent, bool nested)
    {
        take();
        Node* node = document_.create(NodeKind::Define);
        node->line = peek().line;
        node->column = peek().column;
        if (!take_identifier(node->name))
        {
            node->name = "<error>";
        }
        if (nested)
        {
            node->set("nested", "1");
        }
        if (match(TokenKind::Colon))
        {
            String base;
            if (take_identifier(base))
            {
                node->set("base", base);
            }
        }
        parent->add(node);
        if (!expect(TokenKind::LBrace, "'{'"))
        {
            return;
        }
        Int32 pendingId = -1;
        bool pendingOptional = false;
        while (!check(TokenKind::End) && !check(TokenKind::RBrace))
        {
            if (check(TokenKind::Newline) || check(TokenKind::Comma) || check(TokenKind::Semicolon))
            {
                advance();
                continue;
            }
            if (check(TokenKind::LBracket))
            {
                take();
                String key;
                if (!take_identifier(key) || key != "id")
                {
                    error(peek(), "expected '[id: <int>]'");
                }
                if (!expect(TokenKind::Colon, "':'"))
                {
                    return;
                }
                bool negative = match(TokenKind::Minus);
                if (!expect(TokenKind::Int, "integer literal"))
                {
                    return;
                }
                pendingId = static_cast<Int32>((negative ? -1 : 1) * tokens_[cursor_ - 1].intValue);
                expect(TokenKind::RBracket, "']'");
                continue;
            }
            if (match(TokenKind::KwPublic))
            {
                continue;
            }
            if (match(TokenKind::KwOptional))
            {
                pendingOptional = true;
                continue;
            }
            if (check(TokenKind::KwDefine))
            {
                parse_define(node, true);
                continue;
            }
            parse_field(node, pendingId, pendingOptional);
            pendingId = -1;
            pendingOptional = false;
        }
        expect(TokenKind::RBrace, "'}'");
        skip_separators();
    }

    void Parser::parse_field(Node* owner, Int32 id, bool optional)
    {
        Node* type = parse_type();
        Node* field = document_.create(NodeKind::Field);
        field->line = type->line;
        field->column = type->column;
        if (!take_identifier(field->name))
        {
            field->name = "<error>";
        }
        if (id >= 0)
        {
            field->set("id", std::to_string(id));
        }
        if (optional)
        {
            field->set("optional", "1");
        }
        field->add(type);
        owner->add(field);
        match(TokenKind::Semicolon);
    }

    Node* Parser::parse_type()
    {
        Node* type = document_.create(NodeKind::Type);
        type->line = peek().line;
        type->column = peek().column;
        if (match(TokenKind::KwList))
        {
            if (!expect(TokenKind::Lt, "'<'"))
            {
                type->text = "list<?>";
                return type;
            }
            Node* element = parse_type();
            expect(TokenKind::Gt, "'>'");
            type->set("is_list", "1");
            type->add(element);
            type->text = "list<" + element->text + ">";
            return type;
        }
        String name;
        if (!take_identifier(name))
        {
            name = "<error>";
        }
        type->name = name;
        type->text = name;
        return type;
    }

    String Parser::parse_identifier()
    {
        String name;
        take_identifier(name);
        return name;
    }

    String Parser::parse_qualified_name()
    {
        String name = parse_identifier();
        while (match(TokenKind::Dot))
        {
            name += ".";
            name += parse_identifier();
        }
        return name;
    }

    void Parser::parse_using(Node* parent)
    {
        take();
        Node* node = document_.create(NodeKind::Using);
        node->line = peek().line;
        node->column = peek().column;
        if (match(TokenKind::KwList))
        {
            node->set("is_list", "1");
            expect(TokenKind::Lt, "'<'");
            String element;
            if (take_identifier(element))
            {
                node->set("element_name", element);
            }
            expect(TokenKind::Gt, "'>'");
        }
        else
        {
            String type;
            if (take_identifier(type))
            {
                node->set("type_name", type);
            }
        }
        if (!take_identifier(node->name))
        {
            node->name = "<error>";
        }
        parent->add(node);
        if (!expect(TokenKind::LBrace, "'{'"))
        {
            return;
        }
        if (node->flag("is_list"))
        {
            parse_using_list(node);
        }
        else
        {
            parse_object(node);
        }
        expect(TokenKind::RBrace, "'}'");
        skip_separators();
    }

    void Parser::parse_using_list(Node* instance)
    {
        while (!check(TokenKind::End) && !check(TokenKind::RBrace))
        {
            skip_separators();
            if (check(TokenKind::RBrace) || check(TokenKind::End))
            {
                break;
            }
            if (!check(TokenKind::Identifier))
            {
                error(peek(), String("expected instance reference, found ") + token_kind_name(kind_at(0)));
                return;
            }
            Node* ref = document_.create(NodeKind::Ref);
            ref->line = peek().line;
            ref->column = peek().column;
            ref->text = parse_qualified_name();
            instance->add(ref);
        }
    }

    void Parser::parse_object(Node* instance)
    {
        while (!check(TokenKind::End) && !check(TokenKind::RBrace))
        {
            skip_separators();
            if (check(TokenKind::RBrace) || check(TokenKind::End))
            {
                break;
            }
            parse_entry(instance);
        }
    }

    void Parser::parse_entry(Node* container)
    {
        Node* entry = document_.create(NodeKind::Entry);
        entry->line = peek().line;
        entry->column = peek().column;
        if (!take_identifier(entry->name))
        {
            entry->name = "<error>";
        }
        if (!expect(TokenKind::Colon, "':'"))
        {
            container->add(entry);
            return;
        }
        Node* value = parse_value();
        entry->add(value);
        container->add(entry);
    }

    Node* Parser::parse_value()
    {
        if (check(TokenKind::Int) || check(TokenKind::Float) || check(TokenKind::String) || check(TokenKind::KwTrue) || check(TokenKind::KwFalse))
        {
            return literal_from_token(take());
        }
        if (check(TokenKind::Minus))
        {
            const Token minus = take();
            if (check(TokenKind::Int) || check(TokenKind::Float))
            {
                Node* node = literal_from_token(take());
                node->intValue = -node->intValue;
                node->floatValue = -node->floatValue;
                return node;
            }
            error(minus, "expected numeric literal after '-'");
            return document_.create(NodeKind::Int);
        }
        if (check(TokenKind::Identifier))
        {
            Node* ref = document_.create(NodeKind::Ref);
            ref->line = peek().line;
            ref->column = peek().column;
            ref->text = parse_qualified_name();
            return ref;
        }
        if (check(TokenKind::LBrace))
        {
            return parse_object_value();
        }
        if (check(TokenKind::LBracket))
        {
            return parse_array_value();
        }
        error(peek(), String("expected value, found ") + token_kind_name(kind_at(0)));
        Node* empty = document_.create(NodeKind::Int);
        advance();
        return empty;
    }

    Node* Parser::literal_from_token(const Token& token)
    {
        Node* node = document_.create(token.kind == TokenKind::String ? NodeKind::Str :
            (token.kind == TokenKind::Float ? NodeKind::Float :
                (token.kind == TokenKind::Int ? NodeKind::Int : NodeKind::Bool)));
        node->line = token.line;
        node->column = token.column;
        node->intValue = token.intValue;
        node->floatValue = token.floatValue;
        node->text = token.text;
        node->boolValue = token.kind == TokenKind::KwTrue;
        return node;
    }

    Node* Parser::parse_object_value()
    {
        const Token brace = take();
        Node* object = document_.create(NodeKind::Object);
        object->line = brace.line;
        object->column = brace.column;
        while (!check(TokenKind::End) && !check(TokenKind::RBrace))
        {
            skip_separators();
            if (check(TokenKind::RBrace) || check(TokenKind::End))
            {
                break;
            }
            parse_entry(object);
        }
        expect(TokenKind::RBrace, "'}'");
        return object;
    }

    Node* Parser::parse_array_value()
    {
        const Token bracket = take();
        Node* array = document_.create(NodeKind::Array);
        array->line = bracket.line;
        array->column = bracket.column;
        while (!check(TokenKind::End) && !check(TokenKind::RBracket))
        {
            skip_separators();
            if (check(TokenKind::RBracket) || check(TokenKind::End))
            {
                break;
            }
            array->add(parse_value());
        }
        expect(TokenKind::RBracket, "']'");
        return array;
    }
}
