#pragma once

#include "heap.h"
#include "timed_task.h"

#include <algorithm>
namespace kotel {


template<unsigned int N>
class Scheduler: public IScheduler {
public:

    Scheduler(AbstractTimedTask * const (&arr)[N]) {
        int pos = 0;
        for (auto x: arr) {
            _items[pos]._tp = x->get_scheduled_time();
            _items[pos]._task = x;
            ++pos;
        }
    }

    virtual void reschedule() override {
        _flag_reschedule = true;
    }

    void run() {
        if (_flag_reschedule) do_reschedule();
        do {
            auto tp = get_current_timestamp();
            if (tp >= _items[0]._tp) {
                heap_pop(_items, N, compare);
                auto &x = _items[N-1];
                auto start = millis();
                x._task->run(tp);
                auto util = millis() - start;;
                auto rt = x._task->_run_time;
                if (rt > 0) rt = rt - (rt/20+1);
                x._task->_run_time = std::max<unsigned long>(rt, util);
                x._tp = x._task->get_scheduled_time();
                heap_push(_items, N, compare);
            } else {
                break;
            }
        } while (true);
    }

    template<typename Fn>
    void enum_tasks(Fn &&fn) {
        for (unsigned int i = 0; i < N; ++i) {
            fn(_items[i]._task);
        }
    }

protected:
    struct Item { // @suppress("Miss copy constructor or assignment operator")
        TimeStampMs _tp;
        AbstractTimedTask *_task;
    };


    static bool compare(const Item &a, const Item &b) {
        auto aa = a._tp;
        auto ab = b._tp;
        if (~TimeStampMs(0)-a._task->_run_time > a._tp) aa+=a._task->_run_time;
        if (~TimeStampMs(0)-b._task->_run_time > b._tp) ab+=b._task->_run_time;
        return aa < ab;
    }

    Item _items[N];
    bool _flag_reschedule = false;

    void do_reschedule() {
        for (unsigned int i = 0; i < N; ++i) {
            _items[i]._tp = _items[i]._task->get_scheduled_time();
            heap_push(_items, i+1, compare);
        }
        _flag_reschedule = false;
    }


};

template<typename ... Args>
Scheduler<sizeof...(Args)> init_scheduler(Args ... args) {
    return Scheduler<sizeof...(Args)>({args...});
}

}
