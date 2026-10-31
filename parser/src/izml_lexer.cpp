#include "izml_lexer.hpp"

namespace izml
{
    namespace
    {
        struct KeywordEntry
        {
            const char* text;
            TokenKind kind;
        };

        const KeywordEntry keywordTable[] =
        {
            { "enum", TokenKind::KwEnum },
            { "define", TokenKind::KwDefine },
            { "using", TokenKind::KwUsing },
            { "public", TokenKind::KwPublic },
            { "optional", TokenKind::KwOptional },
            { "list", TokenKind::KwList },
            { "true", TokenKind::KwTrue },
            { "false", TokenKind::KwFalse }
        };

        Int32 decode_escape(const String& source, UInt32& index, String& out)
        {
            char c = source[index++];
            switch (c)
            {
            case 'n': out.push_back('\n'); return 0;
            case 't': out.push_back('\t'); return 0;
            case 'r': out.push_back('\r'); return 0;
            case '0': out.push_back('\0'); return 0;
            case '\\': out.push_back('\\'); return 0;
            case '\'': out.push_back('\''); return 0;
            case '"': out.push_back('"'); return 0;
            case 'x':
            {
                Int32 value = 0;
                Int32 digits = 0;
                while (digits < 2 && index < source.size() && std::isxdigit(static_cast<unsigned char>(source[index])))
                {
                    char hex = source[index++];
                    value = value * 16 + (hex <= '9' ? hex - '0' : (hex >= 'a' ? hex - 'a' + 10 : hex - 'A' + 10));
                    ++digits;
                }
                if (digits == 0)
                {
                    return 1;
                }
                out.push_back(static_cast<char>(value));
                return 0;
            }
            default:
                return 1;
            }
        }
    }

    const char* token_kind_name(TokenKind kind)
    {
        switch (kind)
        {
        case TokenKind::End: return "end of file";
        case TokenKind::Newline: return "newline";
        case TokenKind::Identifier: return "identifier";
        case TokenKind::Int: return "integer";
        case TokenKind::Float: return "float";
        case TokenKind::String: return "string";
        case TokenKind::Version: return "version";
        case TokenKind::At: return "'@'";
        case TokenKind::Colon: return "':'";
        case TokenKind::Semicolon: return "';'";
        case TokenKind::Comma: return "','";
        case TokenKind::Dot: return "'.'";
        case TokenKind::DotDot: return "'..'";
        case TokenKind::LBrace: return "'{'";
        case TokenKind::RBrace: return "'}'";
        case TokenKind::LBracket: return "'['";
        case TokenKind::RBracket: return "']'";
        case TokenKind::Lt: return "'<'";
        case TokenKind::Gt: return "'>'";
        case TokenKind::Minus: return "'-'";
        case TokenKind::Assign: return "'='";
        case TokenKind::KwEnum: return "'enum'";
        case TokenKind::KwDefine: return "'define'";
        case TokenKind::KwUsing: return "'using'";
        case TokenKind::KwPublic: return "'public'";
        case TokenKind::KwOptional: return "'optional'";
        case TokenKind::KwList: return "'list'";
        case TokenKind::KwTrue: return "'true'";
        case TokenKind::KwFalse: return "'false'";
        }
        return "token";
    }

    Lexer::Lexer(const String& source, const String& file, std::vector<Diagnostic>& diagnostics)
        : source_(source), file_(file), diagnostics_(diagnostics)
    {
        if (source_.size() >= 3 && static_cast<UInt8>(source_[0]) == 0xEF && static_cast<UInt8>(source_[1]) == 0xBB && static_cast<UInt8>(source_[2]) == 0xBF)
        {
            index_ = 3;
        }
    }

    char Lexer::peek(UInt32 lookahead) const
    {
        UInt32 position = index_ + lookahead;
        return position < source_.size() ? source_[position] : '\0';
    }

    char Lexer::advance()
    {
        char c = source_[index_++];
        if (c == '\n')
        {
            ++line_;
            column_ = 1;
        }
        else
        {
            ++column_;
        }
        return c;
    }

    bool Lexer::at_end() const
    {
        return index_ >= source_.size();
    }

    StringView Lexer::view(const Token& token) const
    {
        return StringView(source_.data() + token.offset, token.length);
    }

    void Lexer::push(TokenKind kind, UInt32 start, Int32 line, Int32 column)
    {
        Token token;
        token.kind = kind;
        token.offset = start;
        token.length = index_ - start;
        token.line = line;
        token.column = column;
        tokens_.push_back(std::move(token));
    }

    void Lexer::error(Int32 line, Int32 column, const String& message)
    {
        Diagnostic diagnostic;
        diagnostic.severity = 0;
        diagnostic.line = line;
        diagnostic.column = column;
        diagnostic.file = file_;
        diagnostic.message = message;
        diagnostics_.push_back(std::move(diagnostic));
    }

    void Lexer::skip_spaces()
    {
        while (!at_end())
        {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f')
            {
                advance();
                continue;
            }
            if (c == '/' && peek(1) == '/')
            {
                while (!at_end() && peek() != '\n')
                {
                    advance();
                }
                continue;
            }
            break;
        }
    }

