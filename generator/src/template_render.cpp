#include "template_render.hpp"

#include <cctype>
#include <set>
#include <utility>

namespace izmlgenc
{
    namespace
    {
        String trim_space(const String& text)
        {
            std::size_t begin = 0;
            std::size_t end = text.size();
            while (begin < end && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r' || text[begin] == '\n'))
            {
                ++begin;
            }
            while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r' || text[end - 1] == '\n'))
            {
                --end;
            }
            return text.substr(begin, end - begin);
        }

        String preprocess(const String& text)
        {
            String result;
            std::size_t start = 0;
            while (start < text.size())
            {
                const std::size_t end = text.find('\n', start);
                const String line = end == String::npos ? text.substr(start) : text.substr(start, end - start + 1);
                const String body = trim_space(line);
                const bool control = body.size() >= 4 && body.rfind("{%", 0) == 0 && body.rfind("%}", body.size() - 2) == body.size() - 2;
                const bool print = body.size() >= 4 && body.rfind("{{", 0) == 0 && body.rfind("}}", body.size() - 2) == body.size() - 2;
                if (control || print)
                {
                    result += body;
                }
                else
                {
                    result += line;
                }
                if (end == String::npos)
                {
                    break;
                }
                start = end + 1;
            }
            return result;
        }

        String first_word(const String& text)
        {
            std::size_t end = 0;
            while (end < text.size() && text[end] != ' ' && text[end] != '\t' && text[end] != '(')
            {
                ++end;
            }
            return text.substr(0, end);
        }

        String rest_after(const String& text, const String& word)
        {
            if (text.size() <= word.size())
            {
                return "";
            }
            return trim_space(text.substr(word.size()));
        }
    }

    class TemplateParser
    {
    public:
        TemplateParser(TemplateEngine& engine, String text, String& error)
            : engine_(engine), text_(std::move(text)), error_(error)
        {
        }

        std::vector<NodePtr> run()
        {
            std::set<String> stops;
            String stop;
            return parse_block(stops, stop);
        }

    private:
        std::vector<NodePtr> parse_block(const std::set<String>& stops, String& stop)
        {
            std::vector<NodePtr> nodes;
            while (position_ < text_.size())
            {
                const std::size_t print = text_.find("{{", position_);
                const std::size_t statement = text_.find("{%", position_);
                const std::size_t comment = text_.find("{#", position_);

                std::size_t next = String::npos;
                for (std::size_t candidate : { print, statement, comment })
                {
                    if (candidate != String::npos && (next == String::npos || candidate < next))
                    {
                        next = candidate;
                    }
                }
                if (next == String::npos)
                {
                    add_text(nodes, text_.substr(position_));
                    position_ = text_.size();
                    break;
                }
                if (next > position_)
                {
                    add_text(nodes, text_.substr(position_, next - position_));
                    position_ = next;
                }
                if (next == print)
                {
                    const std::size_t close = text_.find("}}", position_ + 2);
                    if (close == String::npos)
                    {
                        error_ = "unterminated '{{'";
                        return nodes;
                    }
                    const String body = trim_space(text_.substr(position_ + 2, close - position_ - 2));
                    position_ = close + 2;
                    ExprPtr expr;
                    if (!parse_expression(body, expr, error_))
                    {
                        return nodes;
                    }
                    auto node = std::make_shared<Node>();
                    node->kind = Node::Kind::Print;
                    node->expr = expr;
                    nodes.push_back(node);
                    continue;
                }
                if (next == comment)
                {
                    const std::size_t close = text_.find("#}", position_ + 2);
                    if (close == String::npos)
                    {
                        error_ = "unterminated '{#'";
                        return nodes;
                    }
                    position_ = close + 2;
                    continue;
                }

                const std::size_t close = text_.find("%}", position_ + 2);
                if (close == String::npos)
                {
                    error_ = "unterminated '{%'";
                    return nodes;
                }
                const String tag = trim_space(text_.substr(position_ + 2, close - position_ - 2));
                position_ = close + 2;
                const String word = first_word(tag);

                if (stops.count(word) != 0)
                {
                    stop = tag;
                    return nodes;
                }
                if (word == "if")
                {
                    if (!parse_if(nodes, tag)) { return nodes; }
                    continue;
                }
                if (word == "for")
                {
                    if (!parse_for(nodes, tag)) { return nodes; }
                    continue;
                }
                if (word == "set")
                {
                    if (!parse_set(nodes, tag)) { return nodes; }
                    continue;
                }
                if (word == "macro")
                {
                    if (!parse_macro(tag)) { return nodes; }
                    continue;
                }
                error_ = "unexpected tag '" + word + "'";
                return nodes;
            }
            return nodes;
        }

