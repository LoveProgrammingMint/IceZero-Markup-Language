#ifndef IZMLC_IZRC_HPP
#define IZMLC_IZRC_HPP

#include "izml_io.hpp"
#include "izml_ir.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace izmlc
{
    using Int32 = std::int32_t;
    using Int64 = std::int64_t;
    using UInt8 = std::uint8_t;
    using UInt16 = std::uint16_t;
    using UInt32 = std::uint32_t;
    using UInt64 = std::uint64_t;
    using Byte = std::uint8_t;
    using String = std::string;
    using Bytes = std::vector<Byte>;

    class IzrcWriter
    {
    public:
        explicit IzrcWriter(const izmlr::Program& program);

        bool write(const String& path);

        UInt32 root_count() const
        {
            return root_count_;
        }

        UInt64 byte_size() const
        {
            return static_cast<UInt64>(buffer_.size());
        }

        const std::vector<String>& errors() const
        {
            return errors_;
        }

    private:
        void write_header();
        void write_root(const izmlr::Using& instance);
        void write_object(const izmlr::Define& define, const std::vector<izmlr::Entry>& source);
        void write_zero_object(const izmlr::Define& define);
        void write_value(const izmlr::Field& field, const izmlr::Value& value);
        void write_element(const String& kind, const String& name, const izmlr::Value& value);
        void write_element_enum(const String& name, const izmlr::Value& value);
        void write_zero(const izmlr::Field& field);
        const izmlr::Define* find_define(const String& qualified) const;
        const izmlr::Enum* find_enum(const String& name) const;
        const izmlr::Entry* find_entry(const std::vector<izmlr::Entry>& source, const String& name) const;
        void error(const String& message);

        void put_u8(Byte value);
        void put_u16(UInt16 value);
        void put_u32(UInt32 value);
        void put_u64(UInt64 value);
        void put_f32(float value);
        void put_f64(double value);
        void put_zeros(Int32 count);

        const izmlr::Program& program_;
        Bytes buffer_;
        std::vector<String> errors_;
        UInt32 root_count_ = 0;
    };
}

#endif
