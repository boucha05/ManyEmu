#ifndef __SERIALIZATION_H__
#define __SERIALIZATION_H__

#include <stdint.h>
#include "nes.h"

namespace NES
{
    class IStream;

    class ISerializer
    {
    public:
        virtual void serialize(uint32_t& value) = 0;
        virtual void serialize(uint8_t& value) = 0;
        virtual void serialize(void* value, size_t size) = 0;
    };

    class IArchive : public ISerializer
    {
    public:
        virtual IStream& getStream() = 0;
    };

    class BinaryReader : public IArchive
    {
    public:
        BinaryReader(IStream& stream);
        ~BinaryReader();
        virtual void serialize(uint32_t& data);
        virtual void serialize(uint8_t& data);
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
        virtual void serialize(uint32_t& data);
        virtual void serialize(uint8_t& data);
        virtual void serialize(void* data, size_t size);
        virtual IStream& getStream()
        {
            return *mStream;
        }

    private:
        IStream*    mStream;
    };
}

#endif
