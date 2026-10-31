#ifndef IZMLGENC_TEMPLATE_EXPR_HPP
#define IZMLGENC_TEMPLATE_EXPR_HPP

#include "template_value.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace izmlgenc
{
    struct Expr;
    using ExprPtr = std::shared_ptr<Expr>;

    struct Expr
    {
        enum class Kind
        {
            Literal,
            Variable,
            Member,
            Index,
            Call,
            Unary,
            Binary
        };

        Kind kind = Kind::Literal;
        Value literal;
        String name;
        String text;
        String op;
        ExprPtr left;
        ExprPtr right;
        std::vector<ExprPtr> args;
    };

    class Scope
    {
    public:
        explicit Scope(const Scope* parent = nullptr) : parent_(parent)
        {
        }

        void set(const String& name, Value value);
        const Value* find(const String& name) const;

    private:
        const Scope* parent_ = nullptr;
        std::map<String, Value> vars_;
    };

    struct EvalEnv
    {
        const Value* types = nullptr;
        const Value* readers = nullptr;
        const Value* keywords = nullptr;
        const Value* config = nullptr;
    };

    bool parse_expression(const String& text, ExprPtr& out, String& error);
    Value eval_expression(const Expr& expr, const Scope& scope, const EvalEnv& env, String& error);
    String value_to_string(const Value& value);
}

#endif