        void add_text(std::vector<NodePtr>& nodes, const String& text)
        {
            if (text.empty())
            {
                return;
            }
            auto node = std::make_shared<Node>();
            node->kind = Node::Kind::Text;
            node->text = text;
            nodes.push_back(node);
        }

        bool parse_if(std::vector<NodePtr>& nodes, const String& tag)
        {
            String condition_text = rest_after(tag, "if");
            auto node = std::make_shared<Node>();
            node->kind = Node::Kind::If;

            while (true)
            {
                ExprPtr condition;
                if (!parse_expression(condition_text, condition, error_))
                {
                    return false;
                }
                std::set<String> stops = { "elif", "else", "endif" };
                String stop;
                std::vector<NodePtr> body = parse_block(stops, stop);
                if (!error_.empty()) { return false; }
                node->branches.push_back({ condition, body });
                const String word = first_word(stop);
                if (word == "elif")
                {
                    condition_text = rest_after(stop, "elif");
                    continue;
                }
                if (word == "else")
                {
                    std::set<String> end_stops = { "endif" };
                    String end_stop;
                    node->else_body = parse_block(end_stops, end_stop);
                    if (!error_.empty()) { return false; }
                    if (first_word(end_stop) != "endif")
                    {
                        error_ = "expected 'endif'";
                        return false;
                    }
                    break;
                }
                if (word == "endif")
                {
                    break;
                }
                error_ = "expected 'elif', 'else' or 'endif', found '" + stop + "'";
                return false;
            }
            nodes.push_back(node);
            return true;
        }

        bool parse_for(std::vector<NodePtr>& nodes, const String& tag)
        {
            const String body_text = rest_after(tag, "for");
            const std::size_t in = body_text.find(" in ");
            if (in == String::npos)
            {
                error_ = "expected 'for name in list'";
                return false;
            }
            const String variable = trim_space(body_text.substr(0, in));
            ExprPtr list;
            if (!parse_expression(trim_space(body_text.substr(in + 4)), list, error_))
            {
                return false;
            }
            std::set<String> stops = { "endfor" };
            String stop;
            std::vector<NodePtr> body = parse_block(stops, stop);
            if (!error_.empty()) { return false; }
            if (first_word(stop) != "endfor")
            {
                error_ = "expected 'endfor'";
                return false;
            }
            auto node = std::make_shared<Node>();
            node->kind = Node::Kind::For;
            node->var = variable;
            node->expr = list;
            node->body = body;
            nodes.push_back(node);
            return true;
        }

        bool parse_set(std::vector<NodePtr>& nodes, const String& tag)
        {
            const String body_text = rest_after(tag, "set");
            const std::size_t equals = body_text.find('=');
            if (equals == String::npos)
            {
                error_ = "expected 'set name = value'";
                return false;
            }
            auto node = std::make_shared<Node>();
            node->kind = Node::Kind::Set;
            node->name = trim_space(body_text.substr(0, equals));
            if (!parse_expression(trim_space(body_text.substr(equals + 1)), node->expr, error_))
            {
                return false;
            }
            nodes.push_back(node);
            return true;
        }

        bool parse_macro(const String& tag)
        {
            const String body_text = rest_after(tag, "macro");
            const std::size_t open = body_text.find('(');
            if (open == String::npos)
            {
                error_ = "expected 'macro name(args)' in '" + body_text + "'";
                return false;
            }
            const std::size_t close = body_text.rfind(')');
            if (close == String::npos || close < open)
            {
                error_ = "expected ')' in macro declaration '" + body_text + "'";
                return false;
            }
            Macro macro;
            macro.name = trim_space(body_text.substr(0, open));
            const String args = trim_space(body_text.substr(open + 1, close - open - 1));
            if (!args.empty())
            {
                std::size_t start = 0;
                while (start < args.size())
                {
                    const std::size_t comma = args.find(',', start);
                    macro.args.push_back(trim_space(args.substr(start, comma == String::npos ? args.size() - start : comma - start)));
                    if (comma == String::npos) { break; }
                    start = comma + 1;
                }
            }
            std::set<String> stops = { "endmacro" };
            String stop;
            macro.body = parse_block(stops, stop);
            if (!error_.empty())
            {
                error_ = "macro '" + macro.name + "': " + error_;
                return false;
            }
            if (first_word(stop) != "endmacro")
            {
                error_ = "expected 'endmacro'";
                return false;
            }
            engine_.macros_[macro.name] = std::move(macro);
            return true;
        }

