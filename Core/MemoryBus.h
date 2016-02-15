#ifndef __BUS_H__
#define __BUS_H__

#include <stdint.h>
#include <vector>

struct MEM_PAGE_READ;
struct MEM_PAGE_WRITE;

typedef uint8_t (*Read8Func)(void* context, int32_t ticks, uint32_t addr);
typedef void (*Write8Func)(void* context, int32_t ticks, uint32_t addr, uint8_t value);

struct MEM_ACCESS
{
    uint32_t                base;
    void*                   context;
    union
    {
        struct
        {
            const uint8_t*  mem;
            Read8Func       func;
        }                   read;
        struct
        {
            uint8_t*        mem;
            Write8Func      func;
        }                   write;
    }                       io;

    MEM_ACCESS& setReadMemory(const uint8_t* _mem, uint32_t _base = 0);
    MEM_ACCESS& setReadMethod(Read8Func _func, void* _context, uint32_t _base = 0);
    MEM_ACCESS& setWriteMemory(uint8_t* _mem, uint32_t _base = 0);
    MEM_ACCESS& setWriteMethod(Write8Func _func, void* _context, uint32_t _base = 0);
};

struct MEM_ACCESS_READ_WRITE
{
    MEM_ACCESS      read;
    MEM_ACCESS      write;

    MEM_ACCESS_READ_WRITE& setReadWriteMemory(uint8_t* _mem, uint32_t _base = 0);
};

struct MEM_PAGE
{
    MEM_PAGE*               next;
    MEM_ACCESS*             access;
    uint32_t                start;
    uint32_t                end;
    uint32_t                offset;
};

struct MEMORY_BUS
{
    static const uint32_t PAGE_TABLE_READ = 0;
    static const uint32_t PAGE_TABLE_WRITE = 1;
    static const uint32_t PAGE_TABLE_COUNT = 2;

    uint32_t        mem_limit;
    uint32_t        page_size_log2;
    MEM_PAGE**      page_table[PAGE_TABLE_COUNT];
};

uint8_t memory_bus_read8(const MEMORY_BUS& bus, int32_t ticks, uint16_t addr);
void memory_bus_write8(const MEMORY_BUS& bus, int32_t ticks, uint16_t addr, uint8_t value);

namespace emu
{
    class MemoryBus
    {
    public:
        struct Accessor
        {
            Accessor()
            {
                reset();
            }

            void reset();

            MEM_PAGE*   page;
        };

        MemoryBus();
        ~MemoryBus();
        bool create(uint32_t memSizeLog2, uint32_t pageSizeLog2);
        void destroy();

        MEMORY_BUS& getState()
        {
            return mState;
        }

        bool addMemoryRange(uint32_t pageTableId, uint16_t start, uint16_t end, MEM_ACCESS& access);
        bool addMemoryRange(uint16_t start, uint16_t end, MEM_ACCESS_READ_WRITE& access);
        uint8_t read8(Accessor& accessor, int32_t ticks, uint16_t addr);
        void write8(Accessor& accessor, int32_t ticks, uint16_t addr, uint8_t value);

    private:
        MEMORY_BUS                  mState;
        std::vector<MEM_PAGE*>      mPageReadRef;
        std::vector<MEM_PAGE*>      mPageWriteRef;
        std::vector<MEM_PAGE*>      mPageContainer;

        void initialize();
        MEM_PAGE* allocatePage();
    };
}

#endif
