#include "Core.h"
#include "Log.h"
#include "MemoryBus.h"
#include <memory.h>
#include <stdio.h>
#include <algorithm>

#if _DEBUG
#define ASSERT(expr)    EMU_ASSERT(expr)
#else
#define ASSERT(expr)    (expr)
#endif

namespace
{
    void NOT_IMPLEMENTED(const char* func)
    {
        emu::Log::printf(emu::Log::Type::Warning, "%s(): function not implemented\n", func);
        EMU_ASSERT(false);
    }
}

bool is_valid_page(const MEM_PAGE& page, uint16_t addr)
{
    return (page.start <= addr) && (page.end >= addr);
}

MEM_PAGE* find_page(const MEMORY_BUS& bus, uint16_t addr, uint32_t pageTable)
{
    ASSERT(addr <= bus.mem_limit);
    uint32_t pageIndex = addr >> bus.page_size_log2;
    MEM_PAGE* page = bus.page_table[pageTable][pageIndex];
    while (page)
    {
        if (is_valid_page(*page, addr))
            return page;
        page = page->next;
    }
    EMU_ASSERT(page);
    return page;
}

uint8_t memory_read8(const MEM_PAGE& page, int32_t ticks, uint16_t addr)
{
    MEM_ACCESS* access = page.access;
    uint16_t addrFixed = static_cast<uint16_t>(addr - page.offset);
    const uint8_t* buffer = access->io.read.mem;
    if (buffer)
    {
        return buffer[addrFixed];
    }
    else
    {
        EMU_ASSERT(access->io.read.func);
        return access->io.read.func(access->context, ticks, addrFixed);
    }
}

void memory_write8(const MEM_PAGE& page, int32_t ticks, uint16_t addr, uint8_t value)
{
    MEM_ACCESS* access = page.access;
    uint16_t addrFixed = static_cast<uint16_t>(addr - page.offset);
    uint8_t* buffer = access->io.write.mem;
    if (buffer)
    {
        buffer[addrFixed] = value;
        return;
    }
    else
    {
        ASSERT(access->io.write.func);
        access->io.write.func(access->context, ticks, addrFixed, value);
        return;
    }

}

uint8_t memory_bus_read8(const MEMORY_BUS& bus, int32_t ticks, uint16_t addr)
{
    MEM_PAGE* page = find_page(bus, addr, MEMORY_BUS::PAGE_TABLE_READ);
    return memory_read8(*page, ticks, addr);
}

void memory_bus_write8(const MEMORY_BUS& bus, int32_t ticks, uint16_t addr, uint8_t value)
{
    MEM_PAGE* page = find_page(bus, addr, MEMORY_BUS::PAGE_TABLE_WRITE);
    memory_write8(*page, ticks, addr, value);
}

///////////////////////////////////////////////////////////////////////////////

MEM_ACCESS& MEM_ACCESS::setReadMemory(const uint8_t* _mem, uint32_t _base)
{
    base = _base;
    context = 0;
    io.read.mem = _mem;
    io.read.func = nullptr;
    return *this;
}

MEM_ACCESS& MEM_ACCESS::setReadMethod(Read8Func _func, void* _context, uint32_t _base)
{
    base = _base;
    context = _context;
    io.read.mem = nullptr;
    io.read.func = _func;
    return *this;
}

MEM_ACCESS& MEM_ACCESS::setWriteMemory(uint8_t* _mem, uint32_t _base)
{
    base = _base;
    context = 0;
    io.write.mem = _mem;
    io.write.func = nullptr;
    return *this;
}

