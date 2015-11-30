#include "Mappers.h"
#include <map>

namespace
{
    class MapperRegistryImpl : public NES::MapperRegistry
    {
    public:
        virtual void registerMapper(const NES::MapperInfo& info)
        {
            mapperInfoTable[info.index] = &info;
        }

        virtual void unregisterMapper(const NES::MapperInfo& info)
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

        virtual NES::IMapper* create(uint32_t index)
        {
            auto item = mapperInfoTable.find(index);
            if (item == mapperInfoTable.end())
                return nullptr;
            return item->second->create();
        }

    private:
        typedef std::map<uint32_t, const NES::MapperInfo*> MapperInfoTable;

        MapperInfoTable     mapperInfoTable;
    };
}

namespace NES
{
    MapperRegistry& MapperRegistry::getInstance()
    {
        static MapperRegistryImpl instance;
        return instance;
    }
}
