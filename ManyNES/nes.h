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

        virtual void getDescription(Description& description) const = 0;
        virtual void getContent(Content& content) const = 0;

        static bool readDescription(Rom::Description& description, const char* path);
        static Rom* load(const char* path);
    };

    class Context : public IDisposable
    {
    public:
        virtual void update(void* surface, uint32_t pitch) = 0;

        static Context* create(const Rom& rom);
    };
}

#endif
