#pragma once

#include "heap.h"
#include "task.h"

#include <algorithm>
namespace kotel {


template<unsigned int N>
class Scheduler: public IScheduler {
public:

    Scheduler(AbstractTask * const (&arr)[N]) {
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

    bool run() {
        bool s = false;
        if (_flag_reschedule) do_reschedule();
        auto ln = N;
        while (ln > 0) {
            auto tp = get_current_timestamp();
            if (tp >= _items[0]._tp) {
                heap_pop(_items, ln, compare);
                --ln;
                auto &x = _items[ln];
                s = true;
                x._task->resume(tp);
                x._tp = x._task->get_scheduled_time();
            } else {
                break;
            }
        }
        while (ln < N) {
            ++ln;
            heap_push(_items, ln, compare);
        }
        return s;
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
        AbstractTask *_task;
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
