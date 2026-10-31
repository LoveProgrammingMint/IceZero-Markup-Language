#include "izml_runtime.h"

#include "izml_core.hpp"

#include <string>

struct izmlrt_archive
{
    izml::Archive archive;
};

namespace
{
    thread_local std::string g_error;

    void set_error(const std::string& message)
    {
        g_error = message;
    }

    template <typename T>
    int32_t read_with(izmlrt_archive* archive, T* value, T (izml::Reader::*method)())
    {
        if (archive == nullptr || value == nullptr)
        {
            set_error("null archive or output");
            return 0;
        }
        try
        {
            *value = (archive->archive.reader().*method)();
            return 1;
        }
        catch (const std::exception& exception)
        {
            set_error(exception.what());
            return 0;
        }
    }
}

extern "C" IZMLRUNTIME_API izmlrt_archive* izmlrt_open(const char* path)
{
    if (path == nullptr)
    {
        set_error("null path");
        return nullptr;
    }
    try
    {
        izmlrt_archive* result = new izmlrt_archive();
        result->archive = izml::Archive::from_file(path);
        return result;
    }
    catch (const std::exception& exception)
    {
        set_error(exception.what());
        return nullptr;
    }
}

extern "C" IZMLRUNTIME_API izmlrt_archive* izmlrt_open_memory(const uint8_t* data, uint64_t length)
{
    if (data == nullptr || length == 0)
    {
        set_error("empty IZRC buffer");
        return nullptr;
    }
    try
    {
        izml::Bytes bytes(data, data + length);
        izmlrt_archive* result = new izmlrt_archive();
        result->archive = izml::Archive::load(bytes);
        return result;
    }
    catch (const std::exception& exception)
    {
        set_error(exception.what());
        return nullptr;
    }
}

extern "C" IZMLRUNTIME_API void izmlrt_close(izmlrt_archive* archive)
{
    delete archive;
}

extern "C" IZMLRUNTIME_API uint16_t izmlrt_version(const izmlrt_archive* archive)
{
    return archive != nullptr ? archive->archive.version() : 0;
}

extern "C" IZMLRUNTIME_API uint16_t izmlrt_min_version(const izmlrt_archive* archive)
{
    return archive != nullptr ? archive->archive.min_version() : 0;
}

extern "C" IZMLRUNTIME_API uint32_t izmlrt_root_count(const izmlrt_archive* archive)
{
    return archive != nullptr ? archive->archive.root_count() : 0;
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_u8(izmlrt_archive* archive, uint8_t* value)
{
    return read_with(archive, value, &izml::Reader::u8);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_i8(izmlrt_archive* archive, int8_t* value)
{
    return read_with(archive, value, &izml::Reader::i8);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_u16(izmlrt_archive* archive, uint16_t* value)
{
    return read_with(archive, value, &izml::Reader::u16);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_i16(izmlrt_archive* archive, int16_t* value)
{
    return read_with(archive, value, &izml::Reader::i16);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_u32(izmlrt_archive* archive, uint32_t* value)
{
    return read_with(archive, value, &izml::Reader::u32);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_i32(izmlrt_archive* archive, int32_t* value)
{
    return read_with(archive, value, &izml::Reader::i32);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_u64(izmlrt_archive* archive, uint64_t* value)
{
    return read_with(archive, value, &izml::Reader::u64);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_i64(izmlrt_archive* archive, int64_t* value)
{
    return read_with(archive, value, &izml::Reader::i64);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_f32(izmlrt_archive* archive, float* value)
{
    return read_with(archive, value, &izml::Reader::f32);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_f64(izmlrt_archive* archive, double* value)
{
    return read_with(archive, value, &izml::Reader::f64);
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_bool(izmlrt_archive* archive, int32_t* value)
{
    if (archive == nullptr || value == nullptr)
    {
        set_error("null archive or output");
        return 0;
    }
    try
    {
        *value = archive->archive.reader().boolean() ? 1 : 0;
        return 1;
    }
    catch (const std::exception& exception)
    {
        set_error(exception.what());
        return 0;
    }
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_string(izmlrt_archive* archive, const char** data, uint32_t* length)
{
    if (archive == nullptr || data == nullptr || length == nullptr)
    {
        set_error("null archive or output");
        return 0;
    }
    try
    {
        izml::Reader& reader = archive->archive.reader();
        const uint32_t size = reader.u32();
        if (size > reader.size() - reader.position())
        {
            set_error("truncated string");
            return 0;
        }
        *data = reinterpret_cast<const char*>(reader.current());
        *length = size;
        reader.skip(size);
        return 1;
    }
    catch (const std::exception& exception)
    {
        set_error(exception.what());
        return 0;
    }
}

extern "C" IZMLRUNTIME_API int32_t izmlrt_read_bytes(izmlrt_archive* archive, const uint8_t** data, uint32_t* length)
{
    if (archive == nullptr || data == nullptr || length == nullptr)
    {
        set_error("null archive or output");
        return 0;
    }
    try
    {
        izml::Reader& reader = archive->archive.reader();
        const uint32_t size = reader.u32();
        if (size > reader.size() - reader.position())
        {
            set_error("truncated bytes");
            return 0;
        }
        *data = reader.current();
        *length = size;
        reader.skip(size);
        return 1;
    }
    catch (const std::exception& exception)
    {
        set_error(exception.what());
        return 0;
    }
}

extern "C" IZMLRUNTIME_API const char* izmlrt_last_error(void)
{
    return g_error.c_str();
}
