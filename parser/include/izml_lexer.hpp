#ifndef IZML_LEXER_HPP
#define IZML_LEXER_HPP

#include "izml_common.hpp"

namespace izml
{
    enum class TokenKind : UInt8
    {
        End,
        Newline,
        Identifier,
        Int,
        Float,
        String,
        Version,
        At,
        Colon,
        Semicolon,
        Comma,
        Dot,
        DotDot,
        LBrace,
        RBrace,
        LBracket,
        RBracket,
        Lt,
        Gt,
        Minus,
        Assign,
        KwEnum,
        KwDefine,
        KwUsing,
        KwPublic,
        KwOptional,
        KwList,
        KwTrue,
        KwFalse
    };

    struct Token
    {
        TokenKind kind = TokenKind::End;
        UInt32 offset = 0;
        UInt32 length = 0;
        Int32 line = 1;
        Int32 column = 1;
        Int64 intValue = 0;
        double floatValue = 0.0;
        String text;
    };

    class Lexer
    {
    public:
        Lexer(const String& source, const String& file, std::vector<Diagnostic>& diagnostics);

        std::vector<Token> run();
        StringView view(const Token& token) const;

    private:
        char peek(UInt32 lookahead = 0) const;
        char advance();
        bool at_end() const;
        void skip_spaces();
        void scan_string();
        void scan_number();
        void scan_identifier();
        void scan_version();
        void push(TokenKind kind, UInt32 start, Int32 line, Int32 column);
        void error(Int32 line, Int32 column, const String& message);

        const String& source_;
        const String& file_;
        std::vector<Diagnostic>& diagnostics_;
        std::vector<Token> tokens_;
        UInt32 index_ = 0;
        Int32 line_ = 1;
        Int32 column_ = 1;
    };

    const char* token_kind_name(TokenKind kind);
}

#endif
