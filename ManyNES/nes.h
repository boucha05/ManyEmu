#ifndef __NES_H__
#define __NES_H__

#include <stdint.h>

namespace NES
{
    class IDisposable
    {
    public:
        virtual void dispose() = 0;
    };

    class Rom : public IDisposable
    {
    public:
        enum Mirroring
        {
            Mirroring_Horizontal,
            Mirroring_Vertical,
            Mirroring_FourScreen,
        };

        struct Description
        {
            uint32_t        prgRomPages;
            uint32_t        chrRomPages;
            uint32_t        prgRamPages;
            uint32_t        mapper;
            Mirroring       mirroring;
            bool            battery;
            bool            trainer;
        };

        struct Content
        {
            const uint8_t*  header;
            const uint8_t*  trainer;
            const uint8_t*  prgRom;
            const uint8_t*  chrRom;
        };

        virtual const Description& getDescription() const = 0;
        virtual const Content& getContent() const = 0;

        static bool readDescription(Rom::Description& description, const char* path);
        static Rom* load(const char* path);
    };

    class Context : public IDisposable
    {
    public:
        virtual void reset() = 0;
        virtual void update(void* surface, uint32_t pitch) = 0;
        virtual uint8_t read8(uint16_t addr) = 0;
        virtual void write8(uint16_t addr, uint8_t value) = 0;

        static Context* create(const Rom& rom);
    };
}

#endif
