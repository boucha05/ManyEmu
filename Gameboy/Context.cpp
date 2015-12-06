#include <Core/Clock.h>
#include <Core/MemoryBus.h>
#include <Core/Serialization.h>
#include "GB.h"
#include <vector>

namespace
{
    static const uint32_t MEM_SIZE_LOG2 = 16;
    static const uint32_t MEM_SIZE = 1 << MEM_SIZE_LOG2;
    static const uint32_t MEM_PAGE_SIZE_LOG2 = 8;
}

namespace
{
    class ContextImpl : public gb::Context
    {
    public:
        ContextImpl()
            : rom(nullptr)
        {
        }

        ~ContextImpl()
        {
            destroy();
        }

        bool create(const gb::Rom& _rom)
        {
            rom = &_rom;
            const auto& romDesc = rom->getDescription();
            const auto& romContent = rom->getContent();

            EMU_VERIFY(memory.create(MEM_SIZE_LOG2, MEM_PAGE_SIZE_LOG2));

            return true;
        }

        void destroy()
        {
            memory.destroy();
            rom = nullptr;
        }

        virtual void dispose()
        {
            delete this;
        }

        virtual void reset()
        {
        }

        virtual void setController(uint32_t /*index*/, uint32_t /*buttons*/)
        {
        }

        virtual void setSoundBuffer(int16_t* /*buffer*/, size_t /*size*/)
        {
        }

        virtual void setRenderSurface(void* surface, size_t pitch)
        {
        }

        virtual void update()
        {
        }

        virtual uint8_t read8(uint16_t /*addr*/)
        {
            return 0;
        }

        virtual void write8(uint16_t /*addr*/, uint8_t /*value*/)
        {
        }

        virtual void serializeGameData(emu::ISerializer& /*serializer*/)
        {
        }

        virtual void serializeGameState(emu::ISerializer& /*serializer*/)
        {
        }

    private:
        const gb::Rom*          rom;
        emu::MemoryBus          memory;
    };
}

namespace gb
{
    Context* Context::create(const Rom& rom)
    {
        ContextImpl* context = new ContextImpl;
        if (!context->create(rom))
        {
            context->dispose();
            context = nullptr;
        }
        return context;
    }
}