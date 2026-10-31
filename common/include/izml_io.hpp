#ifndef IZML_IO_HPP
#define IZML_IO_HPP

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace izmlio
{
    using String = std::string;
    using Byte = std::uint8_t;
    using Bytes = std::vector<Byte>;

    inline bool read_file(const String& path, String& out)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            return false;
        }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        out = buffer.str();
        return true;
    }

    inline bool write_file(const String& path, const String& content)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            return false;
        }
        stream.write(content.data(), static_cast<std::streamsize>(content.size()));
        return stream.good();
    }

    inline bool write_bytes(const String& path, const Bytes& content)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            return false;
        }
        stream.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
        return stream.good();
    }

    inline bool file_exists(const String& path)
    {
        std::ifstream stream(path, std::ios::binary);
        return static_cast<bool>(stream);
    }
}

#endif
