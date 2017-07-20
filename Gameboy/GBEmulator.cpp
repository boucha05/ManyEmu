#include <Core/Emulator.h>
#include <Core/Serializer.h>
#include <Core/Stream.h>
#include "gb.h"

namespace
{
    class EmulatorGB : public gb::Emulator
    {
    public:
        virtual bool getSystemInfo(SystemInfo& info) override
        {
            info.name = "Game Boy";
            info.extensions = "gb";
            return true;
        }

        virtual emu::IRom* loadRom(const char* path) override
        {
            return gb::Rom::load(path);
        }

        virtual void unloadRom(emu::IRom& rom) override
        {
            static_cast<gb::Rom&>(rom).dispose();
        }

        virtual emu::IContext* createContext(const emu::IRom& rom) override
        {
            return gb::Context::create(static_cast<const gb::Rom&>(rom), gb::Model::GB);
        }

        virtual void destroyContext(emu::IContext& context) override
        {
            static_cast<gb::Context&>(context).dispose();
        }
    };

    class EmulatorGBC : public EmulatorGB
    {
    public:
        virtual bool getSystemInfo(SystemInfo& info) override
        {
            info.name = "Game Boy Color";
            info.extensions = "gbc";
            return true;
        }

        virtual emu::IContext* createContext(const emu::IRom& rom) override
        {
            return gb::Context::create(static_cast<const gb::Rom&>(rom), gb::Model::GBC);
        }
    };

    class EmulatorSGB : public EmulatorGB
    {
    public:
        virtual bool getSystemInfo(SystemInfo& info) override
        {
            info.name = "Super Game Boy";
            info.extensions = "sgb";
            return true;
        }

        virtual emu::IContext* createContext(const emu::IRom& rom) override
        {
            return gb::Context::create(static_cast<const gb::Rom&>(rom), gb::Model::SGB);
        }
    };

}

namespace gb
{
    Emulator& Emulator::getEmulatorGB()
    {
        static EmulatorGB instance;
        return instance;
    }

    Emulator& Emulator::getEmulatorGBC()
    {
        static EmulatorGBC instance;
        return instance;
    }

    Emulator& Emulator::getEmulatorSGB()
    {
        static EmulatorSGB instance;
        return instance;
    }
}