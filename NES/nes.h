#ifndef __NES_H__
#define __NES_H__

#include <Core/Core.h>

namespace nes
{
    class Rom : public emu::Rom, public emu::IDisposable
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

    class Context : public emu::Context, public emu::IDisposable
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

        virtual void reset() = 0;
        virtual void setController(uint32_t index, uint32_t buttons) = 0;
        virtual void setSoundBuffer(int16_t* buffer, size_t size) = 0;
        virtual void setRenderSurface(void* surface, size_t pitch) = 0;
        virtual void update() = 0;
        virtual uint8_t read8(uint16_t addr) = 0;
        virtual void write8(uint16_t addr, uint8_t value) = 0;
        virtual void serializeGameData(emu::ISerializer& serializer) = 0;
        virtual void serializeGameState(emu::ISerializer& serializer) = 0;

        static Context* create(const Rom& rom);
    };
}

#endif
