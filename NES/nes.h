#ifndef __NES_H__
#define __NES_H__

#include <Core/Core.h>
#include <Core/Context.h>
#include <Core/Emulator.h>
#include <Core/Rom.h>

namespace nes
{
    class Rom : public emu::IRom, public emu::IDisposable
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

    class Context : public emu::IContext, public emu::IDisposable
    {
    public:
        static const uint32_t ButtonA = 0x01;
        static const uint32_t ButtonB = 0x02;
        static const uint32_t ButtonSelect = 0x04;
        static const uint32_t ButtonStart = 0x08;
        static const uint32_t ButtonUp = 0x10;
        static const uint32_t ButtonDown = 0x20;
        static const uint32_t ButtonLeft = 0x40;
        static const uint32_t ButtonRight = 0x80;

        static const uint32_t DisplaySizeX = 256;
        static const uint32_t DisplaySizeY = 224;

        static constexpr float getDisplayFPS_NTSC() { return 60.0988f; }
        static constexpr float getDisplayFPS_PAL() { return 50.0070f; }

        virtual uint8_t read8(uint16_t addr) = 0;
        virtual void write8(uint16_t addr, uint8_t value) = 0;

        static Context* create(const Rom& rom);
    };
}

#endif
