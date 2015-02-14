#include "Serialization.h"
#include "Stream.h"
#include <vector>

namespace NES
{
    BinaryReader::BinaryReader(IStream& stream)
        : mStream(&stream)
    {
    }

    BinaryReader::~BinaryReader()
    {
    }

    void BinaryReader::serialize(uint32_t& data)
    {
        serializeSafe(&data, sizeof(data));
    }

    void BinaryReader::serialize(uint8_t& data)
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

    void BinaryWriter::serialize(uint32_t& data)
    {
        mStream->write(&data, sizeof(data));
    }

    void BinaryWriter::serialize(uint8_t& data)
    {
        mStream->write(&data, sizeof(data));
    }

    void BinaryWriter::serialize(void* data, size_t size)
    {
        mStream->write(data, size);
    }
}