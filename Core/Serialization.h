#ifndef __SERIALIZATION_H__
#define __SERIALIZATION_H__

#include "Core.h"
#include <stdint.h>
#include <vector>

namespace emu
{
    class IStream;

    class ISerializer
    {
    public:
        virtual bool isReading() const = 0;
        virtual void serialize(uint32_t& value) = 0;
        virtual void serialize(int32_t& value) = 0;
        virtual void serialize(uint16_t& data) = 0;
        virtual void serialize(uint8_t& value) = 0;
        virtual void serialize(int8_t& value) = 0;
        virtual void serialize(bool& value) = 0;
        virtual void serialize(void* value, size_t size) = 0;
        void serialize(uint32_t* values, size_t size);
        void serialize(uint8_t* values, size_t size);

        template <typename T>
        void serialize(std::vector<T>& value)
        {
            uint32_t size = static_cast<T>(value.size());
            serialize(size);
            value.resize(size, static_cast<T>(0));

            if (size)
                serialize(&value[0], size * sizeof(T));
        }

        bool isWriting() const
        {
            return !isReading();
        }
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
