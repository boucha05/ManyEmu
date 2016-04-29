#ifndef __CORE_H__
#define __CORE_H__

#define EMU_CONFIG_LITTLE_ENDIAN    1

#include <stdint.h>
#include <vector>
#if defined(DEBUG) || defined(_DEBUG)
#include <assert.h>
#define EMU_ASSERT(e)   emu::Assert(!!(e), #e)
#else
#define EMU_ASSERT(e)
#endif
#define EMU_VERIFY(e)   if (e) ; else return false

#define EMU_INVOKE_ONCE(e)                  \
{                                           \
    static bool __invoked_before = false;   \
    if (!__invoked_before)                  \
    {                                       \
        __invoked_before = true;            \
        e;                                  \
    }                                       \
}

#define EMU_ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))

#if EMU_CONFIG_LITTLE_ENDIAN
#define _EMU_HALF_0                 l
#define _EMU_HALF_1                 h
#define _EMU_LITTLE_ENDIAN(expr)    (expr)
#define _EMU_BIG_ENDIAN(expr)       emu::swap(expr)
#else
#define _EMU_HALF_0                 h
#define _EMU_HALF_1                 l
#define _EMU_LITTLE_ENDIAN(expr)    emu::swap(expr)
#define _EMU_BIG_ENDIAN(expr)       (expr)
#endif

namespace emu
{
    union word8_t
    {
        int8_t          i;
        uint8_t         u;
    };

    union word16_t
    {
        int16_t         i;
        uint16_t        u;
        word8_t         w8[2];
        struct
        {
            word8_t     _EMU_HALF_0;
            word8_t     _EMU_HALF_1;
        }               w;
    };

    union word32_t
    {
        int32_t         i;
        uint32_t        u;
        float           f;
        word8_t         w8[4];
        word16_t        w16[2];
        struct
        {
            word16_t    _EMU_HALF_0;
            word16_t    _EMU_HALF_1;
        }               w;
    };

    union word64_t
    {
        int64_t         i;
        uint64_t        u;
        double          f;
        word8_t         w8[8];
        word16_t        w16[4];
        word32_t        w32[2];
        struct
        {
            word32_t    _EMU_HALF_0;
            word32_t    _EMU_HALF_1;
        }               w;
    };

    inline word16_t swap(word16_t value)
    {
        word16_t result;
        result.w.h = value.w.l;
        result.w.l = value.w.h;
        return result;
    }

    inline word32_t swap(word32_t value)
    {
        word32_t result;
        result.w.h = swap(value.w.l);
        result.w.l = swap(value.w.h);
        return result;
    }

    inline word64_t swap(word64_t value)
    {
        word64_t result;
        result.w.h = swap(value.w.l);
        result.w.l = swap(value.w.h);
        return result;
    }

#define _EMU_ENDIAN_CONVERTERS(type, word)                                  \
    inline type little_endian(type value)                                   \
    {                                                                       \
        word result = _EMU_LITTLE_ENDIAN(reinterpret_cast<word&>(value));   \
        return reinterpret_cast<type&>(result);                             \
    }                                                                       \
                                                                            \
    inline type big_endian(type value)                                      \
    {                                                                       \
        word result =_EMU_BIG_ENDIAN(reinterpret_cast<word&>(value));       \
        return reinterpret_cast<type&>(result);                             \
    }

    _EMU_ENDIAN_CONVERTERS(int16_t, emu::word16_t);
    _EMU_ENDIAN_CONVERTERS(uint16_t, emu::word16_t);
    _EMU_ENDIAN_CONVERTERS(emu::word16_t, emu::word16_t);
    _EMU_ENDIAN_CONVERTERS(int32_t, emu::word32_t);
    _EMU_ENDIAN_CONVERTERS(uint32_t, emu::word32_t);
    _EMU_ENDIAN_CONVERTERS(float, emu::word32_t);
    _EMU_ENDIAN_CONVERTERS(emu::word32_t, emu::word32_t);
    _EMU_ENDIAN_CONVERTERS(int64_t, emu::word64_t);
    _EMU_ENDIAN_CONVERTERS(uint64_t, emu::word64_t);
    _EMU_ENDIAN_CONVERTERS(double, emu::word64_t);
    _EMU_ENDIAN_CONVERTERS(emu::word64_t, emu::word64_t);

    void Assert(bool valid, const char* msg);
    void notImplemented(const char* function);

#define EMU_NOT_IMPLEMENTED()   EMU_INVOKE_ONCE(emu::notImplemented(__FUNCTION__))

    class IStream
    {
    public:
        virtual bool read(void* data, size_t size) = 0;
        virtual bool write(const void* data, size_t size) = 0;
    };

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
        
        void serialize(uint32_t* values, size_t size)
        {
            for (size_t index = 0; index < size; ++index)
                serialize(*values++);
        }

        void serialize(uint8_t* values, size_t size)
        {
            for (size_t index = 0; index < size; ++index)
                serialize(*values++);
        }

        template <typename T>
        void serialize(std::vector<T>& value)
        {
            uint32_t size = static_cast<uint32_t>(value.size());
            serialize(size);
            value.resize(size, static_cast<T>(0));

            if (size)
                serialize(&value[0], size);
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

    class IDisposable
    {
    public:
        virtual void dispose() = 0;
    };

    struct Rom
    {
    };

    struct Context
    {
    };

    class IInputDevice;

    class IEmulator
    {
    public:
        virtual Rom* loadRom(const char* path) = 0;
        virtual void unloadRom(Rom& rom) = 0;
        virtual Context* createContext(const Rom& rom) = 0;
        virtual void destroyContext(Context& context) = 0;
        virtual bool getDisplaySize(emu::Context& context, uint32_t& sizeX, uint32_t& sizeY) = 0;
        virtual bool serializeGameData(Context& context, emu::ISerializer& serializer) = 0;
        virtual bool serializeGameState(Context& context, emu::ISerializer& serializer) = 0;
        virtual bool setRenderBuffer(Context& context, void* buffer, size_t pitch) = 0;
        virtual bool setSoundBuffer(Context& context, void* buffer, size_t size) = 0;
        virtual uint32_t getInputDeviceCount(Context& context) = 0;
        virtual IInputDevice* getInputDevice(Context& context, uint32_t index) = 0;
        virtual bool reset(Context& context) = 0;
        virtual bool execute(Context& context) = 0;
    };
}

#endif
