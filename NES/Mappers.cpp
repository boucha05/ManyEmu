#include "Mappers.h"
#include "Mapper0.h"
#include "Mapper1.h"
#include "Mapper2.h"
#include "Mapper4.h"
#include <map>

namespace
{
    class MapperRegistryImpl : public nes::MapperRegistry
    {
    public:
        virtual void registerMapper(const nes::MapperInfo& info)
        {
            mapperInfoTable[info.index] = &info;
        }

        virtual void unregisterMapper(const nes::MapperInfo& info)
        {
            mapperInfoTable.erase(info.index);
        }

        virtual const char* getMapperName(uint32_t index)
        {
            auto item = mapperInfoTable.find(index);
            if (item == mapperInfoTable.end())
                return nullptr;
            return item->second->name;
        }

        virtual nes::IMapper* create(uint32_t index)
        {
            auto item = mapperInfoTable.find(index);
            if (item == mapperInfoTable.end())
                return nullptr;
            return item->second->create();
        }

    private:
        typedef std::map<uint32_t, const nes::MapperInfo*> MapperInfoTable;

        MapperInfoTable     mapperInfoTable;
    };
}

namespace nes
{
    MapperRegistry& MapperRegistry::getInstance()
    {
        static MapperRegistryImpl instance;
        static nes::AutoRegisterMapper<Mapper0> mapper0(0, "NROM");
        static nes::AutoRegisterMapper<Mapper1> mapper1(1, "SxROM");
        static nes::AutoRegisterMapper<Mapper2> mapper2(2, "UxROM");
        static nes::AutoRegisterMapper<Mapper4> mapper4(4, "MMC3/MMC6");

        return instance;
    }
}
