#ifndef IZMLGENC_TEMPLATE_GENERATOR_HPP
#define IZMLGENC_TEMPLATE_GENERATOR_HPP

#include "izml_ir.hpp"
#include "lang_config.hpp"

#include <string>
#include <vector>

namespace izmlgenc
{
    class TemplateGenerator
    {
    public:
        TemplateGenerator(const izmlr::Program& program, const LangConfig& config, Options options);

        bool run();

        const std::vector<String>& errors() const
        {
            return errors_;
        }

    private:
        void error(const String& message);

        const izmlr::Program& program_;
        const LangConfig& config_;
        Options options_;
        std::vector<String> errors_;
    };
}

#endif
