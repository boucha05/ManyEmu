#include "Mappers.h"
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
        return instance;
    }
}