        TemplateEngine& engine_;
        const String text_;
        String& error_;
        std::size_t position_ = 0;
    };

    bool TemplateEngine::parse(const String& text, std::vector<NodePtr>& out, String& error)
    {
        TemplateParser parser(*this, preprocess(text), error);
        out = parser.run();
        return error.empty();
    }

    bool TemplateEngine::add_macros(const String& text, String& error)
    {
        std::vector<NodePtr> nodes;
        return parse(text, nodes, error);
    }

    bool TemplateEngine::render(const String& text, const Scope& scope, const EvalEnv& env, String& out, String& error)
    {
        std::vector<NodePtr> nodes;
        if (!parse(text, nodes, error))
        {
            return false;
        }
        Scope local(&scope);
        out.clear();
        render_nodes(nodes, local, env, out, error, 0);
        return error.empty();
    }

    void TemplateEngine::render_nodes(const std::vector<NodePtr>& nodes,
                                      Scope& scope,
                                      const EvalEnv& env,
                                      String& out,
                                      String& error,
                                      int depth)
    {
        for (const NodePtr& node : nodes)
        {
            if (!error.empty())
            {
                return;
            }
            render_node(*node, scope, env, out, error, depth);
        }
    }

    void TemplateEngine::render_node(const Node& node,
                                     Scope& scope,
                                     const EvalEnv& env,
                                     String& out,
                                     String& error,
                                     int depth)
    {
        if (node.kind == Node::Kind::Text)
        {
            out += node.text;
            return;
        }
        if (node.kind == Node::Kind::Set)
        {
            Value value = eval_expression(*node.expr, scope, env, error);
            if (error.empty()) { scope.set(node.name, std::move(value)); }
            return;
        }
        if (node.kind == Node::Kind::Macro)
        {
            return;
        }
        if (node.kind == Node::Kind::Print)
        {
            if (node.expr->kind == Expr::Kind::Call)
            {
                auto found = macros_.find(node.expr->name);
                if (found != macros_.end())
                {
                    std::vector<Value> args;
                    for (const ExprPtr& arg : node.expr->args)
                    {
                        args.push_back(eval_expression(*arg, scope, env, error));
                        if (!error.empty()) { return; }
                    }
                    call_macro(found->second, args, scope, env, out, error, depth + 1);
                    return;
                }
            }
            Value value = eval_expression(*node.expr, scope, env, error);
            if (error.empty()) { out += value.as_text(); }
            return;
        }
        if (node.kind == Node::Kind::If)
        {
            for (const Node::Branch& branch : node.branches)
            {
                Value condition = eval_expression(*branch.condition, scope, env, error);
                if (!error.empty()) { return; }
                if (condition.truthy())
                {
                    render_nodes(branch.body, scope, env, out, error, depth);
                    return;
                }
            }
            render_nodes(node.else_body, scope, env, out, error, depth);
            return;
        }
        if (node.kind == Node::Kind::For)
        {
            Value list = eval_expression(*node.expr, scope, env, error);
            if (!error.empty()) { return; }
            if (list.kind != ValueKind::List)
            {
                error = "'for' expects a list";
                return;
            }
            for (std::size_t index = 0; index < list.items.size(); ++index)
            {
                Scope iteration(&scope);
                iteration.set(node.var, list.items[index]);
                Value loop = Value::object_of();
                loop.set("index", Value::int_of(static_cast<Int64>(index + 1)));
                loop.set("index0", Value::int_of(static_cast<Int64>(index)));
                loop.set("first", Value::boolean_of(index == 0));
                loop.set("last", Value::boolean_of(index + 1 == list.items.size()));
                loop.set("length", Value::int_of(static_cast<Int64>(list.items.size())));
                iteration.set("loop", std::move(loop));
                render_nodes(node.body, iteration, env, out, error, depth);
                if (!error.empty()) { return; }
            }
        }
    }

    void TemplateEngine::call_macro(const Macro& macro,
                                    const std::vector<Value>& args,
                                    const Scope& scope,
                                    const EvalEnv& env,
                                    String& out,
                                    String& error,
                                    int depth)
    {
        if (depth > 64)
        {
            error = "macro recursion is too deep";
            return;
        }
        Scope local(&scope);
        for (std::size_t i = 0; i < macro.args.size(); ++i)
        {
            local.set(macro.args[i], i < args.size() ? args[i] : Value());
        }
        render_nodes(macro.body, local, env, out, error, depth);
    }
}
