#ifndef IZMLGENC_TEMPLATE_RENDER_HPP
#define IZMLGENC_TEMPLATE_RENDER_HPP

#include "template_expr.hpp"
#include "template_value.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace izmlgenc
{
    struct Node;
    using NodePtr = std::shared_ptr<Node>;

    struct Macro
    {
        String name;
        std::vector<String> args;
        std::vector<NodePtr> body;
    };

    struct Node
    {
        enum class Kind
        {
            Text,
            Print,
            If,
            For,
            Set,
            Macro
        };

        struct Branch
        {
            ExprPtr condition;
            std::vector<NodePtr> body;
        };

        Kind kind = Kind::Text;
        String text;
        ExprPtr expr;
        std::vector<Branch> branches;
        std::vector<NodePtr> else_body;
        String var;
        String name;
        std::vector<String> args;
        std::vector<NodePtr> body;
    };

    class TemplateEngine
    {
    public:
        bool add_macros(const String& text, String& error);
        bool render(const String& text, const Scope& scope, const EvalEnv& env, String& out, String& error);

        const std::map<String, Macro>* macros() const
        {
            return &macros_;
        }

    private:
        friend class TemplateParser;
        bool parse(const String& text, std::vector<NodePtr>& out, String& error);
        void render_nodes(const std::vector<NodePtr>& nodes,
                          Scope& scope,
                          const EvalEnv& env,
                          String& out,
                          String& error,
                          int depth);
        void render_node(const Node& node,
                         Scope& scope,
                         const EvalEnv& env,
                         String& out,
                         String& error,
                         int depth);
        void call_macro(const Macro& macro,
                        const std::vector<Value>& args,
                        const Scope& scope,
                        const EvalEnv& env,
                        String& out,
                        String& error,
                        int depth);

        std::map<String, Macro> macros_;
    };
}

#endif
