#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stdint.h>
#include <map>
#include <vector>

namespace emu
{
    class ISerializer;

    class Clock
    {
    public:
        typedef void(*TimerCallback)(void* context, int32_t ticks);

        class IListener
        {
        public:
            virtual void execute() {}
            virtual void resetClock() {}
            virtual void advanceClock(int32_t ticks) {}
            virtual void setDesiredTicks(int32_t ticks) {}
        };

        Clock();
        ~Clock();
        bool create();
        void destroy();
        void reset();
        void execute(int32_t tick);
        void advance();
        void addEvent(TimerCallback callback, void* context, int32_t ticks);
        void clearEvents();
        void addListener(IListener& listener);
        void removeListener(IListener& listener);
        void serialize(ISerializer& serializer);

        int32_t getDesiredTicks() const
        {
            return mDesiredTicks;
        }

    private:
        struct TimerEvent
        {
            TimerCallback       callback;
            void*               context;
        };
        typedef std::multimap<int32_t, TimerEvent> TimerQueue;
        typedef std::vector<IListener*> ListenerQueue;

        bool setTargetExecution(int32_t ticks);
        bool canExecute() const;
        void beginExecute();
        void endExecute();
        void setDesiredTicks(int32_t ticks);

        TimerQueue              mTimers;
        ListenerQueue           mListeners;
        int32_t                 mTargetTicks;
        int32_t                 mDesiredTicks;
    };
}

#endif
