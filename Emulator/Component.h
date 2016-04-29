#pragma once

#include <stdint.h>

namespace emu
{
    class IComponent;

    class IComponentType
    {
    public:
        virtual IComponent* createComponent() = 0;
        virtual void destroyComponent(IComponent& component) = 0;
        virtual void* getInterface(IComponent& component, const char* interfaceID) = 0;
    };

    class IComponentTypeBuilder : public IComponentType
    {
    public:
        typedef IComponent* (*CreateFunc)(void* context);
        typedef void (*DestroyFunc)(void* context, IComponent& component);
        typedef void*(*GetInterfaceFunc)(void* context, IComponent& component);

        virtual void setFactory(void* context, CreateFunc createFunc, DestroyFunc destroyFunc) = 0;
        virtual void addInterface(const char* interfaceID, void* context, GetInterfaceFunc func) = 0;
    };

    class IComponentManager
    {
    public:
        virtual IComponentTypeBuilder* createComponentType(const char* componentID) = 0;
        virtual void destroyComponentType(const char* componentID) = 0;
        virtual IComponentType* getComponentType(const char* componentID) = 0;
    };
}