MEM_ACCESS& MEM_ACCESS::setWriteMethod(Write8Func _func, void* _context, uint32_t _base)
{
    base = _base;
    context = _context;
    io.write.mem = nullptr;
    io.write.func = _func;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////

MEM_ACCESS_READ_WRITE& MEM_ACCESS_READ_WRITE::setReadWriteMemory(uint8_t* _mem, uint32_t _base)
{
    read.setReadMemory(_mem, _base);
    write.setWriteMemory(_mem, _base);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////

namespace emu
{
    void MemoryBus::Accessor::reset()
    {
        static MEM_PAGE badPage =
        {
            nullptr,
            nullptr,
            1,
            0,
            0,
        };
        page = &badPage;
    }

    MemoryBus::MemoryBus()
    {
        initialize();
    }

    MemoryBus::~MemoryBus()
    {
        destroy();
    }

    void MemoryBus::initialize()
    {
        memset(&mState, 0, sizeof(mState));
    }

    bool MemoryBus::create(uint32_t memSizeLog2, uint32_t pageSizeLog2)
    {
        if (memSizeLog2 > 32 || pageSizeLog2 <= 0 || memSizeLog2 <= pageSizeLog2)
            return false;

        uint32_t numPages = 1 << pageSizeLog2;
        mPageReadRef.resize(numPages, nullptr);
        mPageWriteRef.resize(numPages, nullptr);
        mState.mem_limit = (1 << memSizeLog2) - 1;
        mState.page_size_log2 = pageSizeLog2;
        mState.page_table[MEMORY_BUS::PAGE_TABLE_READ] = &mPageReadRef[0];
        mState.page_table[MEMORY_BUS::PAGE_TABLE_WRITE] = &mPageWriteRef[0];

        return true;
    }

    void MemoryBus::destroy()
    {
        while (!mPageContainer.empty())
        {
            MEM_PAGE* instance = mPageContainer.back();
            mPageContainer.pop_back();
            delete instance;
        }
        mPageReadRef.clear();
        mPageWriteRef.clear();
        initialize();
    }

    MEM_PAGE* MemoryBus::allocatePage()
    {
        MEM_PAGE* instance = new MEM_PAGE;
        mPageContainer.push_back(instance);
        return instance;
    }

    bool MemoryBus::addMemoryRange(uint32_t pageTableId, uint16_t start, uint16_t end, MEM_ACCESS& access)
    {
        if (pageTableId >= MEMORY_BUS::PAGE_TABLE_COUNT)
            return false;
        if (start > end)
            return false;
        if (end > mState.mem_limit)
            return false;

        MEM_PAGE** page_table = mState.page_table[pageTableId];
        uint32_t offset = start - access.base;
        uint32_t pageIndexStart = start >> mState.page_size_log2;
        uint32_t pageIndexEnd = end >> mState.page_size_log2;
        for (uint32_t pageIndex = pageIndexStart; pageIndex <= pageIndexEnd; ++pageIndex)
        {
            // Create a new page entry
            uint32_t pageStart = pageIndex << mState.page_size_log2;
            uint32_t pageEnd = ((pageIndex + 1) << mState.page_size_log2) - 1;
            MEM_PAGE* memPage = allocatePage();
            memPage->next = nullptr;
            memPage->access = &access;
            memPage->start = std::max(pageStart, static_cast<uint32_t>(start));
            memPage->end = std::min(pageEnd, static_cast<uint32_t>(end));
            memPage->offset = offset;

            // Find where this new entry should start in the table
            // for this page (sorted by increasing start address)
            MEM_PAGE* prev = nullptr;
            MEM_PAGE* next = page_table[pageIndex];
            while (next && next->start < memPage->start)
            {
                prev = next;
                next = next->next;
            }

            // Adjust the end of the previous entry
            if (prev)
            {
                if (prev->end > memPage->end)
                {
                    // Previous entry also ends up passed the new one, create a new
                    // entry passed our entry to finish the range of the previous one
                    next = allocatePage();
                    *next = *prev;
                    next->start = memPage->end + 1;
                }
                if (prev->end >= memPage->start)
                    prev->end = memPage->start - 1;
                prev->next = memPage;
            }
            else
            {
                // Our entry is the first entry, update the start entry
                page_table[pageIndex] = memPage;
            }

            // Now find the next entry
            while (next && (next->end <= memPage->end))
            {
                // Here we could try to recycle the page that is
                // not used anymore before switching to the next one
                next = next->next;
            }
            if (next && next->start <= memPage->end)
            {
                // The new entry overlaps partially on the next one,
                // update the starting address of the next one
                next->start = memPage->end + 1;
            }
            memPage->next = next;
        }
        return true;
    }

    bool MemoryBus::addMemoryRange(uint16_t start, uint16_t end, MEM_ACCESS_READ_WRITE& access)
    {
        EMU_VERIFY(addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, start, end, access.read));
        EMU_VERIFY(addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, start, end, access.write));
        return true;
    }

    uint8_t MemoryBus::read8(Accessor& accessor, int32_t ticks, uint16_t addr)
    {
        if (!is_valid_page(*accessor.page, addr))
            accessor.page = find_page(mState, addr, MEMORY_BUS::PAGE_TABLE_READ);
        return memory_read8(*accessor.page, ticks, addr);
    }

    void MemoryBus::write8(Accessor& accessor, int32_t ticks, uint16_t addr, uint8_t value)
    {
        if (!is_valid_page(*accessor.page, addr))
            accessor.page = find_page(mState, addr, MEMORY_BUS::PAGE_TABLE_WRITE);
        memory_write8(*accessor.page, ticks, addr, value);
    }
}