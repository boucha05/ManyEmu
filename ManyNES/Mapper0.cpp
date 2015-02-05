#include "Mappers.h"

namespace
{
    class Mapper : public NES::Mapper
    {
    public:
        virtual void dispose()
        {
            delete this;
        }

        virtual bool initialize(const Components& components)
        {
            return true;
        }
    };

    NES::AutoRegisterMapper<Mapper> mapper0(0, "NROM");
    NES::AutoRegisterMapper<Mapper> mapper1(1, "SxROM");
}
