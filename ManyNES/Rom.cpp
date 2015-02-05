#include "nes.h"
#include <stdint.h>
#include <stdio.h>
#include <algorithm>
#include <vector>

namespace
{
    static const size_t HEADER_SIZE = 16;
    static const size_t TRAINER_SIZE = 512;
    static const size_t PRG_ROM_PAGE_SIZE = 16384;
    static const size_t CHR_ROM_PAGE_SIZE = 8192;

    static const size_t FLAGS6_VERTICAL_MIRRORING = 0x01;
    static const size_t FLAGS6_BATTERY = 0x02;
    static const size_t FLAGS6_TRAINER = 0x04;
    static const size_t FLAGS6_FOUR_SCREEN_VRAM = 0x08;
    static const size_t FLAGS6_MAPPER_LOWER_MASK = 0xf0;
    static const size_t FLAGS6_MAPPER_LOWER_SHIFT = 4;

    static const size_t FLAGS7_MAPPER_UPPER_MASK = 0xf0;
    static const size_t FLAGS7_MAPPER_UPPER_SHIFT = 0;

    bool readFile(std::vector<char>& buffer, const char* path, size_t maxSize = SIZE_MAX)
    {
        FILE* file = fopen(path, "rb");
        if (!file)
            return false;

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

    class RomImpl : public NES::Rom
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
            if (size == 0)
                return false;

            data.resize(size);
            memcpy(&data[0], buffer, size);

            const uint8_t* pos = &data[0];
            const uint8_t* end = pos + size;

            // Get header
            content.header = pos;
            if ((pos += HEADER_SIZE) > end)
                return false;
            getDescription(description, &data[0], HEADER_SIZE);

            // Get trainer
            if (description.trainer)
            {
                content.trainer = pos;
                if ((pos += TRAINER_SIZE) > end)
                    return false;
            }

            // Get PRG-ROM
            content.prgRom = pos;
            if ((pos += description.prgRomPages * PRG_ROM_PAGE_SIZE) > end)
                return false;

            // Get CHR-ROM
            content.chrRom = pos;
            if ((pos += description.chrRomPages * CHR_ROM_PAGE_SIZE) > end)
                return false;

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

            if ((size < HEADER_SIZE) || (data[0] != 'N') || (data[1] != 'E') || (data[2] != 'S') || (data[3] != 0x1a))
                return false;

            uint8_t header[HEADER_SIZE];
            memcpy(header, data, size);
            if (memcmp(&header[7], "DiskDude!", 9) == 0)
                memset(&header[7], 0, 9);

            description.prgRomPages = header[4];
            description.chrRomPages = header[5];
            description.prgRamPages = header[8];

            uint8_t flags6 = header[6];
            uint8_t flags7 = header[7];
            description.battery = (flags6 & FLAGS6_BATTERY) != 0;
            description.trainer = (flags6 & FLAGS6_TRAINER) != 0;
            description.mirroring = (flags6 & FLAGS6_FOUR_SCREEN_VRAM) ? Mirroring_FourScreen :
                (flags6 & FLAGS6_VERTICAL_MIRRORING) ? Mirroring_Vertical : Mirroring_Horizontal;
            description.mapper = ((flags6 & FLAGS6_MAPPER_LOWER_MASK) >> FLAGS6_MAPPER_LOWER_SHIFT) |
                ((flags7 & FLAGS7_MAPPER_UPPER_MASK) >> FLAGS7_MAPPER_UPPER_SHIFT);

            return true;
        }

    private:
        std::vector<uint8_t>    data;
        Description             description;
        Content                 content;
    };
}

namespace NES
{
    bool Rom::readDescription(Rom::Description& description, const char* path)
    {
        std::vector<char> buffer;
        if (!readFile(buffer, path, HEADER_SIZE))
            return nullptr;

        return RomImpl::getDescription(description, reinterpret_cast<const uint8_t*>(&buffer[0]), buffer.size());
    }

    Rom* Rom::load(const char* path)
    {
        std::vector<char> buffer;
        if (!readFile(buffer, path))
            return nullptr;

        RomImpl* rom = new RomImpl;
        if (!rom->create(&buffer[0], buffer.size()))
        {
            rom->dispose();
            rom = nullptr;
        }
        return rom;
    }
}
