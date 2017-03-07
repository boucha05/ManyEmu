#include "Serializer.h"
#include "Stream.h"

namespace emu
{
    BinaryReader::BinaryReader(IStream& stream)
        : mStream(&stream)
        , mSuccess(false)
    {
    }

    IStream& BinaryReader::getStream()
    {
        return *mStream;
    }

    bool BinaryReader::success() const
    {
        return mSuccess;
    }

    bool BinaryReader::isWriting() const
    {
        return false;
    }

    void BinaryReader::value(bool& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(char& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(int8_t& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(uint8_t& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(int16_t& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(uint16_t& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(int32_t& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(uint32_t& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(int64_t& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(uint64_t& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(float& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(double& item)
    {
        mSuccess |= mStream->read(&item, sizeof(item));
    }

    void BinaryReader::value(std::string& item)
    {
        uint32_t size = 0;
        value(size);
        item.resize(size);
        if (size)
            mSuccess |= mStream->read(&item[0], size);
    }

    void BinaryReader::value(emu::Buffer& item)
    {
        uint32_t size = 0;
        value(size);
        item.resize(size);
        mSuccess |= mStream->read(item.data(), size);
    }

    bool BinaryReader::nodeBegin(const char* name)
    {
        EMU_UNUSED(name);
        return true;
    }

    void BinaryReader::nodeEnd()
    {
    }

    bool BinaryReader::sequenceBegin(size_t& size)
    {
        uint32_t serializedSize = 0;
        value(serializedSize);
        size = serializedSize;
        return true;
    }

    void BinaryReader::sequenceEnd()
    {
    }

    void BinaryReader::sequenceItem()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////

    BinaryWriter::BinaryWriter(IStream& stream)
        : mStream(&stream)
        , mSuccess(false)
    {
    }

    IStream& BinaryWriter::getStream()
    {
        return *mStream;
    }

    bool BinaryWriter::success() const
    {
        return mSuccess;
    }

    bool BinaryWriter::isWriting() const
    {
        return true;
    }

    void BinaryWriter::value(bool& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(char& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(int8_t& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(uint8_t& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(int16_t& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(uint16_t& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(int32_t& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(uint32_t& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(int64_t& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(uint64_t& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(float& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(double& item)
    {
        mSuccess |= mStream->write(&item, sizeof(item));
    }

    void BinaryWriter::value(std::string& item)
    {
        EMU_ASSERT(item.size() <= UINT32_MAX);
        uint32_t serializedSize = static_cast<uint32_t>(item.size());
        value(serializedSize);
        mSuccess |= mStream->write(item.data(), sizeof(std::string::value_type) * item.size());
    }

    void BinaryWriter::value(Buffer& item)
    {
        EMU_ASSERT(item.size() <= UINT32_MAX);
        uint32_t serializedSize = static_cast<uint32_t>(item.size());
        value(serializedSize);
        mSuccess |= mStream->write(item.data(), item.size());
    }

    bool BinaryWriter::nodeBegin(const char* name)
    {
        EMU_UNUSED(name);
        return true;
    }

    void BinaryWriter::nodeEnd()
    {
    }

    bool BinaryWriter::sequenceBegin(size_t& size)
    {
        EMU_ASSERT(size <= UINT32_MAX);
        uint32_t serializedSize = static_cast<uint32_t>(size);
        value(serializedSize);
        return true;
    }

    void BinaryWriter::sequenceEnd()
    {
    }

    void BinaryWriter::sequenceItem()
    {
    }
}