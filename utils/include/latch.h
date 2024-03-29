/*
Copyright (c) 2019, NVIDIA Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include "atomic_wait.h"

namespace std {

class latch {
public:
    constexpr explicit latch(ptrdiff_t expected) : counter(expected) { }
    ~latch() = default;

    latch(const latch&) = delete;
    latch& operator=(const latch&) = delete;

    inline void count_down(ptrdiff_t update = 1) {
        assert(update > 0);
        auto const old = counter.fetch_sub(update, std::memory_order_release);
        assert(old >= update);
#ifndef __NO_WAIT
        if(old == update)
            atomic_notify_all(&counter);
#endif
    }
    inline bool try_wait() const noexcept {
        return counter.load(std::memory_order_acquire) == 0;
    }
    inline void wait() const {
        while(1) {
            auto const current = counter.load(std::memory_order_acquire);
            if(current == 0) 
                return;
#ifndef __NO_WAIT
            atomic_wait_explicit(&counter, current, std::memory_order_relaxed)
#endif
            ;
        }
    }
    inline void arrive_and_wait(ptrdiff_t update = 1) {
        count_down(update);
        wait();
    }

private:
    std::atomic<ptrdiff_t> counter;
};

}

