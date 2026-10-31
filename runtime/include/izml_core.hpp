#ifndef IZML_CORE_HPP
#define IZML_CORE_HPP

#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace izml
{
    using Int8 = std::int8_t;
    using Int16 = std::int16_t;
    using Int32 = std::int32_t;
    using Int64 = std::int64_t;
    using UInt8 = std::uint8_t;
    using UInt16 = std::uint16_t;
    using UInt32 = std::uint32_t;
    using UInt64 = std::uint64_t;
    using Byte = std::uint8_t;
    using Float32 = float;
    using Float64 = double;
    using String = std::string;
    using Bytes = std::vector<Byte>;

    class Error : public std::runtime_error
    {
    public:
        explicit Error(const String& message) : std::runtime_error(message)
        {
        }
    };

    class Reader
    {
    public:
        Reader() = default;

        Reader(const Byte* data, std::size_t size) : data_(data), size_(size)
        {
        }

        std::size_t position() const
        {
            return position_;
        }

        std::size_t size() const
        {
            return size_;
        }

        bool at_end() const
        {
            return position_ >= size_;
        }

        const Byte* current() const
        {
            return data_ + position_;
        }

        void skip(std::size_t count)
        {
            need(count);
            position_ += count;
        }

        Byte u8()
        {
            return read_le<Byte>();
        }

        Int8 i8()
        {
            return static_cast<Int8>(read_le<Byte>());
        }

        UInt16 u16()
        {
            return read_le<UInt16>();
        }

        Int16 i16()
        {
            return static_cast<Int16>(read_le<UInt16>());
        }

        UInt32 u32()
        {
            return read_le<UInt32>();
        }

        Int32 i32()
        {
            return static_cast<Int32>(read_le<UInt32>());
        }

        UInt64 u64()
        {
            return read_le<UInt64>();
        }

        Int64 i64()
        {
            return static_cast<Int64>(read_le<UInt64>());
        }

        Float32 f32()
        {
            const UInt32 bits = read_le<UInt32>();
            Float32 value = 0.0f;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }

        Float64 f64()
        {
            const UInt64 bits = read_le<UInt64>();
            Float64 value = 0.0;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }

        bool boolean()
        {
            return read_le<Byte>() != 0;
        }

        String string()
        {
            const UInt32 length = read_le<UInt32>();
            need(length);
            String value(reinterpret_cast<const char*>(data_ + position_), length);
            position_ += length;
            return value;
        }

        Bytes bytes()
        {
            const UInt32 length = read_le<UInt32>();
            need(length);
            Bytes value(data_ + position_, data_ + position_ + length);
            position_ += length;
            return value;
        }

    private:
        void need(std::size_t count)
        {
            if (data_ == nullptr || count > size_ - position_)
            {
                throw Error("truncated IZRC data");
            }
        }

        template <typename T>
        T read_le()
        {
            using U = std::conditional_t<std::is_signed_v<T>, std::make_unsigned_t<T>, T>;
            need(sizeof(T));
            U value = 0;
            for (std::size_t i = 0; i < sizeof(T); ++i)
            {
                value |= static_cast<U>(static_cast<U>(data_[position_ + i]) << (8 * i));
            }
            position_ += sizeof(T);
            return static_cast<T>(value);
        }

        const Byte* data_ = nullptr;
        std::size_t size_ = 0;
        std::size_t position_ = 0;
    };

    class Archive
    {
    public:
        Archive() = default;
        Archive(const Archive&) = delete;
        Archive& operator=(const Archive&) = delete;
        Archive(Archive&&) = default;
        Archive& operator=(Archive&&) = default;

        static Archive from_file(const String& path)
        {
            std::ifstream stream(path, std::ios::binary);
            if (!stream)
            {
                throw Error("unable to open '" + path + "'");
            }
            Bytes bytes;
            char buffer[4096];
            while (stream.read(buffer, sizeof(buffer)) || stream.gcount() != 0)
            {
                bytes.insert(bytes.end(), buffer, buffer + stream.gcount());
            }
            return load(bytes);
        }

        static Archive load(const Bytes& bytes)
        {
            Archive archive;
            archive.owned_ = bytes;
            archive.init();
            return archive;
        }

        UInt16 version() const
        {
            return version_;
        }

        UInt16 min_version() const
        {
            return min_version_;
        }

        UInt32 root_count() const
        {
            return root_count_;
        }

        Reader& reader()
        {
            return reader_;
        }

        const Reader& reader() const
        {
            return reader_;
        }

        const Bytes& bytes() const
        {
            return owned_;
        }

    private:
        void init()
        {
            if (owned_.size() < 16)
            {
                throw Error("IZRC file is shorter than the 16-byte header");
            }
            if (std::memcmp(owned_.data(), "IZRC", 4) != 0)
            {
                throw Error("invalid IZRC magic");
            }
            version_ = static_cast<UInt16>(owned_[4] | (owned_[5] << 8));
            min_version_ = static_cast<UInt16>(owned_[6] | (owned_[7] << 8));
            root_count_ = static_cast<UInt32>(owned_[8] | (owned_[9] << 8) | (owned_[10] << 16) | (static_cast<UInt32>(owned_[11]) << 24));
            if (version_ != 1)
            {
                throw Error("unsupported IZRC version " + std::to_string(version_));
            }
            reader_ = Reader(owned_.data() + 16, owned_.size() - 16);
        }

        Bytes owned_;
        Reader reader_;
        UInt16 version_ = 0;
        UInt16 min_version_ = 0;
        UInt32 root_count_ = 0;
    };

    enum class FieldKind : Byte
    {
        Bool,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Float32,
        Float64,
        String,
        Enum,
        Define,
        List
    };

    struct FieldDescriptor
    {
        FieldKind kind;
        Byte size;
        Byte isOptional;
    };

    struct TemplateLayout
    {
        Int32 typeTag;
        UInt32 fieldCount;
        UInt32 optionalCount;
        const FieldDescriptor* fields;
    };

    struct InstanceDesc
    {
        const char* name;
        Int32 typeTag;
        UInt32 rootIndex;
    };

    struct ListViewDesc
    {
        const char* name;
        Int32 elementTag;
    };
}

#endif
