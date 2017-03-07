#include "InputController.h"
#include "Serializer.h"
#include "Stream.h"
#include <vector>

namespace
{
    class GroupControllerImpl : public emu::GroupController
    {
    public:
        GroupControllerImpl()
        {
        }

        ~GroupControllerImpl()
        {
            destroy();
        }

        virtual void dispose()
        {
            delete this;
        }

        bool create()
        {
            return true;
        }

        void destroy()
        {
        }

        virtual void reset()
        {
            for (auto controller : mControllers)
            {
                controller->reset();
            }
        }

        virtual uint8_t readInput()
        {
            uint8_t buttonMask = 0;
            for (auto controller : mControllers)
            {
                buttonMask |= controller->readInput();
            }
            return buttonMask;
        }

        virtual void addController(InputController& controller)
        {
            mControllers.push_back(&controller);
        }

    private:
        typedef std::vector<InputController*> ControllerColllection;

        ControllerColllection   mControllers;
    };

    ///////////////////////////////////////////////////////////////////////////

    struct InputStreamData
    {
        std::vector<uint8_t>    repeat;
        emu::Buffer    value;

        void clear()
        {
            repeat.clear();
            value.clear();
        }

        void serialize(emu::ISerializer& serializer)
        {
            // Version
            uint32_t version = 2;
            serializer
                .value("Version", version)
                .value("Repeat", repeat)
                .value("Value", value);
        }
    };

    class InputRecorderImpl : public emu::InputRecorder
    {
    public:
        InputRecorderImpl()
            : mSource(nullptr)
        {
        }

        ~InputRecorderImpl()
        {
            destroy();
        }

        virtual void dispose()
        {
            delete this;
        }

        bool create(InputController& source)
        {
            mSource = &source;
            return true;
        }

        void destroy()
        {
            mSource = nullptr;
        }

        virtual InputController& getSource()
        {
            return *mSource;
        }

        virtual bool save(const char* path)
        {
            emu::FileStream stream(path, "wb");
            if (!stream.valid())
                return false;

            emu::BinaryWriter writer(stream);
            mData.serialize(writer);
            return true;
        }

        virtual uint8_t readInput()
        {
            uint8_t value = mSource->readInput();
            if (mData.value.empty() || (mData.value.back() != value) || (mData.repeat.back() == 0xff))
            {
                // Input has changed or need to be refreshed
                mData.value.push_back(value);
                mData.repeat.push_back(0);
            }
            else
            {
                // Input from previous frame can be reused one more frame
                ++mData.repeat.back();
            }
            return value;
        }

    private:
        InputController*    mSource;
        InputStreamData     mData;
    };

    class InputPlaybackImpl : public emu::InputPlayback
    {
    public:
        InputPlaybackImpl()
            : mSource(nullptr)
            , mPos(0)
            , mCount(0)
        {
        }

        ~InputPlaybackImpl()
        {
            destroy();
        }

        virtual void dispose()
        {
            delete this;
        }

        bool create(InputController& source)
        {
            mSource = &source;
            return true;
        }

        void destroy()
        {
            mSource = nullptr;
        }

        virtual InputController& getSource()
        {
            return *mSource;
        }

        virtual bool load(const char* path)
        {
            emu::FileStream stream(path, "rb");
            if (!stream.valid())
                return false;

            emu::BinaryReader reader(stream);
            mData.serialize(reader);
            mPos = 0;
            mCount = 0;
            return true;
        }

        virtual uint8_t readInput()
        {
            uint8_t value = mSource->readInput();
            if (mPos < mData.value.size())
            {
                uint8_t recorded = mData.value[mPos];
                uint32_t repeat = mData.repeat[mPos];
                if (++mCount > repeat)
                {
                    mCount = 0;
                    ++mPos;
                }
                value |= recorded;
            }
            return value;
        }

    private:
        InputController*    mSource;
        InputStreamData     mData;
        uint32_t            mPos;
        uint32_t            mCount;
    };
}

namespace emu
{
    GroupController* GroupController::create()
    {
        GroupControllerImpl* instance = new GroupControllerImpl();
        if (!instance->create())
        {
            instance->dispose();
            instance = nullptr;
        }
        return instance;
    }

    InputRecorder* InputRecorder::create(InputController& source)
    {
        InputRecorderImpl* instance = new InputRecorderImpl();
        if (!instance->create(source))
        {
            instance->dispose();
            instance = nullptr;
        }
        return instance;
    }

    InputPlayback* InputPlayback::create(InputController& source)
    {
        InputPlaybackImpl* instance = new InputPlaybackImpl();
        if (!instance->create(source))
        {
            instance->dispose();
            instance = nullptr;
        }
        return instance;
    }
}