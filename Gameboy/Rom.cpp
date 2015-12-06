#include "GB.h"
#include <stdint.h>
#include <stdio.h>
#include <algorithm>
#include <vector>

namespace
{
    struct Header
    {
        uint32_t    EntryPoint;
        char        NintendoLogo[0x30];
        char        Title[0xc];
        uint16_t    ManufacturerCode;
        uint8_t     CGBFlag;
        uint16_t    NewLicenseeCode;
        uint8_t     SGBFlag;
        uint8_t     CartridgeType;
        uint8_t     ROMSize;
        uint8_t     RAMSize;
        uint8_t     DestinationCode;
        uint8_t     OldLicenseeCode;
        uint8_t     MaskROMVersionNumber;
        uint8_t     HeaderChecksum;
        uint16_t    GlobalChecksum;
    };

    static const uint32_t HEADER_OFFSET = 0x100;
    static const uint32_t HEADER_SIZE = 0x50;

    static const uint32_t ROM_BANK_SIZE = 0x2000;

    bool readFile(std::vector<char>& buffer, const char* path, size_t maxSize = SIZE_MAX)
    {
        FILE* file = fopen(path, "rb");
        EMU_VERIFY(file);

        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        rewind(file);
        size = std::min(size, maxSize);

        buffer.clear();
        if (size)
        {
            buffer.resize(size, 0);
            size = fread(&buffer[0], 1, size, file);
            buffer.resize(size);
        }
        fclose(file);

        return size > 0;
    }

    class RomImpl : public gb::Rom
    {
    public:
        RomImpl()
        {
            reset();
        }

        ~RomImpl()
        {
            destroy();
        }

        void reset()
        {
            memset(&description, 0, sizeof(description));
            memset(&content, 0, sizeof(content));
        }

        bool create(const char* buffer, uint32_t size)
        {
            EMU_VERIFY(size);

            data.resize(size);
            memcpy(&data[0], buffer, size);

            // Get content
            const uint8_t* pos = &data[0];
            const uint8_t* end = pos + size;
            content.rom = pos;
            EMU_VERIFY((pos += HEADER_OFFSET) <= end);
            content.header = pos;

            // Get header
            getDescription(description, content.rom, size);

            return true;
        }

        void destroy()
        {
            data.clear();
            reset();
        }

        virtual void dispose()
        {
            delete this;
        }

        virtual const Description& getDescription() const
        {
            return description;
        }

        virtual const Content& getContent() const
        {
            return content;
        }

