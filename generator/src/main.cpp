#include "izml_io.hpp"
#include "izml_ir_build.hpp"
#include "izml_parser_c_api.h"
#include "lang_config.hpp"
#include "template_generator.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifndef IZML_RUNTIME_DIR
#define IZML_RUNTIME_DIR ""
#endif

namespace
{
    using Int32 = std::int32_t;
    using String = std::string;

    String executable_dir()
    {
        char buffer[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        String path(buffer, buffer + (length == 0 ? 0 : length));
        const std::size_t position = path.find_last_of("\\/");
        return position == String::npos ? String(".") : path.substr(0, position);
    }

    void ensure_directory(const String& path)
    {
        if (!path.empty())
        {
            CreateDirectoryA(path.c_str(), nullptr);
        }
    }

    String file_name(const String& path)
    {
        const std::size_t position = path.find_last_of("\\/");
        return position == String::npos ? path : path.substr(position + 1);
    }

    String locate_asset(const String& explicit_dir, const String& name)
    {
        std::vector<String> roots;
        if (!explicit_dir.empty())
        {
            roots.push_back(explicit_dir);
            roots.push_back(explicit_dir + "\\izml-runtime");
            roots.push_back(explicit_dir + "\\csharp");
            roots.push_back(explicit_dir + "\\izml-runtime\\csharp");
        }
        if (IZML_RUNTIME_DIR[0] != '\0')
        {
            roots.push_back(IZML_RUNTIME_DIR);
            roots.push_back(String(IZML_RUNTIME_DIR) + "\\csharp");
        }
        const String base = executable_dir();
        roots.push_back(base);
        roots.push_back(base + "\\izml-runtime");
        roots.push_back(base + "\\izml-runtime\\csharp");
        roots.push_back(base + "\\..\\izml-runtime");
        roots.push_back(base + "\\..\\izml-runtime\\csharp");
        roots.push_back(base + "\\..");
        for (const String& root : roots)
        {
            const String candidate = root + "\\" + name;
            if (izmlio::file_exists(candidate))
            {
                return candidate;
            }
        }
        return String();
    }

    void usage()
    {
        std::cout <<
            "izmlgenc - IZML code generator\n"
            "\n"
            "usage: izmlgenc <input.izml> --lang <name> [options]\n"
            "\n"
            "options:\n"
            "  -o, --out <dir>         output directory (default: .)\n"
            "      --lang <name>       .izmlgen-lang header key\n"
            "      --lang-dir <dir>    directory with .izmlgen-lang files (default: .)\n"
            "      --bundle-runtime    write [runtime] outputs and copy runtime_binary\n"
            "      --runtime-dir <dir> directory that holds the runtime binary\n"
            "  -h, --help              show this help\n";
    }

    void print_diagnostics(izml_document* document)
    {
        const Int32 count = izml_diagnostic_count(document);
        for (Int32 i = 0; i < count; ++i)
        {
            izml_diagnostic diagnostic = {};
            if (izml_diagnostic_get(document, i, &diagnostic) == 0)
            {
                continue;
            }
            const char* severity = diagnostic.severity == 0 ? "error" : (diagnostic.severity == 1 ? "warning" : "note");
            std::cerr << diagnostic.file << "(" << diagnostic.line << "," << diagnostic.column << "): "
                << severity << ": " << diagnostic.message << "\n";
        }
    }
}

int main(int argc, char** argv)
{
    std::vector<String> args(argv + 1, argv + argc);
    if (args.empty())
    {
        usage();
        return 1;
    }

    izmlgenc::Options options;
    String input;
    String lang;
    String lang_dir;
    for (std::size_t i = 0; i < args.size(); ++i)
    {
        const String& arg = args[i];
        auto value_for = [&](const char* name, String& target) -> bool
        {
            if (i + 1 >= args.size())
            {
                std::cerr << "izmlgenc: missing value for " << name << "\n";
                return false;
            }
            target = args[++i];
            return true;
        };

        if (arg == "-h" || arg == "--help")
        {
            usage();
            return 0;
        }
        if (arg == "-o" || arg == "--out")
        {
            if (!value_for("--out", options.output_dir)) { return 2; }
            continue;
        }
        if (arg == "--lang")
        {
            if (!value_for("--lang", lang)) { return 2; }
            continue;
        }
        if (arg == "--lang-dir")
        {
            if (!value_for("--lang-dir", lang_dir)) { return 2; }
            continue;
        }
        if (arg == "--runtime-dir")
        {
            if (!value_for("--runtime-dir", options.runtime_dir)) { return 2; }
            continue;
        }
        if (arg == "--bundle-runtime")
        {
            options.bundle_runtime = true;
            continue;
        }
        if (!arg.empty() && arg[0] == '-')
        {
            std::cerr << "izmlgenc: unknown option '" << arg << "'\n";
            return 2;
        }
        if (!input.empty())
        {
            std::cerr << "izmlgenc: unexpected argument '" << arg << "'\n";
            return 2;
        }
        input = arg;
    }

    if (input.empty())
    {
        std::cerr << "izmlgenc: no input file was given\n";
        usage();
        return 2;
    }
    if (lang.empty())
    {
        std::cerr << "izmlgenc: --lang is required\n";
        return 2;
    }

    std::map<String, izmlgenc::LangConfig> configs;
    String error;
    const String primary_dir = lang_dir.empty() ? String(".") : lang_dir;
    bool loaded = izmlgenc::load_lang_configs(primary_dir, configs, error);
    if (!loaded)
    {
        std::cerr << "izmlgenc: " << error << "\n";
        return 1;
    }
    if (configs.empty() && lang_dir.empty())
    {
        const String beside_exe = executable_dir();
        if (beside_exe != ".")
        {
            error.clear();
            loaded = izmlgenc::load_lang_configs(beside_exe, configs, error);
            if (!loaded)
            {
                std::cerr << "izmlgenc: " << error << "\n";
                return 1;
            }
        }
    }
    if (configs.empty())
    {
        std::cerr << "izmlgenc: no .izmlgen-lang config found in ./\n";
        return 1;
    }

    auto selected = configs.find(lang);
    if (selected == configs.end())
    {
        std::cerr << "izmlgenc: language config '" << lang << "' was not found; available:";
        for (const auto& entry : configs)
        {
            std::cerr << " " << entry.first;
        }
        std::cerr << "\n";
        return 1;
    }
    const izmlgenc::LangConfig& config = selected->second;

    izml_document* document = izml_parse_file(input.c_str());
    if (document == nullptr)
    {
        std::cerr << "izmlgenc: unable to open '" << input << "'\n";
        return 1;
    }
    if (izml_document_ok(document) == 0)
    {
        print_diagnostics(document);
        std::cerr << "izmlgenc: '" << input << "' did not pass analysis\n";
        izml_document_destroy(document);
        return 1;
    }

    izmlr::Program program;
    if (!izmlr::build(document, program))
    {
        std::cerr << "izmlgenc: unable to build IZLR for '" << input << "'\n";
        izml_document_destroy(document);
        return 1;
    }
    izml_document_destroy(document);

    options.input = input;
    ensure_directory(options.output_dir);

    izmlgenc::TemplateGenerator generator(program, config, options);
    const bool ok = generator.run();
    for (const String& message : generator.errors())
    {
        std::cerr << "izmlgenc: " << message << "\n";
    }
    if (!ok)
    {
        return 1;
    }

    if (options.bundle_runtime)
    {
        auto binary = config.config.find("runtime_binary");
        if (binary != config.config.end() && !binary->second.empty())
        {
            const String source = locate_asset(options.runtime_dir, binary->second);
            if (source.empty())
            {
                std::cerr << "izmlgenc: '" << binary->second << "' was not found; pass --runtime-dir\n";
            }
            else
            {
                const String target = (options.output_dir.empty() ? String(".") : options.output_dir) + "\\" + file_name(source);
                if (!CopyFileA(source.c_str(), target.c_str(), FALSE))
                {
                    std::cerr << "izmlgenc: unable to copy '" << source << "' to '" << target << "'\n";
                }
            }
        }
    }

    std::cout << "izmlgenc: generated " << config.lang << "\n";
    return 0;
}
