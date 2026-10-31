#include "izrc_writer.hpp"

#include <cstring>

namespace izmlc
{
    IzrcWriter::IzrcWriter(const izmlr::Program& program) : program_(program)
    {
    }

    void IzrcWriter::put_u8(Byte value)
    {
        buffer_.push_back(value);
    }

    void IzrcWriter::put_u16(UInt16 value)
    {
        put_u8(static_cast<Byte>(value & 0xFF));
        put_u8(static_cast<Byte>((value >> 8) & 0xFF));
    }

    void IzrcWriter::put_u32(UInt32 value)
    {
        put_u16(static_cast<UInt16>(value & 0xFFFF));
        put_u16(static_cast<UInt16>((value >> 16) & 0xFFFF));
    }

    void IzrcWriter::put_u64(UInt64 value)
    {
        put_u32(static_cast<UInt32>(value & 0xFFFFFFFF));
        put_u32(static_cast<UInt32>((value >> 32) & 0xFFFFFFFF));
    }

    void IzrcWriter::put_f32(float value)
    {
        UInt32 bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        put_u32(bits);
    }

    void IzrcWriter::put_f64(double value)
    {
        UInt64 bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        put_u64(bits);
    }

    void IzrcWriter::put_zeros(Int32 count)
    {
        for (Int32 i = 0; i < count; ++i)
        {
            put_u8(0);
        }
    }

    void IzrcWriter::error(const String& message)
    {
        errors_.push_back(message);
    }

    const izmlr::Define* IzrcWriter::find_define(const String& qualified) const
    {
        return program_.find_define(qualified);
    }

    const izmlr::Enum* IzrcWriter::find_enum(const String& name) const
    {
        return program_.find_enum(name);
    }

    const izmlr::Entry* IzrcWriter::find_entry(const std::vector<izmlr::Entry>& source, const String& name) const
    {
        for (const izmlr::Entry& entry : source)
        {
            if (entry.name == name)
            {
                return &entry;
            }
        }
        return nullptr;
    }

    bool IzrcWriter::write(const String& path)
    {
        buffer_.clear();
        errors_.clear();
        root_count_ = static_cast<UInt32>(program_.root_count);
        write_header();
        for (const izmlr::Using& instance : program_.usings)
        {
            if (instance.is_list)
            {
                continue;
            }
            write_root(instance);
        }
        if (!errors_.empty())
        {
            return false;
        }
        return izmlio::write_bytes(path, buffer_);
    }

    void IzrcWriter::write_header()
    {
        buffer_.insert(buffer_.end(), { 'I', 'Z', 'R', 'C' });
        put_u16(1);
        put_u16(1);
        put_u32(root_count_);
        put_u32(0);
    }

    void IzrcWriter::write_root(const izmlr::Using& instance)
    {
        const izmlr::Define* define = find_define(instance.type_name);
        if (define == nullptr)
        {
            error("unknown define '" + instance.type_name + "' for instance '" + instance.name + "'");
            return;
        }
        put_u32(static_cast<UInt32>(define->type_tag));
        write_object(*define, instance.entries);
    }

    void IzrcWriter::write_object(const izmlr::Define& define, const std::vector<izmlr::Entry>& source)
    {
        const Int32 bitmapSize = (define.optional_count + 7) / 8;
        std::vector<Byte> bitmap(static_cast<std::size_t>(bitmapSize), 0);
        for (const izmlr::Field& field : define.layout)
        {
            if (!field.optional)
            {
                continue;
            }
            if (find_entry(source, field.name) == nullptr)
            {
                continue;
            }
            const Int32 bit = field.optional_bit;
            bitmap[static_cast<std::size_t>(bit / 8)] |= static_cast<Byte>(1u << (bit % 8));
        }
        for (Byte value : bitmap)
        {
            put_u8(value);
        }
        for (const izmlr::Field& field : define.layout)
        {
            const izmlr::Entry* entry = find_entry(source, field.name);
            if (entry != nullptr)
            {
                write_value(field, entry->value);
                continue;
            }
            if (!field.optional)
            {
                error("missing required field '" + field.name + "' in '" + define.name + "'");
                return;
            }
            write_zero(field);
        }
    }

