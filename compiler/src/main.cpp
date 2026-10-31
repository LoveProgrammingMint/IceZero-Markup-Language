#include "izml_ir_build.hpp"
#include "izml_parser_c_api.h"
#include "izrc_writer.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    using Int32 = std::int32_t;
    using String = std::string;

    void usage()
    {
        std::cout <<
            "izmlc - IZML to IZRC compiler\n"
            "\n"
            "usage: izmlc <input.izml> [options]\n"
            "\n"
            "options:\n"
            "  -o, --out <path>   output file (default: <input>.izrc)\n"
            "  -h, --help         show this help\n";
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
    String input;
    String output;
    for (std::size_t i = 0; i < args.size(); ++i)
    {
        const String& arg = args[i];
        if (arg == "-h" || arg == "--help")
        {
            usage();
            return 0;
        }
        if (arg == "-o" || arg == "--out")
        {
            if (i + 1 >= args.size())
            {
                std::cerr << "izmlc: missing value for " << arg << "\n";
                return 2;
            }
            output = args[++i];
            continue;
        }
        if (!arg.empty() && arg[0] == '-')
        {
            std::cerr << "izmlc: unknown option '" << arg << "'\n";
            return 2;
        }
        if (!input.empty())
        {
            std::cerr << "izmlc: unexpected argument '" << arg << "'\n";
            return 2;
        }
        input = arg;
    }

    if (input.empty())
    {
        std::cerr << "izmlc: no input file was given\n";
        usage();
        return 2;
    }

    izml_document* document = izml_parse_file(input.c_str());
    if (document == nullptr)
    {
        std::cerr << "izmlc: unable to open '" << input << "'\n";
        return 1;
    }
    if (izml_document_ok(document) == 0)
    {
        print_diagnostics(document);
        std::cerr << "izmlc: '" << input << "' did not pass analysis\n";
        izml_document_destroy(document);
        return 1;
    }

    izmlr::Program program;
    if (!izmlr::build(document, program))
    {
        std::cerr << "izmlc: unable to build IZLR for '" << input << "'\n";
        izml_document_destroy(document);
        return 1;
    }
    izml_document_destroy(document);

    if (output.empty())
    {
        output = input;
        const std::size_t dot = output.find_last_of('.');
        const std::size_t slash = output.find_last_of("\\/");
        if (dot != String::npos && (slash == String::npos || dot > slash))
        {
            output.erase(dot);
        }
        output += ".izrc";
    }

    izmlc::IzrcWriter writer(program);
    const bool ok = writer.write(output);
    for (const String& message : writer.errors())
    {
        std::cerr << "izmlc: " << message << "\n";
    }
    if (!ok)
    {
        return 1;
    }

    std::cout << "izmlc: wrote " << output << " (" << writer.root_count()
        << " root" << (writer.root_count() == 1 ? "" : "s") << ", "
        << writer.byte_size() << " bytes)\n";
    return 0;
}
