#include <Core/MemoryBus.h>
#include <Core/Serialization.h>
#include "Mappers.h"
#include "nes.h"
#include "PPU.h"
#include <vector>

namespace
{
    class Mapper : public NES::IMapper
    {
    public:
        Mapper()
            : mRom(nullptr)
            , mClock(nullptr)
            , mPpu(nullptr)
            , mMapperListener(nullptr)
        {
        }

        virtual void dispose()
        {
            if (mPpu)
                mPpu->removeListener(mPpuListener);
            delete this;
        }

        virtual bool initialize(const Components& components)
        {
            if (!components.memory)
                return false;
            auto& cpuMemory = *components.memory;

            // Clock
            mClock = components.clock;

            // PRG ROM
            if (!components.rom)
                return false;
            mRom = components.rom;
            mMemPrgRomWrite.setWriteMethod(regWrite, this);

            // CHR ROM
            if (!components.ppu)
                return false;
            mPpu = components.ppu;
            auto& ppuMemory = mPpu->getMemory();
            mPpuListener.initialize(*this);
            mPpu->addListener(mPpuListener);

            // Mapper listener
            mMapperListener = components.listener;

            // Name table
            const auto& romDescription = mRom->getDescription();
            if (romDescription.mirroring == NES::Rom::Mirroring_FourScreen)
                mNameTableLocal.resize(0x0800);

            // Load banks
            reset();

            // Enable memory ranges
            for (uint32_t bank = 0; bank < 4; ++bank)
            {
                uint32_t addrStart = 0x8000 + bank * 0x2000;
                uint32_t addrEnd = addrStart + 0x1fff;
                cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, addrStart, addrEnd, mMemPrgRomRead[bank]);
            }
            cpuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_WRITE, 0x8000, 0xffff, mMemPrgRomWrite);
            for (uint32_t bank = 0; bank < 8; ++bank)
            {
                uint32_t addrStart = 0x0000 + bank * 0x0400;
                uint32_t addrEnd = addrStart + 0x03ff;
                ppuMemory.addMemoryRange(MEMORY_BUS::PAGE_TABLE_READ, addrStart, addrEnd, mMemChrRomRead[bank]);
            }