    void Lexer::scan_string()
    {
        UInt32 start = index_;
        Int32 line = line_;
        Int32 column = column_;
        advance();
        Token token;
        token.kind = TokenKind::String;
        token.offset = start;
        token.line = line;
        token.column = column;
        bool terminated = false;
        while (!at_end())
        {
            char c = peek();
            if (c == '"')
            {
                advance();
                terminated = true;
                break;
            }
            if (c == '\n')
            {
                break;
            }
            if (c == '\\')
            {
                advance();
                if (at_end())
                {
                    break;
                }
                UInt32 escapeIndex = index_;
                if (decode_escape(source_, index_, token.text) != 0)
                {
                    error(line_, column_, String("unknown escape sequence '\\") + source_[escapeIndex] + "'");
                }
                continue;
            }
            token.text.push_back(c);
            advance();
        }
        if (!terminated)
        {
            error(line, column, "unterminated string literal");
        }
        token.length = index_ - start;
        tokens_.push_back(std::move(token));
    }

    void Lexer::scan_number()
    {
        UInt32 start = index_;
        Int32 line = line_;
        Int32 column = column_;
        bool isFloat = false;
        while (is_digit(peek()))
        {
            advance();
        }
        if (peek() == '.' && is_digit(peek(1)))
        {
            isFloat = true;
            advance();
            while (is_digit(peek()))
            {
                advance();
            }
        }
        if (peek() == 'e' || peek() == 'E')
        {
            UInt32 save = index_;
            advance();
            if (peek() == '+' || peek() == '-')
            {
                advance();
            }
            if (is_digit(peek()))
            {
                isFloat = true;
                while (is_digit(peek()))
                {
                    advance();
                }
            }
            else
            {
                index_ = save;
            }
        }
        String digits(source_.substr(start, index_ - start));
        Token token;
        token.offset = start;
        token.length = index_ - start;
        token.line = line;
        token.column = column;
        if (isFloat)
        {
            token.kind = TokenKind::Float;
            token.floatValue = std::strtod(digits.c_str(), nullptr);
            token.intValue = static_cast<Int64>(token.floatValue);
        }
        else
        {
            token.kind = TokenKind::Int;
            token.intValue = static_cast<Int64>(std::strtoull(digits.c_str(), nullptr, 10));
            token.floatValue = static_cast<double>(token.intValue);
        }
        tokens_.push_back(std::move(token));
    }

    void Lexer::scan_identifier()
    {
        UInt32 start = index_;
        Int32 line = line_;
        Int32 column = column_;
        while (is_identifier_char(peek()))
        {
            advance();
        }
        String text = source_.substr(start, index_ - start);
        TokenKind kind = TokenKind::Identifier;
        for (const KeywordEntry& entry : keywordTable)
        {
            if (text == entry.text)
            {
                kind = entry.kind;
                break;
            }
        }
        Token token;
        token.kind = kind;
        token.offset = start;
        token.length = index_ - start;
        token.line = line;
        token.column = column;
        token.text = std::move(text);
        tokens_.push_back(std::move(token));
    }

    void Lexer::scan_version()
    {
        UInt32 start = index_;
        Int32 line = line_;
        Int32 column = column_;
        advance();
        while (is_digit(peek()))
        {
            advance();
        }
        if (peek() != '.' || !is_digit(peek(1)))
        {
            error(line, column, "expected version literal of the form vX.Y");
        }
        else
        {
            advance();
            while (is_digit(peek()))
            {
                advance();
            }
        }
        Token token;
        token.kind = TokenKind::Version;
        token.offset = start;
        token.length = index_ - start;
        token.line = line;
        token.column = column;
        token.text = source_.substr(start, index_ - start);
        tokens_.push_back(std::move(token));
    }

    std::vector<Token> Lexer::run()
    {
        while (!at_end())
        {
            skip_spaces();
            if (at_end())
            {
                break;
            }
            UInt32 start = index_;
            Int32 line = line_;
            Int32 column = column_;
            char c = peek();
            if (c == '\n')
            {
                advance();
                push(TokenKind::Newline, start, line, column);
                continue;
            }
            if (c == 'v' && is_digit(peek(1)))
            {
                scan_version();
                continue;
            }
            if (is_identifier_start(c))
            {
                scan_identifier();
                continue;
            }
            if (is_digit(c))
            {
                scan_number();
                continue;
            }
            if (c == '"')
            {
                scan_string();
                continue;
            }
            advance();
            switch (c)
            {
            case '@': push(TokenKind::At, start, line, column); break;
            case ':': push(TokenKind::Colon, start, line, column); break;
            case ';': push(TokenKind::Semicolon, start, line, column); break;
            case ',': push(TokenKind::Comma, start, line, column); break;
            case '.':
                if (peek() == '.')
                {
                    advance();
                    push(TokenKind::DotDot, start, line, column);
                }
                else
                {
                    push(TokenKind::Dot, start, line, column);
                }
                break;
            case '{': push(TokenKind::LBrace, start, line, column); break;
            case '}': push(TokenKind::RBrace, start, line, column); break;
            case '[': push(TokenKind::LBracket, start, line, column); break;
            case ']': push(TokenKind::RBracket, start, line, column); break;
            case '<': push(TokenKind::Lt, start, line, column); break;
            case '>': push(TokenKind::Gt, start, line, column); break;
            case '-': push(TokenKind::Minus, start, line, column); break;
            case '=': push(TokenKind::Assign, start, line, column); break;
            default:
                error(line, column, String("unexpected character '") + c + "'");
                break;
            }
        }
        Token end;
        end.kind = TokenKind::End;
        end.offset = static_cast<UInt32>(source_.size());
        end.line = line_;
        end.column = column_;
        tokens_.push_back(std::move(end));
        return std::move(tokens_);
    }
}
