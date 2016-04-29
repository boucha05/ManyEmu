#include "EmulatorImpl.h"
#include "InputDevice.h"
#include <Core/Core.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace
{
    class InputDevice : public emu::IInputDevice, public emu::IInputDeviceMapping
    {
    public:
        virtual void addListener(IListener& listener)
        {
            mListeners.push_back(&listener);
        }

        virtual void removeListener(IListener& listener)
        {
            std::remove(mListeners.begin(), mListeners.end(), &listener);
        }

        virtual uint32_t getButtonCount() override
        {
            return static_cast<uint32_t>(mButtonValue.size());
        }

        virtual const char* getButtonName(uint32_t index) override
        {
            return index < getButtonCount() ? mButtonName[index].c_str() : nullptr;
        }

        virtual uint32_t getButtonIndex(const char* name) override
        {
            if (!name)
                return 0xffffffff;
            auto item = mButtonIndex.find(name);
            return item != mButtonIndex.end() ? item->second : 0xffffffff;
        }

        virtual void setButton(uint32_t index, bool value) override
        {
            if (index < getButtonCount())
            {
                mButtonValue[index] = value;
            }
            for (auto listener : mListeners)
                listener->onButtonSet(index, value);
        }

        virtual bool getButton(uint32_t index) override
        {
            return index < getButtonCount() ? mButtonValue[index] : false;
        }

        virtual uint32_t addButton(const char* name) override
        {
            auto item = mButtonIndex.find(name);
            if (item != mButtonIndex.end())
            {
                return item->second;
            }
            else
            {
                uint32_t index = static_cast<uint32_t>(mButtonValue.size());
                mButtonValue.push_back(false);
                mButtonName.push_back(name);
                mButtonIndex[name] = index;
                return index;
            }
        }

    private:
        typedef std::vector<IListener*> Listeners;
        typedef std::unordered_map<std::string, uint32_t> NameMap;
        typedef std::vector<std::string> NameArray;
        typedef std::vector<bool> ValueArray;

        Listeners   mListeners;
        NameMap     mButtonIndex;
        NameArray   mButtonName;
        ValueArray  mButtonValue;
    };
}

namespace emu
{
    namespace impl
    {
        bool registerInputDevice(emu::IEmulatorAPI& api)
        {
            auto type = api.getComponentManager().createComponentType(InputDevice::ComponentID);
            EMU_VERIFY(type);
            type->setFactory(nullptr, [](void*)
            {
                return reinterpret_cast<IComponent*>(new ::InputDevice());
            },
                [](void*, IComponent& component)
            {
                delete &reinterpret_cast<::InputDevice&>(component);
            });
            type->addInterface(IInputDevice::getID(), nullptr, [](void*, IComponent& component) -> void*
            {
                return &static_cast<IInputDevice&>(reinterpret_cast<::InputDevice&>(component));
            });
            type->addInterface(IInputDeviceMapping::getID(), nullptr, [](void*, IComponent& component) -> void*
            {
                return &static_cast<IInputDeviceMapping&>(reinterpret_cast<::InputDevice&>(component));
            });
            return true;
        }

        void unregisterInputDevice(emu::IEmulatorAPI& api)
        {
            api.getComponentManager().destroyComponentType(InputDevice::ComponentID);
        }
    }
}