            return true;
        }

        virtual void reset()
        {
            for (uint32_t port = 0; port < 8; ++port)
                mBankPorts[port] = 0;
            mChrMode = 0;
            mPrgMode = 0;
            mPort = 0;
            mMirroring = 0;
            mWramEnable = false;
            mWramWriteProtect = true;
            mIrqCount = 0xff;
            mIrqReload = 0;
            mIrqEnable = false;
            mIrqPending = false;
            updateMemoryMap();
            updateIrqStatus(0);
        }

        virtual void beginFrame()
        {
            updateIrqStatus(0);
        }

        virtual void serializeGameState(emu::ISerializer& serializer)
        {
            uint32_t version = 1;
            serializer.serialize(version);
            serializer.serialize(mNameTableLocal);
            serializer.serialize(mBankPorts, EMU_ARRAY_SIZE(mBankPorts));
            serializer.serialize(mChrMode);
            serializer.serialize(mPrgMode);
            serializer.serialize(mPort);
            serializer.serialize(mMirroring);
            serializer.serialize(mWramEnable);
            serializer.serialize(mWramWriteProtect);
            serializer.serialize(mIrqCount);
            serializer.serialize(mIrqReload);
            serializer.serialize(mIrqEnable);
            serializer.serialize(mIrqPending);
            updateMemoryMap();
            updateIrqStatus(0);
        }

    private:
        void updateMemoryMap()
        {
            const auto& romDescription = mRom->getDescription();
            const auto& romContent = mRom->getContent();
            
            // PRG ROM
            uint32_t prgBank[4];
            uint32_t prgBankCount = romDescription.prgRomPages * 2;
            if (mPrgMode == 0)
            {
                prgBank[0] = mBankPorts[6];
                prgBank[1] = mBankPorts[7];
                prgBank[2] = prgBankCount - 2;
                prgBank[3] = prgBankCount - 1;
            }
            else
            {
                prgBank[0] = prgBankCount - 2;
                prgBank[1] = mBankPorts[7];
                prgBank[2] = mBankPorts[6];
                prgBank[3] = prgBankCount - 1;
            }
            for (uint32_t bank = 0; bank < 4; ++bank)
            {
                EMU_ASSERT(prgBank[bank] < prgBankCount);
                mMemPrgRomRead[bank].setReadMemory(&romContent.prgRom[0x2000 * prgBank[bank]]);
            }

            // CHR ROM
            uint32_t chrBank[8];
            uint32_t chrBankCount = romDescription.chrRomPages * 8;
            if (mChrMode == 0)
            {
                chrBank[0] = mBankPorts[0] & ~1;
                chrBank[1] = mBankPorts[0] | 1;
                chrBank[2] = mBankPorts[1] & ~1;
                chrBank[3] = mBankPorts[1] | 1;
                chrBank[4] = mBankPorts[2];
                chrBank[5] = mBankPorts[3];
                chrBank[6] = mBankPorts[4];
                chrBank[7] = mBankPorts[5];
            }
            else
            {
                chrBank[0] = mBankPorts[2];
                chrBank[1] = mBankPorts[3];
                chrBank[2] = mBankPorts[4];
                chrBank[3] = mBankPorts[5];
                chrBank[4] = mBankPorts[0] & ~1;
                chrBank[5] = mBankPorts[0] | 1;
                chrBank[6] = mBankPorts[1] & ~1;
                chrBank[7] = mBankPorts[1] | 1;
            }
            for (uint32_t bank = 0; bank < 8; ++bank)
            {
                EMU_ASSERT(chrBank[bank] < chrBankCount);
                mMemChrRomRead[bank].setReadMemory(&romContent.chrRom[0x0400 * chrBank[bank]]);
            }

            // Name tables
            uint8_t* nameTable[4];
            uint8_t* nameTableVRAM = mPpu->getNameTableMemory();
            if (romDescription.mirroring == NES::Rom::Mirroring_FourScreen)
            {
                uint8_t* nameTableLocal = mNameTableLocal.empty() ? nameTableVRAM : &mNameTableLocal[0];
                nameTable[0] = &nameTableVRAM[0x0000];
                nameTable[1] = &nameTableVRAM[0x0400];
                nameTable[2] = &nameTableLocal[0x0000];
                nameTable[3] = &nameTableLocal[0x0400];
            }
            else
            {
                if (mMirroring == 0)
                {
                    // Vertical mirroring
                    nameTable[0] = nameTable[2] = &nameTableVRAM[0x0000];
                    nameTable[1] = nameTable[3] = &nameTableVRAM[0x0400];
                }
                else
                {
                    // Horizontal mirroring
                    nameTable[0] = nameTable[1] = &nameTableVRAM[0x0000];
                    nameTable[2] = nameTable[3] = &nameTableVRAM[0x0400];
                }
            }
            for (uint32_t bank = 0; bank < 4; ++bank)
            {
                mPpu->getNameTableRead(bank)->setReadMemory(nameTable[bank]);
                mPpu->getNameTableWrite(bank)->setWriteMemory(nameTable[bank]);
            }
        }

        void onLineSync(int32_t tick)
        {
            if (mIrqCount)
                setLineSync(tick, mIrqCount);
        }

        static void onLineSync(void* context, int32_t tick)
        {
            static_cast<Mapper*>(context)->onLineSync(tick);
        }

        void setLineSync(int32_t tick, uint32_t lines)
        {
            uint32_t desiredTick = tick + mPpu->getTickCount(lines, 0);
            mClock->addEvent(onLineSync, this, desiredTick);
        }

        void updateIrqStatus(int32_t tick)
        {
            setLineSync(tick, mIrqCount);
            static bool irqEnabled = true;
            if (irqEnabled)
                mMapperListener->onIrqUpdate(mIrqPending && mIrqEnable);
        }

        void onVisibleLineStart(int32_t tick)
        {
            if (--mIrqCount == 0)
            {
                mIrqCount = mIrqReload;
                mIrqPending = true;
                updateIrqStatus(tick);
            }
        }

        void regWrite(int32_t tick, uint32_t addr, uint8_t value)
        {
            switch (addr & 0x6001)
            {
            case 0x0000:
                mChrMode = (value >> 7) & 0x01;
                mPrgMode = (value >> 6) & 0x01;
                mPort = value & 0x07;
                updateMemoryMap();
                break;

            case 0x0001:
                mBankPorts[mPort] = value;
                updateMemoryMap();
                break;

            case 0x2000:
                mMirroring = value & 0x01;
                updateMemoryMap();
                break;

            case 0x2001:
                mWramEnable = (value & 0x80) != 0;
                mWramWriteProtect = (value & 0x40) == 0;
                updateMemoryMap();
                break;

            case 0x4000:
                mIrqReload = value & 0xff;
                break;

            case 0x4001:
                mIrqCount = mIrqReload + 1;
                updateIrqStatus(tick);
                break;

            case 0x6000:
                mIrqEnable = false;
                mIrqPending = false;
                updateIrqStatus(tick);
                break;

            case 0x6001:
                mIrqEnable = true;
                updateIrqStatus(tick);
                break;

            default:
                EMU_ASSERT(false);
            }
        }

        static void regWrite(void* context, int32_t ticks, uint32_t addr, uint8_t value)
        {
            static_cast<Mapper*>(context)->regWrite(ticks, addr, value);
        }

        class PPUListener : public NES::PPU::IListener
        {
        public:
            PPUListener()
                : mMapper(nullptr)
            {
            }

            void initialize(Mapper& mapper)
            {
                mMapper = &mapper;
            }

            virtual void onVisibleLineStart(int32_t tick)
            {
                mMapper->onVisibleLineStart(tick);
            }

        private:
            Mapper*     mMapper;
        };

        const NES::Rom*             mRom;
        emu::Clock*                 mClock;
        NES::PPU*                   mPpu;
        PPUListener                 mPpuListener;
        NES::IMapper::IListener*    mMapperListener;
        std::vector<uint8_t>        mNameTableLocal;
        MEM_ACCESS                  mMemPrgRomRead[4];
        MEM_ACCESS                  mMemPrgRomWrite;
        MEM_ACCESS                  mMemChrRomRead[8];
        uint8_t                     mBankPorts[8];
        uint8_t                     mChrMode;
        uint8_t                     mPrgMode;
        uint8_t                     mPort;
        uint8_t                     mMirroring;
        bool                        mWramEnable;
        bool                        mWramWriteProtect;
        uint8_t                     mIrqCount;
        uint8_t                     mIrqReload;
        bool                        mIrqEnable;
        bool                        mIrqPending;
    };

    NES::AutoRegisterMapper<Mapper> mapper(4, "MMC3/MMC6");
}
