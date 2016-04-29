#include "EmulatorImpl.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    using namespace emu;

    class ComponentType : public IComponentTypeBuilder
    {
    public:
        virtual IComponent* createComponent() override
        {
            if (mFactory)
                return mFactory->createFunc(mFactory->context);
            return nullptr;
        }

        virtual void destroyComponent(IComponent& component) override
        {
            if (mFactory)
                return mFactory->destroyFunc(mFactory->context, component);
        }

        virtual void* getInterface(IComponent& component, const char* interfaceID) override
        {
            auto item = mInterfaceMap.find(interfaceID);
            if (item == mInterfaceMap.end())
                return nullptr;
            const auto& entry = mInterfaceTable[item->second];
            return entry.func(entry.context, component);
        }

        virtual void setFactory(void* context, CreateFunc createFunc, DestroyFunc destroyFunc) override
        {
            mFactory.reset(new FactoryInfo{ context, createFunc, destroyFunc });
        }

        virtual void addInterface(const char* interfaceID, void* context, GetInterfaceFunc func) override
        {
            InterfaceInfo value = { interfaceID, context, func };
            auto item = mInterfaceMap.find(interfaceID);
            if (item == mInterfaceMap.end())
            {
                mInterfaceMap[interfaceID] = static_cast<uint32_t>(mInterfaceTable.size());
                mInterfaceTable.push_back(value);
            }
            else
            {
                mInterfaceTable[item->second] = value;
            }
        }

    private:
        struct FactoryInfo
        {
            void*               context;
            CreateFunc          createFunc;
            DestroyFunc         destroyFunc;
        };

        struct InterfaceInfo
        {
            std::string         name;
            void*               context;
            GetInterfaceFunc    func;
        };

        typedef std::unique_ptr<FactoryInfo> FactoryPtr;
        typedef std::unordered_map<std::string, uint32_t> StringMap;
        typedef std::vector<InterfaceInfo> InterfaceTable;

        FactoryPtr      mFactory;
        StringMap       mInterfaceMap;
        InterfaceTable  mInterfaceTable;
    };

    class ComponentManager : public IComponentManager
    {
    public:
        virtual IComponentTypeBuilder* createComponentType(const char* componentID) override
        {
            if (!isValidComponentID(componentID))
                return nullptr;
            auto& item = mComponents[componentID];
            item.reset(new ComponentType());
            return item.get();
        }

        virtual void destroyComponentType(const char* componentID)
        {
            if (isValidComponentID(componentID))
                mComponents.erase(componentID);
        }

        virtual IComponentType* getComponentType(const char* componentID)
        {
            if (!isValidComponentID(componentID))
                return nullptr;
            auto item = mComponents.find(componentID);
            if (item == mComponents.end())
                return nullptr;
            return item->second.get();
        }

    private:
        typedef std::unordered_map<std::string, std::unique_ptr<ComponentType>> ComponentDictionary;

        static bool isValidComponentID(const char* componentID)
        {
            return componentID && (componentID[0] != 0);
        }

        ComponentDictionary     mComponents;
    };
}

namespace emu
{
    namespace impl
    {
        IComponentManager* createComponentManager()
        {
            return new ComponentManager();
        }

        void destroyComponentManager(IComponentManager& instance)
        {
            delete static_cast<ComponentManager*>(&instance);
        }
    };
}