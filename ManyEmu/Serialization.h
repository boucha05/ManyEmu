#ifndef __SERIALIZATION_H__
#define __SERIALIZATION_H__

#include <Core/Core.h>
#include <stdint.h>
#include <vector>

namespace emu
{
    class BinaryReader : public IArchive
    {
    public:
        BinaryReader(IStream& stream);
        ~BinaryReader();
        virtual bool isReading() const;
        virtual void serialize(uint32_t& data);
        virtual void serialize(int32_t& data);
        virtual void serialize(uint16_t& data);
        virtual void serialize(uint8_t& data);
        virtual void serialize(int8_t& data);
        virtual void serialize(bool& value);
        virtual void serialize(void* data, size_t size);
        virtual IStream& getStream()
        {
            return *mStream;
        }

    private:
        IStream*    mStream;

        void serializeSafe(void* data, size_t size);
    };

    class BinaryWriter : public IArchive
    {
    public:
        BinaryWriter(IStream& stream);
        ~BinaryWriter();
        virtual bool isReading() const;
        virtual void serialize(uint32_t& data);
        virtual void serialize(int32_t& data);
        virtual void serialize(uint16_t& data);
        virtual void serialize(uint8_t& data);
        virtual void serialize(int8_t& data);
        virtual void serialize(bool& value);
        virtual void serialize(void* data, size_t size);
        virtual IStream& getStream()
        {
            return *mStream;
        }

    private:
        IStream*    mStream;
    };

    class TextWriter : public IArchive
    {
    public:
        TextWriter(IStream& stream);
        ~TextWriter();
        virtual bool isReading() const;
        virtual void serialize(uint32_t& data);
        virtual void serialize(int32_t& data);
        virtual void serialize(uint16_t& data);
        virtual void serialize(uint8_t& data);
        virtual void serialize(int8_t& data);
        virtual void serialize(bool& value);
        virtual void serialize(void* data, size_t size);
        virtual void write(const char* text, ...);
        virtual IStream& getStream()
        {
            return *mStream;
        }

    private:
        IStream*    mStream;
    };
}

#endif
