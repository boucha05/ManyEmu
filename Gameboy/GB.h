#ifndef __GB_H__
#define __GB_H__

#include <Core/Core.h>
#include <Emulator/Emulator.h>

namespace gb
{
    enum class Model : uint8_t
    {
        GB,
        GBC,
        SGB,
    };

    class Rom : public emu::Rom, public emu::IDisposable
    {
    public:
        enum class Mapper : uint8_t
        {
            ROM,
            MBC1,
            MBC2,
            MBC3,
            MBC4,
            MBC5,
            MMM01,
            PocketCamera,
            BandaiTama5,
            HuC1,
            HuC3,
            INVALID,
        };

        enum class Destination : uint8_t
        {
            Japan,
            NonJapan,
            INVALID,
        };

        struct Description
        {
            uint32_t            romSize;
            uint32_t            ramSize;
            char                title[17];
            bool                useCGB;
            bool                onlyCGB;
            bool                useSGB;
            bool                hasRam;
            bool                hasBattery;
            bool                hasTimer;
            bool                hasRumble;
            Mapper              mapper;
            Destination         destination;
            uint8_t             cartridgeType;
            uint8_t             version;
            uint8_t             licenseeOld;
            uint16_t            licenseeNew;
            uint32_t            manufacturer;
            uint8_t             headerChecksum;
            uint16_t            globalChecksum;
        };

        struct Content
        {
            const uint8_t*  rom;
            const uint8_t*  header;
        };

        virtual const Description& getDescription() const = 0;
        virtual const Content& getContent() const = 0;

        static bool readDescription(Rom::Description& description, const char* path);
        static Rom* load(const char* path);
        static const char* getMapperName(Mapper value);
        static const char* getDestinationName(Destination value);
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

        static const uint32_t DisplaySizeX = 160;
        static const uint32_t DisplaySizeY = 144;

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

        static Context* create(emu::IEmulatorAPI& api, const Rom& rom, Model model);
    };

    emu::IEmulator* createEmulatorGB(emu::IEmulatorAPI& api);
    void destroyEmulatorGB(emu::IEmulator& emulator);
    emu::IEmulator* createEmulatorGBC(emu::IEmulatorAPI& api);
    void destroyEmulatorGBC(emu::IEmulator& emulator);
}

#endif
