#include "Tests.h"
#include "nes.h"
#include <string>

bool runTestRom(const char* path)
{
    bool success = false;
    auto rom = nes::Rom::load(path);
    if (rom)
    {
        auto context = nes::Context::create(*rom);
        if (context)
        {
            context->update();
            for (uint32_t frame = 0; frame < 250; ++frame)
            {
                context->update();
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
        "01-basics.nes",    // OK
        "02-implied.nes",   // OK
        "03-immediate.nes",
        "04-zero_page.nes",
        "05-zp_xy.nes",
        "06-absolute.nes",  // VERIFIED
        "07-abs_xy.nes",
        "08-ind_x.nes",
        "09-ind_y.nes",
        "10-branches.nes",  // OK
        "11-stack.nes",     // OK
        "12-jmp_jsr.nes",   // OK
        "13-rts.nes",       // OK
        "14-rti.nes",       // OK
        "15-brk.nes",       // OK
        "16-special.nes",   // OK
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
