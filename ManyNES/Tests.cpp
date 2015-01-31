#include "Tests.h"
#include "nes.h"
#include <string>

bool runTestRom(const char* path)
{
    bool success = false;
    auto rom = NES::Rom::load(path);
    if (rom)
    {
        auto context = NES::Context::create(*rom);
        if (context)
        {
            context->update(nullptr, 0);
            for (uint32_t frame = 0; frame < 250; ++frame)
            {
                context->update(nullptr, 0);
                if ((context->read8(0x6001) != 0xde) || (context->read8(0x6002) != 0xb0) || (context->read8(0x6003) != 0x61))
                    continue;
                uint8_t state = context->read8(0x6000);
                if ((state & 0x80) == 0)
                {
                    char result[0x2000];
                    uint16_t pos = 0;
                    do
                    {
                        result[pos] = context->read8(0x6004 + pos);
                    } while (result[pos++]);
                    printf("%s", result);
                    success = state == 0;
                    break;
                }
            }
            context->dispose();
        }
        rom->dispose();
    }
    return success;
}

bool runTestRoms()
{
    static const char* testFiles[] =
    {
        "01-basics.nes",
        "02-implied.nes",
        "03-immediate.nes",
        "04-zero_page.nes",
        "05-zp_xy.nes",
        "06-absolute.nes",
        "07-abs_xy.nes",
        "08-ind_x.nes",
        "09-ind_y.nes",
        "10-branches.nes",
        "11-stack.nes",
        "12-jmp_jsr.nes",
        "13-rts.nes",
        "14-rti.nes",
        "15-brk.nes",
        "16-special.nes",
    };

    uint32_t executed = 0;
    uint32_t passed = 0;
    for (auto file : testFiles)
    {
        std::string path = "ROMs\\rom_singles\\";
        path += file;
        ++executed;
        if (runTestRom(path.c_str()))
            ++passed;
    }

    printf("Executed: %d\nPassed: %d\n", executed, passed);

    return true;
}