    void IzrcWriter::write_zero_object(const izmlr::Define& define)
    {
        put_zeros((define.optional_count + 7) / 8);
        for (const izmlr::Field& field : define.layout)
        {
            write_zero(field);
        }
    }

    void IzrcWriter::write_zero(const izmlr::Field& field)
    {
        if (field.kind == "list" || field.kind == "string")
        {
            put_u32(0);
            return;
        }
        if (field.kind == "define")
        {
            const izmlr::Define* define = find_define(field.type_name);
            if (define == nullptr)
            {
                error("unknown define '" + field.type_name + "'");
                return;
            }
            write_zero_object(*define);
            return;
        }
        if (field.kind == "enum")
        {
            const izmlr::Enum* enum_type = find_enum(field.type_name);
            put_zeros(enum_type != nullptr ? enum_type->base_size : 1);
            return;
        }
        put_zeros(field.size);
    }

    void IzrcWriter::write_value(const izmlr::Field& field, const izmlr::Value& value)
    {
        if (field.kind == "list")
        {
            if (value.kind != izmlr::ValueKind::Array)
            {
                error("field '" + field.name + "' expects a list value");
                return;
            }
            put_u32(static_cast<UInt32>(value.items.size()));
            for (const izmlr::Value& item : value.items)
            {
                write_element(field.element_kind, field.element_name, item);
            }
            return;
        }
        write_element(field.kind, field.type_name, value);
    }

    void IzrcWriter::write_element(const String& kind, const String& name, const izmlr::Value& value)
    {
        const Int64 intValue = value.integer;
        const double doubleValue = value.kind == izmlr::ValueKind::Int ? static_cast<double>(intValue) : value.real;
        if (kind == "bool")
        {
            put_u8(value.boolean ? 1 : 0);
            return;
        }
        if (kind == "byte" || kind == "sbyte")
        {
            put_u8(static_cast<Byte>(intValue));
            return;
        }
        if (kind == "short" || kind == "ushort")
        {
            put_u16(static_cast<UInt16>(intValue));
            return;
        }
        if (kind == "int" || kind == "uint")
        {
            put_u32(static_cast<UInt32>(intValue));
            return;
        }
        if (kind == "long" || kind == "ulong")
        {
            put_u64(static_cast<UInt64>(intValue));
            return;
        }
        if (kind == "float")
        {
            put_f32(static_cast<float>(doubleValue));
            return;
        }
        if (kind == "double")
        {
            put_f64(doubleValue);
            return;
        }
        if (kind == "string")
        {
            put_u32(static_cast<UInt32>(value.text.size()));
            buffer_.insert(buffer_.end(), value.text.begin(), value.text.end());
            return;
        }
        if (kind == "enum")
        {
            write_element_enum(name, value);
            return;
        }
        if (kind == "define")
        {
            const izmlr::Define* define = find_define(name);
            if (define == nullptr)
            {
                error("unknown define '" + name + "'");
                return;
            }
            write_object(*define, value.entries);
            return;
        }
        error("unsupported field type '" + kind + "'");
    }

    void IzrcWriter::write_element_enum(const String& name, const izmlr::Value& value)
    {
        const izmlr::Enum* enum_type = find_enum(name);
        if (enum_type == nullptr)
        {
            error("unknown enum '" + name + "'");
            return;
        }
        Int64 raw = value.integer;
        if (value.kind == izmlr::ValueKind::Ref)
        {
            String member = value.text;
            const std::size_t dot = member.find_last_of('.');
            if (dot != String::npos)
            {
                member = member.substr(dot + 1);
            }
            bool found = false;
            for (const izmlr::EnumMember& candidate : enum_type->members)
            {
                if (candidate.name == member)
                {
                    raw = candidate.value;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                error("unknown enum member '" + value.text + "' in '" + name + "'");
                return;
            }
        }
        if (enum_type->base_kind == "byte" || enum_type->base_kind == "sbyte")
        {
            put_u8(static_cast<Byte>(raw));
        }
        else if (enum_type->base_kind == "short" || enum_type->base_kind == "ushort")
        {
            put_u16(static_cast<UInt16>(raw));
        }
        else if (enum_type->base_kind == "int" || enum_type->base_kind == "uint")
        {
            put_u32(static_cast<UInt32>(raw));
        }
        else
        {
            put_u64(static_cast<UInt64>(raw));
        }
    }
}
