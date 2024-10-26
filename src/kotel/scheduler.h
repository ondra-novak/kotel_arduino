#pragma once

#include "heap.h"
#include "timed_task.h"

namespace kotel {


template<unsigned int N>
class Scheduler: public IScheduler {
public:

    Scheduler(ITimedTask * const (&arr)[N]) {
        int pos = 0;
        for (auto x: arr) {
            _items[pos]._tp = x->get_scheduled_time();
            _items[pos]._task = x;
            ++pos;
        }
    }

    virtual void reschedule() override {
        for (unsigned int i = 0; i < N; ++i) {
            _items[i]._tp = _items[i]._task->get_scheduled_time();
            heap_push(_items, i+1, compare);
        }
    }

    void run() {
        do {
            auto tp = get_current_timestamp();
            if (tp >= _items[0]._tp) {
                heap_pop(_items, N, compare);
                auto &x = _items[N-1];
                x._task->run(tp);
                x._tp = x._task->get_scheduled_time();
                heap_push(_items, N, compare);
            } else {
                break;
            }
        } while (true);
    }

protected:
    struct Item { // @suppress("Miss copy constructor or assignment operator")
        TimeStampMs _tp;
        ITimedTask *_task;
    };


    static bool compare(const Item &a, const Item &b) {
        return a._tp < b._tp;
    }

    Item _items[N];


};

template<typename ... Args>
Scheduler<sizeof...(Args)> init_scheduler(Args ... args) {
    return Scheduler<sizeof...(Args)>({args...});
}

}
