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

    NES::AutoRegisterMapper<Mapper> mapper(0, "NROM");
}
