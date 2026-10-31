#include "lang_config.hpp"

#include "izml_io.hpp"

#include <filesystem>
#include <vector>

namespace izmlgenc
{
    namespace
    {
        String trim(const String& text)
        {
            std::size_t begin = 0;
            std::size_t end = text.size();
            while (begin < end && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r'))
            {
                ++begin;
            }
            while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r'))
            {
                --end;
            }
            return text.substr(begin, end - begin);
        }

        String unquote(const String& text)
        {
            if (text.size() >= 2)
            {
                const char first = text.front();
                const char last = text.back();
                if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
                {
                    return text.substr(1, text.size() - 2);
                }
            }
            return text;
        }

        bool parse_assignment(const String& text, String& key, String& value)
        {
            const std::size_t equals = text.find('=');
            if (equals == String::npos)
            {
                return false;
            }
            key = trim(text.substr(0, equals));
            value = unquote(trim(text.substr(equals + 1)));
            return !key.empty();
        }

        String normalize(String text)
        {
            String result;
            result.reserve(text.size());
            for (std::size_t i = 0; i < text.size(); ++i)
            {
                if (text[i] == '\r')
                {
                    if (i + 1 < text.size() && text[i + 1] == '\n')
                    {
                        continue;
                    }
                    result.push_back('\n');
                    continue;
                }
                result.push_back(text[i]);
            }
            return result;
        }

        bool parse_header(const String& line, LangConfig& config, String& error)
        {
            if (line.rfind("@izmlgen-lang", 0) != 0)
            {
                error = "missing '@izmlgen-lang' header";
                return false;
            }
            std::size_t cursor = 13;
            while (cursor < line.size())
            {
                while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t'))
                {
                    ++cursor;
                }
                std::size_t next = cursor;
                while (next < line.size() && line[next] != ' ' && line[next] != '\t')
                {
                    ++next;
                }
                String key;
                String value;
                if (parse_assignment(line.substr(cursor, next - cursor), key, value))
                {
                    if (key == "lang") { config.lang = value; }
                    else if (key == "version") { config.version = value; }
                    else { config.config[key] = value; }
                }
                cursor = next;
            }
            if (config.lang.empty())
            {
                error = "the '@izmlgen-lang' header has no lang";
                return false;
            }
            return true;
        }

        bool parse_file(const String& path, LangConfig& config, String& error)
        {
            String text;
            if (!izmlio::read_file(path, text))
            {
                error = "unable to read '" + path + "'";
                return false;
            }
            config.path = path;
            text = normalize(text);

            enum class Section
            {
                None,
                Config,
                Types,
                Readers,
                Keywords,
                Macros,
                Template,
                Runtime
            };

            Section section = Section::None;
            LangOutput output;
            std::vector<String> body_lines;
            bool header_seen = false;
            bool file_seen = false;

            auto finish_output = [&]() -> bool
            {
                if (!file_seen)
                {
                    error = "section '" + output.name + "' in '" + path + "' has no file";
                    return false;
                }
                String body;
                for (const String& line : body_lines)
                {
                    body += line;
                }
                output.body = body;
                config.outputs.push_back(output);
                return true;
            };

            std::size_t start = 0;
            while (start <= text.size())
            {
                const std::size_t end = text.find('\n', start);
                const String line = end == String::npos ? text.substr(start) : text.substr(start, end - start);
                const String trimmed = trim(line);
                start = end == String::npos ? text.size() + 1 : end + 1;

                if (!header_seen)
                {
                    if (trimmed.empty())
                    {
                        continue;
                    }
                    config.config.clear();
                    String header_error;
                    if (!parse_header(trimmed, config, header_error))
                    {
                        error = header_error + " in '" + path + "'";
                        return false;
                    }
                    header_seen = true;
                    continue;
                }

                if (section == Section::Macros)
                {
                    if (trimmed == "[/macros]")
                    {
                        for (const String& body_line : body_lines)
                        {
                            config.macros += body_line;
                        }
                        body_lines.clear();
                        section = Section::None;
                        continue;
                    }
                    body_lines.push_back(line + "\n");
                    continue;
                }
                if (section == Section::Template || section == Section::Runtime)
                {
                    if (trimmed == (section == Section::Template ? "[/template]" : "[/runtime]"))
                    {
                        if (!finish_output())
                        {
                            return false;
                        }
                        body_lines.clear();
                        output = LangOutput();
                        file_seen = false;
                        section = Section::None;
                        continue;
                    }
                    if (!file_seen)
                    {
                        String key;
                        String value;
                        if (parse_assignment(trimmed, key, value) && key == "file")
                        {
                            output.file = value;
                            file_seen = true;
                            continue;
                        }
                        error = "the first line of '" + output.name + "' must be 'file = ...'";
                        return false;
                    }
                    body_lines.push_back(line + "\n");
                    continue;
                }

                if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']')
                {
                    const String name = trimmed.substr(1, trimmed.size() - 2);
                    if (name == "config" || name == "types" || name == "readers" || name == "keywords")
                    {
                        section = name == "config" ? Section::Config :
                            (name == "types" ? Section::Types :
                                (name == "readers" ? Section::Readers : Section::Keywords));
                        continue;
                    }
                    if (name == "macros")
                    {
                        section = Section::Macros;
                        body_lines.clear();
                        continue;
                    }
                    if (name == "runtime")
                    {
                        section = Section::Runtime;
                        output.name = "runtime";
                        output.runtime = true;
                        body_lines.clear();
                        file_seen = false;
                        continue;
                    }
                    if (name.rfind("template:", 0) == 0)
                    {
                        section = Section::Template;
                        output.name = name.substr(9);
                        output.runtime = false;
                        body_lines.clear();
                        file_seen = false;
                        continue;
                    }
                    error = "unknown section '[" + name + "]' in '" + path + "'";
                    return false;
                }

                if (section == Section::Config || section == Section::Types ||
                    section == Section::Readers || section == Section::Keywords)
                {
                    if (trimmed.empty() || trimmed[0] == '#' || (trimmed.size() >= 2 && trimmed[0] == '/' && trimmed[1] == '/'))
                    {
                        continue;
                    }
                    String key;
                    String value;
                    if (!parse_assignment(trimmed, key, value))
                    {
                        error = "malformed option '" + trimmed + "' in '" + path + "'";
                        return false;
                    }
                    if (section == Section::Config) { config.config[key] = value; }
                    else if (section == Section::Types) { config.types[key] = value; }
                    else if (section == Section::Readers) { config.readers[key] = value; }
                    else { config.keywords[key] = value; }
                    continue;
                }

                if (!trimmed.empty())
                {
                    error = "unexpected text outside a section in '" + path + "': " + trimmed;
                    return false;
                }
            }

            if (section == Section::Macros)
            {
                for (const String& body_line : body_lines)
                {
                    config.macros += body_line;
                }
            }
            else if (section == Section::Template || section == Section::Runtime)
            {
                if (!finish_output())
                {
                    return false;
                }
            }
            if (!header_seen)
            {
                error = "empty language config '" + path + "'";
                return false;
            }
            return true;
        }
    }

    bool load_lang_configs(const String& directory, std::map<String, LangConfig>& configs, String& error)
    {
        configs.clear();
        std::filesystem::path root(directory);
        std::error_code code;
        if (!std::filesystem::is_directory(root, code))
        {
            error = "language config directory not found: " + directory;
            return false;
        }

        std::vector<std::filesystem::path> files;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root, code))
        {
            if (!entry.is_regular_file(code))
            {
                continue;
            }
            const String name = entry.path().filename().string();
            if (name.size() > 13 && name.compare(name.size() - 13, 13, ".izmlgen-lang") == 0)
            {
                files.push_back(entry.path());
            }
        }
        if (code)
        {
            error = "unable to scan language configs in " + directory;
            return false;
        }

        for (const std::filesystem::path& file : files)
        {
            LangConfig config;
            String parse_error;
            if (!parse_file(file.string(), config, parse_error))
            {
                error = parse_error;
                return false;
            }
            auto existing = configs.find(config.lang);
            if (existing != configs.end())
            {
                error = "duplicate language '" + config.lang + "' in '" + existing->second.path + "' and '" + config.path + "'";
                return false;
            }
            configs.emplace(config.lang, std::move(config));
        }
        return true;
    }
}
