#ifndef IZMLGENC_LANG_CONFIG_HPP
#define IZMLGENC_LANG_CONFIG_HPP

#include <map>
#include <string>
#include <vector>

namespace izmlgenc
{
    using String = std::string;

    struct LangOutput
    {
        String name;
        String file;
        String body;
        bool runtime = false;
    };

    struct LangConfig
    {
        String lang;
        String version;
        String path;
        std::map<String, String> config;
        std::map<String, String> types;
        std::map<String, String> readers;
        std::map<String, String> keywords;
        String macros;
        std::vector<LangOutput> outputs;
    };

    bool load_lang_configs(const String& directory, std::map<String, LangConfig>& configs, String& error);

    struct Options
    {
        String input;
        String output_dir;
        bool bundle_runtime = false;
        String runtime_dir;
    };
}

#endif
