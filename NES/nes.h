#ifndef __NES_H__
#define __NES_H__

#include <Core/Core.h>
#include <Emulator/Emulator.h>

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
        enum class Button : uint32_t
        {
            A, B, Select, Start, Up, Down, Left, Right, COUNT
        };
        static const uint32_t ButtonA = 1 << static_cast<uint32_t>(Button::A);
        static const uint32_t ButtonB = 1 << static_cast<uint32_t>(Button::B);
        static const uint32_t ButtonSelect = 1 << static_cast<uint32_t>(Button::Select);
        static const uint32_t ButtonStart = 1 << static_cast<uint32_t>(Button::Start);
        static const uint32_t ButtonUp = 1 << static_cast<uint32_t>(Button::Up);
        static const uint32_t ButtonDown = 1 << static_cast<uint32_t>(Button::Down);
        static const uint32_t ButtonLeft = 1 << static_cast<uint32_t>(Button::Left);
        static const uint32_t ButtonRight = 1 << static_cast<uint32_t>(Button::Right);
        static const uint32_t ButtonAll = (1 << static_cast<uint32_t>(Button::COUNT)) - 1;

        static const uint32_t DisplaySizeX = 256;
        static const uint32_t DisplaySizeY = 224;

        virtual void reset() = 0;
        virtual uint32_t getInputDeviceCount() = 0;
        virtual emu::IInputDevice* getInputDevice(uint32_t index) = 0;
        virtual void setSoundBuffer(int16_t* buffer, size_t size) = 0;
        virtual void setRenderSurface(void* surface, size_t pitch) = 0;
        virtual void update() = 0;
        virtual uint8_t read8(uint16_t addr) = 0;
        virtual void write8(uint16_t addr, uint8_t value) = 0;
        virtual void serializeGameData(emu::ISerializer& serializer) = 0;
        virtual void serializeGameState(emu::ISerializer& serializer) = 0;

        static Context* create(emu::IEmulatorAPI& api, const Rom& rom);
    };
}

#endif