        static bool getDescription(Description& description, const uint8_t* data, size_t size)
        {
            memset(&description, 0, sizeof(description));

            EMU_VERIFY(HEADER_OFFSET + HEADER_SIZE <= size);

            const auto& header = *reinterpret_cast<const Header*>(data + HEADER_OFFSET);

            switch (header.ROMSize)
            {
            case 0x00: description.romSize = 0x008000; break;
            case 0x01: description.romSize = 0x010000; break;
            case 0x02: description.romSize = 0x020000; break;
            case 0x03: description.romSize = 0x040000; break;
            case 0x04: description.romSize = 0x080000; break;
            case 0x05: description.romSize = 0x100000; break;
            case 0x06: description.romSize = 0x200000; break;
            case 0x07: description.romSize = 0x400000; break;
            case 0x52: description.romSize = 0x120000; break;
            case 0x53: description.romSize = 0x140000; break;
            case 0x54: description.romSize = 0x180000; break;
            default: EMU_VERIFY(false);
            }

            switch (header.RAMSize)
            {
            case 0x00: description.ramSize = 0x0000; break;
            case 0x01: description.ramSize = 0x0800; break;
            case 0x02: description.ramSize = 0x2000; break;
            case 0x03: description.ramSize = 0x8000; break;
            default: EMU_VERIFY(false);
            }

            memcpy(description.title, header.Title, 16);
            description.manufacturer = emu::little_endian(header.ManufacturerCode);
            description.useCGB = (header.CGBFlag & 0x80) != 0;
            description.onlyCGB = (header.CGBFlag & 0x40) != 0;
            description.useSGB = header.SGBFlag == 0x03;
            description.cartridgeType = header.CartridgeType;

            switch (header.CartridgeType)
            {
            case 0x00: description.mapper = Mapper::ROM; break;
            case 0x01: description.mapper = Mapper::MBC1; break;
            case 0x02: description.mapper = Mapper::MBC1; description.hasRam = true; break;
            case 0x03: description.mapper = Mapper::MBC1; description.hasRam = true; description.hasBattery = true; break;
            case 0x04: description.mapper = Mapper::MBC2; break;
            case 0x05: description.mapper = Mapper::MBC2; description.hasBattery = true; break;
            case 0x08: description.mapper = Mapper::ROM; description.hasRam = true; break;
            case 0x09: description.mapper = Mapper::ROM; description.hasRam = true; description.hasBattery = true; break;
            case 0x0b: description.mapper = Mapper::MMM01; break;
            case 0x0c: description.mapper = Mapper::MMM01; description.hasRam = true; break;
            case 0x0d: description.mapper = Mapper::MMM01; description.hasRam = true; description.hasBattery = true; break;
            case 0x0f: description.mapper = Mapper::MBC3; description.hasTimer = true; description.hasBattery = true; break;
            case 0x10: description.mapper = Mapper::MBC3; description.hasTimer = true; description.hasRam = true; description.hasBattery = true; break;
            case 0x11: description.mapper = Mapper::MBC3; break;
            case 0x12: description.mapper = Mapper::MBC3; description.hasRam = true; break;
            case 0x13: description.mapper = Mapper::MBC3; description.hasRam = true; description.hasBattery = true; break;
            case 0x15: description.mapper = Mapper::MBC4; break;
            case 0x16: description.mapper = Mapper::MBC4; description.hasRam = true; break;
            case 0x17: description.mapper = Mapper::MBC4; description.hasRam = true; description.hasBattery = true; break;
            case 0x19: description.mapper = Mapper::MBC5; break;
            case 0x1a: description.mapper = Mapper::MBC5; description.hasRam = true; break;
            case 0x1b: description.mapper = Mapper::MBC5; description.hasRam = true; description.hasBattery = true; break;
            case 0x1c: description.mapper = Mapper::MBC5; description.hasRumble = true; break;
            case 0x1d: description.mapper = Mapper::MBC5; description.hasRumble = true; description.hasRam = true; break;
            case 0x1e: description.mapper = Mapper::MBC5; description.hasRumble = true; description.hasRam = true; description.hasBattery = true; break;
            case 0xfc: description.mapper = Mapper::PocketCamera; break;
            case 0xfd: description.mapper = Mapper::BandaiTama5; break;
            case 0xfe: description.mapper = Mapper::HuC3; break;
            case 0xff: description.mapper = Mapper::HuC3; description.hasRam = true; description.hasBattery = true; break;
            default: EMU_VERIFY(false);
            }

            switch (header.DestinationCode)
            {
            case 0x00: description.destination = Destination::Japan; break;
            case 0x01: description.destination = Destination::NonJapan; break;
            default: EMU_VERIFY(false);
            }

            description.version = header.MaskROMVersionNumber;
            description.licenseeOld = header.OldLicenseeCode;
            description.licenseeNew = emu::little_endian(header.NewLicenseeCode);
            description.manufacturer = emu::little_endian(header.ManufacturerCode);
            description.headerChecksum = header.HeaderChecksum;
            description.globalChecksum = emu::little_endian(header.GlobalChecksum);
            return true;
        }

    private:
        std::vector<uint8_t>    data;
        Description             description;
        Content                 content;
    };
}

namespace gb
{
    bool Rom::readDescription(Rom::Description& description, const char* path)
    {
        std::vector<char> buffer;
        EMU_VERIFY(!readFile(buffer, path, HEADER_OFFSET + HEADER_SIZE));
        EMU_VERIFY(RomImpl::getDescription(description, reinterpret_cast<const uint8_t*>(&buffer[HEADER_OFFSET]), buffer.size()));
        return true;
    }

    Rom* Rom::load(const char* path)
    {
        std::vector<char> buffer;
        if (!readFile(buffer, path))
            return nullptr;

        RomImpl* rom = new RomImpl;
        if (!rom->create(&buffer[0], static_cast<uint32_t>(buffer.size())))
        {
            rom->dispose();
            rom = nullptr;
        }
        return rom;
    }
}
