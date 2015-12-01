#ifndef __MAPPERS_H__
#define __MAPPERS_H__

#include "nes.h"
#include <stdint.h>

namespace emu
{
    class Clock;
    class MemoryBus;
    class ISerializer;
}

namespace nes
{
    class APU;
    class Cpu6502;
    class PPU;

    class IMapper : public emu::IDisposable
    {
    public:
        class IListener
        {
        public:
            virtual void onIrqUpdate(bool active) {}
        };

        struct Components
        {
            const Rom*          rom;
            emu::Clock*         clock;
            emu::MemoryBus*     memory;
            Cpu6502*            cpu;
            PPU*                ppu;
            APU*                apu;
            IListener*          listener;
        };
        virtual bool initialize(const Components& components) = 0;
        virtual void reset() { }
        virtual void beginFrame() { }
        virtual void update() { }
        virtual void serializeGameData(emu::ISerializer& serializer) { }
        virtual void serializeGameState(emu::ISerializer& serializer) { }
    };

    typedef IMapper* (*MapperCreateFunc)();

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
        virtual IMapper* create(uint32_t index) = 0;

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
        static IMapper* createMapper()
        {
            return new T;
        }

        MapperInfo  info;
    };
}

#endif
