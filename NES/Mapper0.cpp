#include "Mappers.h"

namespace
{
    class Mapper : public nes::IMapper
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

    nes::AutoRegisterMapper<Mapper> mapper(0, "NROM");
}
