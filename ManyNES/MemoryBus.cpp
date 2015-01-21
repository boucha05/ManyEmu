#include "MemoryBus.h"
#include <assert.h>
#include <stdio.h>

namespace
{
    void NOT_IMPLEMENTED(const char* func)
    {
        printf("%s(): function not implemented\n", func);
        assert(false);
    }
}

void memory_bus_initialize(MEMORY_BUS& bus)
{
    NOT_IMPLEMENTED("memory_bus_initialize");
}

bool memory_bus_add_page_read(MEMORY_BUS& bus, MEM_PAGE_READ& mem_page_read)
{
    NOT_IMPLEMENTED("memory_bus_initialize");
    return false;
}

bool memory_bus_add_page_write(MEMORY_BUS& bus, MEM_PAGE_WRITE& mem_page_write)
{
    NOT_IMPLEMENTED("memory_bus_initialize");
    return false;
}

MEM_PAGE_READ* memory_bus_find_page_read(const MEMORY_BUS& bus, uint16_t addr)
{
    NOT_IMPLEMENTED("memory_bus_initialize");
    return nullptr;
}

MEM_PAGE_WRITE* memory_bus_find_page_write(const MEMORY_BUS& bus, uint16_t addr)
{
    NOT_IMPLEMENTED("memory_bus_initialize");
    return nullptr;
}

uint8_t memory_bus_read8(const MEMORY_BUS& bus, uint16_t addr)
{
    NOT_IMPLEMENTED("memory_bus_initialize");
    return 0;
}

void memory_bus_write8(const MEMORY_BUS& bus, uint16_t addr, uint8_t value)
{
    NOT_IMPLEMENTED("memory_bus_initialize");
}

///////////////////////////////////////////////////////////////////////////////

MemoryBus::MemoryBus()
{
}

MemoryBus::~MemoryBus()
{
}
