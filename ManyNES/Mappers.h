#ifndef __MAPPERS_H__
#define __MAPPERS_H__

#include "nes.h"
#include <stdint.h>

namespace NES
{
    class APU;
    class Cpu6502;
    class MemoryBus;
    class PPU;

    class Mapper : public IDisposable
    {
    public:
        struct Components
        {
            const Rom*  rom;
            MemoryBus*  memory;
            Cpu6502*    cpu;
            PPU*        ppu;
            APU*        apu;
        };
        virtual bool initialize(const Components& components) = 0;
        virtual void reset() { }
        virtual void update() { }
    };

    typedef Mapper* (*MapperCreateFunc)();

    struct MapperInfo
    {
        uint32_t            index;
        const char*         name;
        MapperCreateFunc    create;
    };

    class MapperRegistry
    {
    public:
        virtual void registerMapper(const MapperInfo& info) = 0;
        virtual void unregisterMapper(const MapperInfo& info) = 0;
        virtual const char* getMapperName(uint32_t index) = 0;
        virtual Mapper* create(uint32_t index) = 0;

        static MapperRegistry& getInstance();
    };

    template <typename T>
    class AutoRegisterMapper
    {
    public:
        AutoRegisterMapper(uint32_t index, const char* name)
        {
            info.index = index;
            info.name = name;
            info.create = &createMapper;

            MapperRegistry::getInstance().registerMapper(info);
        }

        ~AutoRegisterMapper()
        {
            MapperRegistry::getInstance().unregisterMapper(info);
        }

    private:
        static Mapper* createMapper()
        {
            return new T;
        }

        MapperInfo  info;
    };
}

#endif
