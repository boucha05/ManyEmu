#include "MemoryBus.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <algorithm>

#if _DEBUG
#define ASSERT(expr)    assert(expr)
#else
#define ASSERT(expr)    (expr)
#endif

namespace
{
    void NOT_IMPLEMENTED(const char* func)
    {
        printf("%s(): function not implemented\n", func);
        assert(false);
    }
}

uint8_t memory_bus_read8(const MEMORY_BUS& bus, int32_t ticks, uint16_t addr)
{
    ASSERT(addr <= bus.mem_limit);
    uint32_t pageIndex = addr >> bus.page_size_log2;
    MEM_PAGE* page = bus.page_table[MEMORY_BUS::PAGE_TABLE_READ][pageIndex];
    while (page)
    {
        if ((page->start <= addr) && (page->end >= addr))
        {
            MEM_ACCESS* access = page->access;
            uint16_t addrFixed = addr - page->offset;
            const uint8_t* buffer = access->io.read.mem;
            if (buffer)
            {
                return buffer[addrFixed];
            }
            else
            {
                ASSERT(access->io.read.func);
                return access->io.read.func(access->context, ticks, addrFixed);
            }
        }
        page = page->next;
    }
    ASSERT(false);
    return 0;
}

void memory_bus_write8(const MEMORY_BUS& bus, int32_t ticks, uint16_t addr, uint8_t value)
{
    ASSERT(addr <= bus.mem_limit);
    uint32_t pageIndex = addr >> bus.page_size_log2;
    MEM_PAGE* page = bus.page_table[MEMORY_BUS::PAGE_TABLE_WRITE][pageIndex];
    while (page)
    {
        if ((page->start <= addr) && (page->end >= addr))
        {
            MEM_ACCESS* access = page->access;
            uint16_t addrFixed = addr - page->offset;
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
        page = page->next;
    }
    ASSERT(false);
}

///////////////////////////////////////////////////////////////////////////////

void MEM_ACCESS::setReadMemory(const uint8_t* _mem, uint32_t _base)
{
    base = _base;
    context = 0;
    io.read.mem = _mem;
    io.read.func = nullptr;
}

void MEM_ACCESS::setReadMethod(Read8Func _func, void* _context, uint32_t _base)
{
    base = _base;
    context = _context;
    io.read.mem = nullptr;
    io.read.func = _func;
}

void MEM_ACCESS::setWriteMemory(uint8_t* _mem, uint32_t _base)
{
    base = _base;
    context = 0;
    io.write.mem = _mem;
    io.write.func = nullptr;
}

void MEM_ACCESS::setWriteMethod(Write8Func _func, void* _context, uint32_t _base)
{
    base = _base;
    context = _context;
    io.write.mem = nullptr;
    io.write.func = _func;
}

///////////////////////////////////////////////////////////////////////////////

namespace NES
{
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
            uint32_t pageStart = pageIndex << mState.page_size_log2;
            uint32_t pageEnd = ((pageIndex + 1) << mState.page_size_log2) - 1;
            MEM_PAGE* memPage = allocatePage();
            memPage->next = page_table[pageIndex];
            memPage->access = &access;
            memPage->start = std::max(pageStart, static_cast<uint32_t>(start));
            memPage->end = std::min(pageEnd, static_cast<uint32_t>(end));
            memPage->offset = offset;
            page_table[pageIndex] = memPage;
        }
        return false;
    }
}