#include "Mappers.h"

namespace nes
{
    class Mapper0 : public nes::IMapper
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
}
