#include "template_expr.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace izmlgenc
{
    namespace
    {
        struct Token
        {
            enum class Kind
            {
                End,
                Identifier,
                Int,
                Double,
                String,
                Symbol
            };

            Kind kind = Kind::End;
            String text;
            Int64 integer = 0;
            double real = 0.0;
        };

        class Lexer
        {
        public:
            explicit Lexer(const String& text) : text_(text)
            {
            }

            std::vector<Token> run(String& error)
            {
                std::vector<Token> tokens;
                while (position_ < text_.size())
                {
                    const char c = text_[position_];
                    if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                    {
                        ++position_;
                        continue;
                    }
                    if (std::isdigit(static_cast<unsigned char>(c)))
                    {
                        scan_number(tokens);
                        continue;
                    }
                    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
                    {
                        scan_identifier(tokens);
                        continue;
                    }
                    if (c == '"' || c == '\'')
                    {
                        if (!scan_string(tokens, c, error))
                        {
                            return tokens;
                        }
                        continue;
                    }
                    scan_symbol(tokens);
                }
                Token end;
                tokens.push_back(end);
                return tokens;
            }

        private:
            void scan_number(std::vector<Token>& tokens)
            {
                const std::size_t start = position_;
                while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_])))
                {
                    ++position_;
                }
                bool is_double = false;
                if (position_ < text_.size() && text_[position_] == '.')
                {
                    is_double = true;
                    ++position_;
                    while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_])))
                    {
                        ++position_;
                    }
                }
                const String body = text_.substr(start, position_ - start);
                Token token;
                if (is_double)
                {
                    token.kind = Token::Kind::Double;
                    token.real = std::strtod(body.c_str(), nullptr);
                }
                else
                {
                    token.kind = Token::Kind::Int;
                    token.integer = static_cast<Int64>(std::strtoll(body.c_str(), nullptr, 10));
                }
                tokens.push_back(token);
            }

            void scan_identifier(std::vector<Token>& tokens)
            {
                const std::size_t start = position_;
                while (position_ < text_.size() &&
                    (std::isalnum(static_cast<unsigned char>(text_[position_])) || text_[position_] == '_'))
                {
                    ++position_;
                }
                Token token;
                token.kind = Token::Kind::Identifier;
                token.text = text_.substr(start, position_ - start);
                tokens.push_back(token);
            }

            bool scan_string(std::vector<Token>& tokens, char quote, String& error)
            {
                ++position_;
                Token token;
                token.kind = Token::Kind::String;
                while (position_ < text_.size())
                {
                    const char c = text_[position_++];
                    if (c == quote)
                    {
                        tokens.push_back(token);
                        return true;
                    }
                    if (c == '\\')
                    {
                        if (position_ >= text_.size())
                        {
                            break;
                        }
                        const char next = text_[position_++];
                        switch (next)
                        {
                        case 'n': token.text.push_back('\n'); break;
                        case 't': token.text.push_back('\t'); break;
                        case 'r': token.text.push_back('\r'); break;
                        case '\\': token.text.push_back('\\'); break;
                        case '"': token.text.push_back('"'); break;
                        case '\'': token.text.push_back('\''); break;
                        case 'x':
                        {
                            int value = 0;
                            int digits = 0;
                            while (digits < 2 && position_ < text_.size() &&
                                std::isxdigit(static_cast<unsigned char>(text_[position_])))
                            {
                                const char hex = text_[position_++];
                                value = value * 16 + (hex <= '9' ? hex - '0' : (hex >= 'a' ? hex - 'a' + 10 : hex - 'A' + 10));
                                ++digits;
                            }
                            token.text.push_back(static_cast<char>(value));
                            break;
                        }
                        default: token.text.push_back(next); break;
                        }
                        continue;
                    }
                    token.text.push_back(c);
                }
                error = "unterminated string in expression";
                return false;
            }

            void scan_symbol(std::vector<Token>& tokens)
            {
                const char c = text_[position_++];
                Token token;
                token.kind = Token::Kind::Symbol;
                if (c == '=' && position_ < text_.size() && text_[position_] == '=')
                {
                    ++position_;
                    token.text = "==";
                }
                else if (c == '!' && position_ < text_.size() && text_[position_] == '=')
                {
                    ++position_;
                    token.text = "!=";
                }
                else if (c == '<' && position_ < text_.size() && text_[position_] == '=')
                {
                    ++position_;
                    token.text = "<=";
                }
                else if (c == '>' && position_ < text_.size() && text_[position_] == '=')
                {
                    ++position_;
                    token.text = ">=";
                }
                else
                {
                    token.text = String(1, c);
                }
                tokens.push_back(token);
            }

            const String& text_;
            std::size_t position_ = 0;
        };

        class Parser
        {
        public:
            Parser(const std::vector<Token>& tokens, String& error) : tokens_(tokens), error_(error)
            {
            }

            ExprPtr run()
            {
                ExprPtr expr = parse_or();
                if (!error_.empty())
                {
                    return nullptr;
                }
                if (peek().kind != Token::Kind::End)
                {
                    error_ = "unexpected token '" + peek().text + "' in expression";
                    return nullptr;
                }
                return expr;
            }

        private:
            const Token& peek(std::size_t lookahead = 0) const
            {
                const std::size_t index = cursor_ + lookahead;
                return index < tokens_.size() ? tokens_[index] : tokens_.back();
            }

            bool match(const String& text)
            {
                if (peek().kind == Token::Kind::Symbol && peek().text == text)
                {
                    ++cursor_;
                    return true;
                }
                return false;
            }

            bool match_word(const String& text)
            {
                if (peek().kind == Token::Kind::Identifier && peek().text == text)
                {
                    ++cursor_;
                    return true;
                }
                return false;
            }

            ExprPtr make(Expr::Kind kind)
            {
                auto expr = std::make_shared<Expr>();
                expr->kind = kind;
                return expr;
            }

            ExprPtr parse_or()
            {
                ExprPtr left = parse_and();
                while (match_word("or") || match("||"))
                {
                    ExprPtr expr = make(Expr::Kind::Binary);
                    expr->op = "or";
                    expr->left = left;
                    expr->right = parse_and();
                    left = expr;
                }
                return left;
            }

            ExprPtr parse_and()
            {
                ExprPtr left = parse_equality();
                while (match_word("and") || match("&&"))
                {
                    ExprPtr expr = make(Expr::Kind::Binary);
                    expr->op = "and";
                    expr->left = left;
                    expr->right = parse_equality();
                    left = expr;
                }
                return left;
            }

            ExprPtr parse_equality()
            {
                ExprPtr left = parse_comparison();
                while (match("==") || match("!="))
                {
                    ExprPtr expr = make(Expr::Kind::Binary);
                    expr->op = tokens_[cursor_ - 1].text;
                    expr->left = left;
                    expr->right = parse_comparison();
                    left = expr;
                }
                return left;
            }

            ExprPtr parse_comparison()
            {
                ExprPtr left = parse_additive();
                while (match("<") || match(">") || match("<=") || match(">="))
                {
                    ExprPtr expr = make(Expr::Kind::Binary);
                    expr->op = tokens_[cursor_ - 1].text;
                    expr->left = left;
                    expr->right = parse_additive();
                    left = expr;
                }
                return left;
            }

            ExprPtr parse_additive()
            {
                ExprPtr left = parse_multiplicative();
                while (match("+") || match("-"))
                {
                    ExprPtr expr = make(Expr::Kind::Binary);
                    expr->op = tokens_[cursor_ - 1].text;
                    expr->left = left;
                    expr->right = parse_multiplicative();
                    left = expr;
                }
                return left;
            }

            ExprPtr parse_multiplicative()
            {
                ExprPtr left = parse_unary();
                while (match("*") || match("/") || match("%"))
                {
                    ExprPtr expr = make(Expr::Kind::Binary);
                    expr->op = tokens_[cursor_ - 1].text;
                    expr->left = left;
                    expr->right = parse_unary();
                    left = expr;
                }
                return left;
            }

            ExprPtr parse_unary()
            {
                if (match_word("not") || match("!"))
                {
                    ExprPtr expr = make(Expr::Kind::Unary);
                    expr->op = "not";
                    expr->right = parse_unary();
                    return expr;
                }
                if (match("-"))
                {
                    ExprPtr expr = make(Expr::Kind::Unary);
                    expr->op = "-";
                    expr->right = parse_unary();
                    return expr;
                }
                return parse_postfix();
            }

            ExprPtr parse_postfix()
            {
                ExprPtr expr = parse_primary();
                while (!error_.empty() == false)
                {
                    if (match("."))
                    {
                        if (peek().kind != Token::Kind::Identifier)
                        {
                            error_ = "expected member name after '.'";
                            return expr;
                        }
                        ExprPtr member = make(Expr::Kind::Member);
                        member->left = expr;
                        member->text = peek().text;
                        ++cursor_;
                        expr = member;
                        continue;
                    }
                    if (match("["))
                    {
                        ExprPtr index = make(Expr::Kind::Index);
                        index->left = expr;
                        index->right = parse_or();
                        if (!match("]"))
                        {
                            error_ = "expected ']'";
                            return expr;
                        }
                        expr = index;
                        continue;
                    }
                    if (match("("))
                    {
                        if (expr->kind != Expr::Kind::Variable)
                        {
                            error_ = "only named functions can be called";
                            return expr;
                        }
                        ExprPtr call = make(Expr::Kind::Call);
                        call->name = expr->name;
                        if (!match(")"))
                        {
                            do
                            {
                                call->args.push_back(parse_or());
                                if (error_.empty() == false) { return expr; }
                            }
                            while (match(","));
                            if (!match(")"))
                            {
                                error_ = "expected ')'";
                                return expr;
                            }
                        }
                        expr = call;
                        continue;
                    }
                    break;
                }
                return expr;
            }

            ExprPtr parse_primary()
            {
                if (peek().kind == Token::Kind::Int || peek().kind == Token::Kind::Double ||
                    peek().kind == Token::Kind::String)
                {
                    ExprPtr expr = make(Expr::Kind::Literal);
                    const Token& token = tokens_[cursor_++];
                    if (token.kind == Token::Kind::Int) { expr->literal = Value::int_of(token.integer); }
                    else if (token.kind == Token::Kind::Double) { expr->literal = Value::double_of(token.real); }
                    else { expr->literal = Value::string_of(token.text); }
                    return expr;
                }
                if (peek().kind == Token::Kind::Identifier)
                {
                    const String word = tokens_[cursor_++].text;
                    if (word == "true") { ExprPtr expr = make(Expr::Kind::Literal); expr->literal = Value::boolean_of(true); return expr; }
                    if (word == "false") { ExprPtr expr = make(Expr::Kind::Literal); expr->literal = Value::boolean_of(false); return expr; }
                    if (word == "null") { ExprPtr expr = make(Expr::Kind::Literal); return expr; }
                    ExprPtr expr = make(Expr::Kind::Variable);
                    expr->name = word;
                    return expr;
                }
                if (match("("))
                {
                    ExprPtr expr = parse_or();
                    if (!match(")"))
                    {
                        error_ = "expected ')'";
                    }
                    return expr;
                }
                error_ = "expected expression";
                return make(Expr::Kind::Literal);
            }

            const std::vector<Token>& tokens_;
            String& error_;
            std::size_t cursor_ = 0;
        };

        bool config_flag(const Value* config, const String& key, bool fallback)
        {
            const Value* value = config != nullptr ? config->find(key) : nullptr;
            if (value == nullptr)
            {
                return fallback;
            }
            if (value->kind == ValueKind::Bool) { return value->boolean; }
            if (value->kind == ValueKind::Int) { return value->integer != 0; }
            return value->text != "0" && value->text != "false" && !value->text.empty();
        }

        String config_text(const Value* config, const String& key, const String& fallback)
        {
            const Value* value = config != nullptr ? config->find(key) : nullptr;
            return value != nullptr ? value->as_text() : fallback;
        }

        String map_lookup(const Value* map, const String& key)
        {
            if (map == nullptr)
            {
                return "";
            }
            const Value* value = map->find(key);
            return value != nullptr ? value->as_text() : "";
        }
    }

    void Scope::set(const String& name, Value value)
    {
        vars_[name] = std::move(value);
    }

    const Value* Scope::find(const String& name) const
    {
        auto found = vars_.find(name);
        if (found != vars_.end())
        {
            return &found->second;
        }
        return parent_ != nullptr ? parent_->find(name) : nullptr;
    }

    bool parse_expression(const String& text, ExprPtr& out, String& error)
    {
        Lexer lexer(text);
        std::vector<Token> tokens = lexer.run(error);
        if (!error.empty())
        {
            return false;
        }
        Parser parser(tokens, error);
        out = parser.run();
        return out != nullptr && error.empty();
    }

    String value_to_string(const Value& value)
    {
        return value.as_text();
    }

    namespace
    {
        bool is_number(const Value& value)
        {
            return value.kind == ValueKind::Int || value.kind == ValueKind::Double;
        }

        double number_of(const Value& value)
        {
            return value.kind == ValueKind::Double ? value.real : static_cast<double>(value.integer);
        }

        String c_escape(const String& text, bool non_ascii)
        {
            String result;
            for (unsigned char c : text)
            {
                switch (c)
                {
                case '\\': result += "\\\\"; break;
                case '"': result += "\\\""; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                case '\0': result += "\\0"; break;
                default:
                    if (non_ascii && c >= 0x7F)
                    {
                        char buffer[8];
                        std::snprintf(buffer, sizeof(buffer), "\\x%02X", c);
                        result += buffer;
                    }
                    else
                    {
                        result.push_back(static_cast<char>(c));
                    }
                    break;
                }
            }
            return result;
        }

        String identifier_of(const String& text)
        {
            String result;
            for (char c : text)
            {
                result.push_back(std::isalnum(static_cast<unsigned char>(c)) || c == '_' ? c : '_');
            }
            if (result.empty() || std::isdigit(static_cast<unsigned char>(result[0])))
            {
                result.insert(result.begin(), '_');
            }
            return result;
        }
    }

    Value eval_expression(const Expr& expr, const Scope& scope, const EvalEnv& env, String& error)
    {
        if (expr.kind == Expr::Kind::Literal)
        {
            return expr.literal;
        }
        if (expr.kind == Expr::Kind::Variable)
        {
            const Value* value = scope.find(expr.name);
            if (value == nullptr)
            {
                error = "unknown variable '" + expr.name + "'";
                return Value();
            }
            return *value;
        }
        if (expr.kind == Expr::Kind::Member)
        {
            Value base = eval_expression(*expr.left, scope, env, error);
            if (error.empty() == false) { return Value(); }
            const Value* value = base.find(expr.text);
            return value != nullptr ? *value : Value();
        }
        if (expr.kind == Expr::Kind::Index)
        {
            Value base = eval_expression(*expr.left, scope, env, error);
            Value key = eval_expression(*expr.right, scope, env, error);
            if (error.empty() == false) { return Value(); }
            if (base.kind == ValueKind::List)
            {
                const Int64 index = key.kind == ValueKind::Int ? key.integer : std::strtoll(key.as_text().c_str(), nullptr, 10);
                if (index < 0 || static_cast<std::size_t>(index) >= base.items.size())
                {
                    return Value();
                }
                return base.items[static_cast<std::size_t>(index)];
            }
            if (base.kind == ValueKind::Object)
            {
                const Value* value = base.find(key.as_text());
                return value != nullptr ? *value : Value();
            }
            return Value();
        }
        if (expr.kind == Expr::Kind::Unary)
        {
            Value operand = eval_expression(*expr.right, scope, env, error);
            if (error.empty() == false) { return Value(); }
            if (expr.op == "not")
            {
                return Value::boolean_of(!operand.truthy());
            }
            if (operand.kind == ValueKind::Double) { return Value::double_of(-operand.real); }
            return Value::int_of(-operand.integer);
        }
        if (expr.kind == Expr::Kind::Binary)
        {
            if (expr.op == "and")
            {
                Value left = eval_expression(*expr.left, scope, env, error);
                if (error.empty() == false) { return Value(); }
                if (!left.truthy()) { return Value::boolean_of(false); }
                return Value::boolean_of(eval_expression(*expr.right, scope, env, error).truthy());
            }
            if (expr.op == "or")
            {
                Value left = eval_expression(*expr.left, scope, env, error);
                if (error.empty() == false) { return Value(); }
                if (left.truthy()) { return Value::boolean_of(true); }
                return Value::boolean_of(eval_expression(*expr.right, scope, env, error).truthy());
            }
            Value left = eval_expression(*expr.left, scope, env, error);
            Value right = eval_expression(*expr.right, scope, env, error);
            if (error.empty() == false) { return Value(); }

            if (expr.op == "==" || expr.op == "!=")
            {
                bool equal = false;
                if (is_number(left) && is_number(right)) { equal = number_of(left) == number_of(right); }
                else if (left.kind == ValueKind::Bool && right.kind == ValueKind::Bool) { equal = left.boolean == right.boolean; }
                else { equal = left.as_text() == right.as_text(); }
                return Value::boolean_of(expr.op == "==" ? equal : !equal);
            }
            if (expr.op == "<" || expr.op == ">" || expr.op == "<=" || expr.op == ">=")
            {
                const bool numeric = is_number(left) && is_number(right);
                const bool less = numeric ? number_of(left) < number_of(right) : left.as_text() < right.as_text();
                const bool greater = numeric ? number_of(left) > number_of(right) : left.as_text() > right.as_text();
                const bool equal = numeric ? number_of(left) == number_of(right) : left.as_text() == right.as_text();
                if (expr.op == "<") { return Value::boolean_of(less); }
                if (expr.op == ">") { return Value::boolean_of(greater); }
                if (expr.op == "<=") { return Value::boolean_of(less || equal); }
                return Value::boolean_of(greater || equal);
            }
            if (expr.op == "+" && (left.kind == ValueKind::String || right.kind == ValueKind::String))
            {
                return Value::string_of(left.as_text() + right.as_text());
            }
            if (expr.op == "+" || expr.op == "-" || expr.op == "*" || expr.op == "/" || expr.op == "%")
            {
                if (left.kind == ValueKind::Double || right.kind == ValueKind::Double)
                {
                    const double a = number_of(left);
                    const double b = number_of(right);
                    if (expr.op == "+") { return Value::double_of(a + b); }
                    if (expr.op == "-") { return Value::double_of(a - b); }
                    if (expr.op == "*") { return Value::double_of(a * b); }
                    if (expr.op == "/") { return Value::double_of(b == 0.0 ? 0.0 : a / b); }
                    return Value::int_of(static_cast<Int64>(b == 0.0 ? 0 : std::fmod(a, b)));
                }
                const Int64 a = left.integer;
                const Int64 b = right.integer;
                if (expr.op == "+") { return Value::int_of(a + b); }
                if (expr.op == "-") { return Value::int_of(a - b); }
                if (expr.op == "*") { return Value::int_of(a * b); }
                if (expr.op == "/") { return Value::int_of(b == 0 ? 0 : a / b); }
                return Value::int_of(b == 0 ? 0 : a % b);
            }
            error = "unsupported binary operator '" + expr.op + "'";
            return Value();
        }
        if (expr.kind == Expr::Kind::Call)
        {
            std::vector<Value> args;
            for (const ExprPtr& arg : expr.args)
            {
                args.push_back(eval_expression(*arg, scope, env, error));
                if (error.empty() == false) { return Value(); }
            }
            const String& name = expr.name;
            if (name == "type" && args.size() == 1) { return Value::string_of(map_lookup(env.types, args[0].as_text())); }
            if (name == "reader" && args.size() == 1) { return Value::string_of(map_lookup(env.readers, args[0].as_text())); }
            if (name == "ident" && args.size() == 1) { return Value::string_of(identifier_of(args[0].as_text())); }
            if (name == "qual" && args.size() == 1)
            {
                const String separator = config_text(env.config, "ident_sep", "::");
                String text = args[0].as_text();
                String result;
                for (char c : text)
                {
                    result += c == '.' ? separator : String(1, c);
                }
                return Value::string_of(result);
            }
            if (name == "kw" && args.size() == 1)
            {
                const String text = args[0].as_text();
                const Value* keyword = env.keywords != nullptr ? env.keywords->find(text) : nullptr;
                return Value::string_of(keyword != nullptr ? config_text(env.config, "keyword_prefix", "@") + text : text);
            }
            if (name == "escape" && args.size() == 1)
            {
                return Value::string_of(c_escape(args[0].as_text(), config_flag(env.config, "escape_non_ascii", true)));
            }
            if (name == "pad" && args.size() == 1) { return Value::string_of(String(static_cast<std::size_t>(args[0].integer < 0 ? 0 : args[0].integer), ' ')); }
            if (name == "len" && args.size() == 1)
            {
                const Value& value = args[0];
                if (value.kind == ValueKind::String) { return Value::int_of(static_cast<Int64>(value.text.size())); }
                if (value.kind == ValueKind::List) { return Value::int_of(static_cast<Int64>(value.items.size())); }
                if (value.kind == ValueKind::Object) { return Value::int_of(static_cast<Int64>(value.object.size())); }
                return Value::int_of(0);
            }
            if (name == "default" && args.size() == 2) { return args[0].truthy() ? args[0] : args[1]; }
            if (name == "str" && args.size() == 1) { return Value::string_of(args[0].as_text()); }
            if (name == "has" && args.size() == 2)
            {
                return Value::boolean_of(args[0].find(args[1].as_text()) != nullptr);
            }
            if (name == "join" && args.size() == 2)
            {
                String result;
                if (args[0].kind == ValueKind::List)
                {
                    for (const Value& item : args[0].items)
                    {
                        if (!result.empty()) { result += args[1].as_text(); }
                        result += item.as_text();
                    }
                }
                return Value::string_of(result);
            }
            if (name == "range")
            {
                const Int64 begin = args.size() >= 1 ? args[0].integer : 0;
                const Int64 end = args.size() >= 2 ? args[1].integer : begin;
                std::vector<Value> items;
                for (Int64 i = (args.size() >= 2 ? begin : 0); i < end; ++i)
                {
                    items.push_back(Value::int_of(i));
                }
                return Value::list_of(std::move(items));
            }
            error = "unknown function '" + name + "'";
            return Value();
        }
        return Value();
    }
}
