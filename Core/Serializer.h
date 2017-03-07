#ifndef __SERIALIZATION_H__
#define __SERIALIZATION_H__

#include "CollectionTraits.h"
#include "Core.h"
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

namespace emu
{
    class IStream;

    class ISerializer
    {
    public:
        virtual ~ISerializer() {}
        virtual bool success() const = 0;
        virtual bool isWriting() const = 0;
        virtual void value(bool& item) = 0;
        virtual void value(char& item) = 0;
        virtual void value(int8_t& item) = 0;
        virtual void value(uint8_t& item) = 0;
        virtual void value(int16_t& item) = 0;
        virtual void value(uint16_t& item) = 0;
        virtual void value(int32_t& item) = 0;
        virtual void value(uint32_t& item) = 0;
        virtual void value(int64_t& item) = 0;
        virtual void value(uint64_t& item) = 0;
        virtual void value(float& item) = 0;
        virtual void value(double& item) = 0;
        virtual void value(std::string& item) = 0;
        virtual void value(Buffer& item) = 0;
        virtual bool nodeBegin(const char* name) = 0;
        virtual void nodeEnd() = 0;
        virtual bool sequenceBegin(size_t& size) = 0;
        virtual void sequenceEnd() = 0;
        virtual void sequenceItem() = 0;

        bool isReading() const
        {
            return !isWriting();
        }

        template <typename T>
        ISerializer& value(const char* name, T& value)
        {
            if (nodeBegin(name))
            {
                serialize(*this, value);
                nodeEnd();
            }
            return *this;
        }

        template <typename T>
        ISerializer& sequence(T& item)
        {
            if (isReading())
            {
                CollectionTraits<T>::clear(item);
                size_t count = 0;
                if (sequenceBegin(count))
                {
                    CollectionTraits<T>::reserve(item, count);
                    for (size_t n = 0; n < count; ++n)
                    {
                        sequenceItem();
                        CollectionTraits<T>::ElementType element;
                        serialize(*this, element);
                        CollectionTraits<T>::push_back(item, element, count);
                    }
                    sequenceEnd();
                }
            }
            else
            {
                auto count = CollectionTraits<T>::size(item);
                if (sequenceBegin(count))
                {
                    for (auto element = CollectionTraits<T>::begin(item); element != CollectionTraits<T>::end(item); ++element)
                    {
                        sequenceItem();
                        serialize(*this, *element);
                    }
                    sequenceEnd();
                }
            }
            return *this;
        }
    };

    class IStreamSerializer : public ISerializer
    {
    public:
        virtual IStream& getStream() = 0;
    };

    class BinaryReader : public IStreamSerializer
    {
    public:
        BinaryReader(IStream& stream);
        virtual IStream& getStream() override;
        virtual bool success() const override;
        virtual bool isWriting() const override;
        virtual void value(bool& item) override;
        virtual void value(char& item) override;
        virtual void value(int8_t& item) override;
        virtual void value(uint8_t& item) override;
        virtual void value(int16_t& item) override;
        virtual void value(uint16_t& item) override;
        virtual void value(int32_t& item) override;
        virtual void value(uint32_t& item) override;
        virtual void value(int64_t& item) override;
        virtual void value(uint64_t& item) override;
        virtual void value(float& item) override;
        virtual void value(double& item) override;
        virtual void value(std::string& item) override;
        virtual void value(Buffer& item) override;
        virtual bool nodeBegin(const char* name) override;
        virtual void nodeEnd() override;
        virtual bool sequenceBegin(size_t& size) override;
        virtual void sequenceEnd() override;
        virtual void sequenceItem() override;

    private:
        IStream*     mStream;
        bool        mSuccess;
    };

    class BinaryWriter : public IStreamSerializer
    {
    public:
        BinaryWriter(IStream& stream);
        virtual IStream& getStream() override;
        virtual bool success() const override;
        virtual bool isWriting() const override;
        virtual void value(bool& item) override;
        virtual void value(char& item) override;
        virtual void value(int8_t& item) override;
        virtual void value(uint8_t& item) override;
        virtual void value(int16_t& item) override;
        virtual void value(uint16_t& item) override;
        virtual void value(int32_t& item) override;
        virtual void value(uint32_t& item) override;
        virtual void value(int64_t& item) override;
        virtual void value(uint64_t& item) override;
        virtual void value(float& item) override;
        virtual void value(double& item) override;
        virtual void value(std::string& item) override;
        virtual void value(Buffer& item) override;
        virtual bool nodeBegin(const char* name) override;
        virtual void nodeEnd() override;
        virtual bool sequenceBegin(size_t& size) override;
        virtual void sequenceEnd() override;
        virtual void sequenceItem() override;

    private:
        IStream*    mStream;
        bool        mSuccess;
    };

    template <typename T>
    class has_serialize_member
    {
    private:
        typedef char yes_type;
        typedef int no_type;
        template <typename C>
        static yes_type test(decltype(&C::serialize)) { return 0; }

        template <typename C>
        static no_type test(...) { return 0; }
    public:
        static constexpr bool value = sizeof(decltype(test<T>(0))) == sizeof(yes_type);
    };

    template <typename T>
    std::enable_if_t<std::is_class<T>::value && has_serialize_member<T>::value>
        serialize(ISerializer& serializer, T& item)
    {
        item.serialize(serializer);
    }

    inline void serialize(ISerializer& serializer, bool& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, char& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, int8_t& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, uint8_t& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, int16_t& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, uint16_t& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, int32_t& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, uint32_t& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, int64_t& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, uint64_t& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, float& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, double& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, std::string& item)
    {
        serializer.value(item);
    }

    inline void serialize(ISerializer& serializer, Buffer& item)
    {
        serializer.value(item);
    }

    template <typename T>
    std::enable_if_t<std::is_enum<T>::value>
        serialize(ISerializer& serializer, T& item)
    {
        typedef typename std::underlying_type<T>::type integral_type;
        integral_type& value = reinterpret_cast<integral_type&>(item);
        serializer.value(value);
    }

    template <typename T>
    std::enable_if_t<CollectionTraits<T>::value && std::is_same<typename CollectionTraits<T>::CollectionType, T>::value>
        serialize(ISerializer& serializer, T& item)
    {
        serializer.sequence(item);
    }
}

#endif