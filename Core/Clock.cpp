#include "Clock.h"
#include "Core.h"
#include "Serializer.h"

namespace
{
    void advancedDummyCallback(void* context, int32_t ticks)
    {
        EMU_UNUSED(context);
        EMU_UNUSED(ticks);
    }
}

namespace emu
{
    Clock::Clock()
        : mTargetTicks(0)
        , mDesiredTicks(0)
    {
    }

    Clock::~Clock()
    {
        destroy();
    }

    bool Clock::create()
    {
        return true;
    }

    void Clock::destroy()
    {
    }

    void Clock::reset()
    {
        mTargetTicks = 0;
        mDesiredTicks = 0;
        setDesiredTicks(0);
        mTimers.clear();

        for (auto listener : mListeners)
            listener->resetClock();
    }

    void Clock::execute(int32_t tick)
    {
        setTargetExecution(tick);
        while (canExecute())
        {
            beginExecute();
            for (auto listener : mListeners)
                listener->execute();
            endExecute();
        }
    }

    bool Clock::setTargetExecution(int32_t ticks)
    {
        if (canExecute())
            return false;

        mTargetTicks = ticks;
        addEvent(advancedDummyCallback, nullptr, ticks);
        return true;
    }

    bool Clock::canExecute() const
    {
        return mDesiredTicks < mTargetTicks;
    }

    void Clock::beginExecute()
    {
        // Execute until the first timer event
        int32_t firstEventTicks = mTimers.begin()->first;
        setDesiredTicks(firstEventTicks);
    }

    void Clock::endExecute()
    {
        // Signal events that are ready
        while (!mTimers.empty() && (mTimers.begin()->first <= mDesiredTicks))
        {
            const auto& timerEvent = mTimers.begin()->second;
            timerEvent.callback(timerEvent.context, mTimers.begin()->first);
            mTimers.erase(mTimers.begin());
        }
    }

    void Clock::advance()
    {
        // Prepare timers for next target
        TimerQueue oldTimers = mTimers;
        mTimers.clear();
        for (auto timerEvent : oldTimers)
        {
            mTimers.insert(std::pair<int32_t, TimerEvent>(timerEvent.first - mTargetTicks, timerEvent.second));
        }

        int32_t targetTicks = mTargetTicks;
        mDesiredTicks -= targetTicks;
        mTargetTicks = 0;

        // Advance reference time on listeners
        for (auto listener : mListeners)
        {
            listener->advanceClock(targetTicks);
        }
    }

    void Clock::setDesiredTicks(int32_t ticks)
    {
        mDesiredTicks = ticks;
        for (auto listener : mListeners)
        {
            listener->setDesiredTicks(mDesiredTicks);
        }
    }

    void Clock::addEvent(TimerCallback callback, void* context, int32_t ticks)
    {
        TimerEvent timerEvent;
        timerEvent.callback = callback;
        timerEvent.context = context;
        mTimers.insert(std::pair<int32_t, TimerEvent>(ticks, timerEvent));
        if (ticks < mDesiredTicks)
            setDesiredTicks(ticks);
    }

    void Clock::addSync(int32_t ticks)
    {
        addEvent([](void* context, int32_t ticks) { EMU_UNUSED(context); EMU_UNUSED(ticks); }, nullptr, ticks);
    }

    void Clock::clearEvents()
    {
        mTimers.clear();
    }

    void Clock::addListener(IListener& listener)
    {
        listener.resetClock();
        listener.setDesiredTicks(mDesiredTicks);
        mListeners.push_back(&listener);
    }

    void Clock::removeListener(IListener& listener)
    {
        auto item = std::find(mListeners.begin(), mListeners.end(), &listener);
        mListeners.erase(item);
    }

    void Clock::serialize(ISerializer& serializer)
    {
        // TODO: Can't serialize properly if we have timers queued
        //EMU_ASSERT(mTimers.empty());

        uint32_t version = 1;
        EMU_UNUSED(version);
        serializer
            .value("TargetTicks", mTargetTicks)
            .value("DesiredTicks", mDesiredTicks);
    }
}
