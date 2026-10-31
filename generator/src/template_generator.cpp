#include "template_generator.hpp"

#include "izml_io.hpp"
#include "template_expr.hpp"
#include "template_render.hpp"
#include "template_value.hpp"

#include <utility>

namespace izmlgenc
{
    namespace
    {
        Value make_map(const std::map<String, String>& source)
        {
            Value result = Value::object_of();
            for (const auto& pair : source)
            {
                result.set(pair.first, Value::string_of(pair.second));
            }
            return result;
        }
    }

    TemplateGenerator::TemplateGenerator(const izmlr::Program& program, const LangConfig& config, Options options)
        : program_(program), config_(config), options_(std::move(options))
    {
    }

    void TemplateGenerator::error(const String& message)
    {
        errors_.push_back(message);
    }

    bool TemplateGenerator::run()
    {
        TemplateEngine engine;
        String engine_error;
        if (!engine.add_macros(config_.macros, engine_error))
        {
            error("macros: " + engine_error);
            return false;
        }

        Value program = program_value(program_);
        Value config = make_map(config_.config);
        Value types = make_map(config_.types);
        Value readers = make_map(config_.readers);
        Value keywords = make_map(config_.keywords);

        Scope scope;
        scope.set("program", program);
        scope.set("config", config);
        scope.set("types", types);
        scope.set("readers", readers);
        scope.set("keywords", keywords);

        EvalEnv env;
        env.config = &config;
        env.types = &types;
        env.readers = &readers;
        env.keywords = &keywords;

        bool has_template = false;
        for (const LangOutput& output : config_.outputs)
        {
            if (output.runtime)
            {
                continue;
            }
            has_template = true;
            String text;
            if (!engine.render(output.body, scope, env, text, engine_error))
            {
                error("template '" + output.name + "': " + engine_error);
                return false;
            }
            const String path = options_.output_dir.empty() ? output.file : options_.output_dir + "\\" + output.file;
            if (!izmlio::write_file(path, text))
            {
                error("unable to write '" + path + "'");
                return false;
            }
        }
        if (!has_template)
        {
            error("language '" + config_.lang + "' has no [template:*] section");
            return false;
        }

        if (options_.bundle_runtime)
        {
            for (const LangOutput& output : config_.outputs)
            {
                if (!output.runtime)
                {
                    continue;
                }
                const String path = options_.output_dir.empty() ? output.file : options_.output_dir + "\\" + output.file;
                if (!izmlio::write_file(path, output.body))
                {
                    error("unable to write '" + path + "'");
                    return false;
                }
            }
        }
        return true;
    }
}
