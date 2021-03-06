#ifndef __GB_H__
#define __GB_H__

#include <Core/Core.h>
#include <Core/Context.h>
#include <Core/Emulator.h>
#include <Core/Rom.h>

namespace gb
{
    enum class Model : uint8_t
    {
        GB,
        GBC,
        SGB,
    };

    class Rom : public emu::IRom, public emu::IDisposable
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
        static const uint32_t ButtonAll = 0xff;

        static const uint32_t DisplaySizeX = 160;
        static const uint32_t DisplaySizeY = 144;

        virtual uint8_t read8(uint16_t addr) = 0;
        virtual void write8(uint16_t addr, uint8_t value) = 0;

        static Context* create(const Rom& rom, Model model);
    };

    class Emulator : public emu::IEmulator
    {
    public:
        static Emulator& getEmulatorGB();
        static Emulator& getEmulatorGBC();
        static Emulator& getEmulatorSGB();
    };
}

#endif
