#ifndef __BUS_H__
#define __BUS_H__

#include <stdint.h>

struct MEM_PAGE_READ;
struct MEM_PAGE_WRITE;

typedef bool(*Read8Func)(void* context, uint32_t addr, uint8_t value);
typedef bool(*Write8Func)(void* context, uint32_t addr, uint8_t& value);

struct MEM_PAGE
{
    MEM_PAGE*       next;
    uint32_t        start;
    uint32_t        size;
};

struct MEM_PAGE_READ : public MEM_PAGE
{
    uint8_t*        mem;
    Read8Func*      func;
};

struct MEM_PAGE_WRITE : public MEM_PAGE
{
    uint8_t*        mem;
    Write8Func*     func;
};

static const uint32_t MEM_SIZE_LOG2 = 16;
static const uint32_t MEM_SIZE = 1 << MEM_SIZE_LOG2;
static const uint32_t MEM_PAGE_SIZE_LOG2 = 10;
static const uint32_t MEM_PAGE_SIZE = 1 << MEM_PAGE_SIZE_LOG2;
static const uint32_t MEM_PAGE_COUNT = 16 - MEM_PAGE_SIZE_LOG2;

struct MEMORY_BUS
{
    MEM_PAGE_READ*      mem_page_read[MEM_PAGE_COUNT];
    MEM_PAGE_WRITE*     mem_page_write[MEM_PAGE_COUNT];
};

void memory_bus_initialize(MEMORY_BUS& bus);
bool memory_bus_add_page_read(MEMORY_BUS& bus, MEM_PAGE_READ& mem_page_read);
bool memory_bus_add_page_write(MEMORY_BUS& bus, MEM_PAGE_WRITE& mem_page_write);
MEM_PAGE_READ* memory_bus_find_page_read(const MEMORY_BUS& bus, uint16_t addr);
MEM_PAGE_WRITE* memory_bus_find_page_write(const MEMORY_BUS& bus, uint16_t addr);
uint8_t memory_bus_read8(const MEMORY_BUS& bus, uint16_t addr);
void memory_bus_write8(const MEMORY_BUS& bus, uint16_t addr, uint8_t value);

class MemoryBus
{
public:
    MemoryBus();
    ~MemoryBus();
    
    MEMORY_BUS& getState()
    {
        return mMemoryBus;
    }

private:
    MEMORY_BUS  mMemoryBus;
};

#endif
