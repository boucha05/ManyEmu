#include "Serialization.h"
#include "Stream.h"
#include <vector>

namespace NES
{
    void ISerializer::serialize(uint32_t* values, size_t size)
    {
        for (size_t index = 0; index < size; ++index)
            serialize(*values++);
    }

    void ISerializer::serialize(uint8_t* values, size_t size)
    {
        for (size_t index = 0; index < size; ++index)
            serialize(*values++);
    }

    void ISerializer::serialize(std::vector<uint8_t>& value)
    {
        uint32_t size = static_cast<uint32_t>(value.size());
        serialize(size);
        value.resize(size, 0);

        if (size)
            serialize(&value[0], size * sizeof(uint8_t));
    }

    ///////////////////////////////////////////////////////////////////////////

    BinaryReader::BinaryReader(IStream& stream)
        : mStream(&stream)
    {
    }

    BinaryReader::~BinaryReader()
    {
    }

    bool BinaryReader::isReading() const
    {
        return true;
    }

    void BinaryReader::serialize(uint32_t& data)
    {
        serializeSafe(&data, sizeof(data));
    }

    void BinaryReader::serialize(int32_t& data)
    {
        serializeSafe(&data, sizeof(data));
    }

    void BinaryReader::serialize(uint16_t& data)
    {
        serializeSafe(&data, sizeof(data));
    }

    void BinaryReader::serialize(uint8_t& data)
    {
        serializeSafe(&data, sizeof(data));
    }

    void BinaryReader::serialize(int8_t& data)
    {
        serializeSafe(&data, sizeof(data));
    }

    void BinaryReader::serialize(bool& data)
    {
        serializeSafe(&data, sizeof(data));
    }

    void BinaryReader::serialize(void* data, size_t size)
    {
        serializeSafe(data, size);
    }

    void BinaryReader::serializeSafe(void* data, size_t size)
    {
        mStream->read(data, size);
    }

    ///////////////////////////////////////////////////////////////////////////

    BinaryWriter::BinaryWriter(IStream& stream)
        : mStream(&stream)
    {
    }

    BinaryWriter::~BinaryWriter()
    {
    }

    bool BinaryWriter::isReading() const
    {
        return false;
    }

    void BinaryWriter::serialize(uint32_t& data)
    {
        mStream->write(&data, sizeof(data));
    }

    void BinaryWriter::serialize(int32_t& data)
    {
        mStream->write(&data, sizeof(data));
    }

    void BinaryWriter::serialize(uint16_t& data)
    {
        mStream->write(&data, sizeof(data));
    }

    void BinaryWriter::serialize(uint8_t& data)
    {
        mStream->write(&data, sizeof(data));
    }

    void BinaryWriter::serialize(int8_t& data)
    {
        mStream->write(&data, sizeof(data));
    }

    void BinaryWriter::serialize(bool& data)
    {
        mStream->write(&data, sizeof(data));
    }

    void BinaryWriter::serialize(void* data, size_t size)
    {
        mStream->write(data, size);
    }

    ///////////////////////////////////////////////////////////////////////////

    TextWriter::TextWriter(IStream& stream)
        : mStream(&stream)
    {
    }

    TextWriter::~TextWriter()
    {
    }

    bool TextWriter::isReading() const
    {
        return false;
    }

    void TextWriter::serialize(uint32_t& data)
    {
        write("%d", data);
    }

    void TextWriter::serialize(int32_t& data)
    {
        write("%d", data);
    }

    void TextWriter::serialize(uint16_t& data)
    {
        write("%d", data);
    }

    void TextWriter::serialize(uint8_t& data)
    {
        write("%d", data);
    }

    void TextWriter::serialize(int8_t& data)
    {
        write("%d", data);
    }

    void TextWriter::serialize(bool& data)
    {
        write(data ? "true" : "false");
    }

    void TextWriter::serialize(void* data, size_t size)
    {
        mStream->write(data, size);
    }

    void TextWriter::write(const char* text, ...)
    {
        char temp[4096];
        va_list args;
        va_start(args, text);
        size_t size = SDL_vsnprintf(temp, sizeof(temp), text, args);
        mStream->write(temp, size);
        va_end(args);
    }
}