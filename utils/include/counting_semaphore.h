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

#include <cstddef>
#include <algorithm>
#include <thread> // For `std::this_thread::sleep_for`.
#include "atomic_wait.h"

#ifndef SEMHDR
#define SEMHDR

//#define __NO_SEM
//#define __NO_SEM_BACK
//#define __NO_SEM_FRONT
//#define __NO_SEM_POLL

#if defined(__APPLE__) || defined(__linux__)
	#define __semaphore_no_inline __attribute__ ((noinline))
#elif defined(_WIN32)
	#define __semaphore_no_inline __declspec(noinline)
    #include <Windows.h>
	#undef min
	#undef max
#else
    #define __semaphore_no_inline
    #define __NO_SEM
#endif

#ifndef __NO_SEM

#if defined(__APPLE__)

    #include <dispatch/dispatch.h>

    #define __SEM_POST_ONE
    static constexpr ptrdiff_t __semaphore_max = std::numeric_limits<long>::max();
    using __semaphore_sem_t = dispatch_semaphore_t;

    inline bool __semaphore_sem_init(__semaphore_sem_t &sem, int init) {
        return (sem = dispatch_semaphore_create(init)) != NULL;
    }
    inline bool __semaphore_sem_destroy(__semaphore_sem_t &sem) {
        assert(sem != NULL);
        dispatch_release(sem);
        return true;
    }
    inline bool __semaphore_sem_post(__semaphore_sem_t &sem, int inc) {
        assert(inc == 1);
        dispatch_semaphore_signal(sem);
        return true;
    }
    inline bool __semaphore_sem_wait(__semaphore_sem_t &sem) {
        return dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0;
    }
    template <class Rep, class Period>
    inline bool __semaphore_sem_wait_timed(__semaphore_sem_t &sem, std::chrono::duration<Rep, Period> const &delta) {
        return dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count())) == 0;
    }

#endif //__APPLE__

#if defined(__linux__)

    #include <unistd.h>
    #include <sched.h>
    #include <semaphore.h>

    #ifndef __NO_SEM_POLL
        #define __NO_SEM_POLL
    #endif
    #define __SEM_POST_ONE
    static constexpr ptrdiff_t __semaphore_max = SEM_VALUE_MAX;
    using __semaphore_sem_t = sem_t;

    inline bool __semaphore_sem_init(__semaphore_sem_t &sem, int init) {
        return sem_init(&sem, 0, init) == 0;
    }
    inline bool __semaphore_sem_destroy(__semaphore_sem_t &sem) {
        return sem_destroy(&sem) == 0;
    }
    inline bool __semaphore_sem_post(__semaphore_sem_t &sem, int inc) {
        assert(inc == 1);
        return sem_post(&sem) == 0;
    }
    inline bool __semaphore_sem_wait(__semaphore_sem_t &sem) {
        return sem_wait(&sem) == 0;
    }
    template <class Rep, class Period>
    inline bool __semaphore_sem_wait_timed(__semaphore_sem_t &sem, std::chrono::duration<Rep, Period> const &delta) {
        struct timespec ts;
        ts.tv_sec = static_cast<long>(std::chrono::duration_cast<std::chrono::seconds>(delta).count());
        ts.tv_nsec = static_cast<long>(std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count());
        return sem_timedwait(&sem, &ts) == 0;
    }

#endif //__linux__

#if defined(_WIN32)

    #define __NO_SEM_BACK

    static constexpr ptrdiff_t __semaphore_max = std::numeric_limits<long>::max();
    using __semaphore_sem_t = HANDLE;

    inline bool __semaphore_sem_init(__semaphore_sem_t &sem, int init) {
        bool const ret = (sem = CreateSemaphore(NULL, init, INT_MAX, NULL)) != NULL;
        assert(ret);
        return ret;
    }
    inline bool __semaphore_sem_destroy(__semaphore_sem_t &sem) {
        assert(sem != NULL);
        return CloseHandle(sem) == TRUE;
    }
    inline bool __semaphore_sem_post(__semaphore_sem_t &sem, int inc) {
        assert(sem != NULL);
        assert(inc > 0);
        return ReleaseSemaphore(sem, inc, NULL) == TRUE;
    }
    inline bool __semaphore_sem_wait(__semaphore_sem_t &sem) {
        assert(sem != NULL);
        return WaitForSingleObject(sem, INFINITE) == WAIT_OBJECT_0;
    }
    template <class Rep, class Period>
    inline bool __semaphore_sem_wait_timed(__semaphore_sem_t &sem, std::chrono::duration<Rep, Period> const &delta) {
        assert(sem != NULL);
        return WaitForSingleObject(sem, (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(delta).count()) == WAIT_OBJECT_0;
    }

#endif // _WIN32

#endif

class __atomic_semaphore_base {

    __semaphore_no_inline inline bool __fetch_sub_if_slow(ptrdiff_t old) {
        while (old != 0) {
            if (count.compare_exchange_weak(old, old - 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }
        return false;
    }
    inline bool __fetch_sub_if_strong() {

        ptrdiff_t old = count.load(std::memory_order_acquire);
        if (old == 0)
            return false;
        if(count.compare_exchange_strong(old, old - 1, std::memory_order_acquire, std::memory_order_relaxed))
            return true;
        return __fetch_sub_if_slow(old); // fail only if not available
    }
    inline bool __fetch_sub_if() {

        ptrdiff_t old = count.load(std::memory_order_acquire);
        if (old == 0)
            return false;
        if(count.compare_exchange_weak(old, old - 1, std::memory_order_acquire, std::memory_order_relaxed))
            return true;
        return __fetch_sub_if_slow(old); // fail only if not available
    }
    __semaphore_no_inline inline void __wait_slow() {
        while (1) {
            ptrdiff_t const old = count.load(std::memory_order_acquire);
            if(old != 0)
                break;
            atomic_wait_explicit(&count, old, std::memory_order_relaxed);
        }
    }
    __semaphore_no_inline inline bool __acquire_slow_timed(std::chrono::nanoseconds const& rel_time) {

        using __clock = std::conditional<std::chrono::high_resolution_clock::is_steady,
                                         std::chrono::high_resolution_clock,
                                         std::chrono::steady_clock>::type;

        auto const start = __clock::now();
        while (1) {
            ptrdiff_t const old = count.load(std::memory_order_acquire);
            if(old != 0 && __fetch_sub_if_slow(old))
                return true;
            auto const elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(__clock::now() - start);
            auto const delta = rel_time - elapsed;
            if(delta <= std::chrono::nanoseconds(0))
                return false;
            auto const sleep = std::min((elapsed.count() >> 2) + 100, delta.count());
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleep));
        }
    }
    std::atomic<ptrdiff_t> count;

public:
    static constexpr ptrdiff_t max() noexcept {
        return std::numeric_limits<ptrdiff_t>::max();
    }

    __atomic_semaphore_base(ptrdiff_t count) : count(count) { }

    ~__atomic_semaphore_base() = default;

    __atomic_semaphore_base(__atomic_semaphore_base const&) = delete;
    __atomic_semaphore_base& operator=(__atomic_semaphore_base const&) = delete;

    inline void release(ptrdiff_t update = 1) {
        count.fetch_add(update, std::memory_order_release);
        if(update > 1)
            atomic_notify_all(&count);
        else
            atomic_notify_one(&count);
    }
    inline void acquire() {
        while (!try_acquire())
            __wait_slow();
    }

    inline bool try_acquire() noexcept {
        return __fetch_sub_if();
    }
    inline bool try_acquire_strong() noexcept {
        return __fetch_sub_if_strong();
    }
    template <class Clock, class Duration>
    inline bool try_acquire_until(std::chrono::time_point<Clock, Duration> const& abs_time) {

        if (try_acquire())
            return true;
        else
            return __acquire_slow_timed(abs_time - Clock::now());
    }
    template <class Rep, class Period>
    inline bool try_acquire_for(std::chrono::duration<Rep, Period> const& rel_time) {

        if (try_acquire())
            return true;
        else
            return __acquire_slow_timed(rel_time);
    }
};

#ifndef __NO_SEM

class __semaphore_base {

    inline void __backfill() {
#ifndef __NO_SEM_BACK
        auto const back_amount = __backbuffer.fetch_sub(2, std::memory_order_acquire);
        bool const post_one = back_amount > 0;
        bool const post_two = back_amount > 1;
        auto const success = (!post_one || __semaphore_sem_post(__semaphore, 1)) &&
                             (!post_two || __semaphore_sem_post(__semaphore, 1));
        (void)success; // Suppress unused variable warnings in builds with asserts off.
        assert(success);
        if(!post_one || !post_two)
            __backbuffer.fetch_add(!post_one ? 2 : 1, std::memory_order_relaxed);
#endif
    }
    inline bool __try_acquire_fast() {
#ifndef __NO_SEM_FRONT
#ifndef __NO_SEM_POLL
        if(__builtin_expect(__frontbuffer.load(std::memory_order_seq_cst) <= 0,0)) {
            using __clock = std::conditional<std::chrono::high_resolution_clock::is_steady,
                                             std::chrono::high_resolution_clock,
                                             std::chrono::steady_clock>::type;
            auto const start = __clock::now();
            while (__frontbuffer.load(std::memory_order_seq_cst) <= 0) {
                auto const elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(__clock::now() - start);
                if(elapsed > std::chrono::microseconds(5))
                    break;
                std::this_thread::sleep_for((elapsed + std::chrono::nanoseconds(100)) / 4);
            }
        }
#endif
        auto const old = __frontbuffer.fetch_sub(1, std::memory_order_seq_cst);
        return old > 0; // after: non-negative
#else
        return false;
#endif
    }
    inline void __try_failed() {
#ifndef __NO_SEM_FRONT
        __frontbuffer.fetch_add(1, std::memory_order_relaxed);
#endif
    }
    __semaphore_no_inline inline void __release_slow(ptrdiff_t post_amount) {
#ifdef __SEM_POST_ONE
    #ifndef __NO_SEM_BACK
        bool const post_one = post_amount > 0;
        bool const post_two = post_amount > 1;
        if(post_amount > 2)
            __backbuffer.fetch_add(post_amount - 2, std::memory_order_acq_rel);
        auto const success = (!post_one || __semaphore_sem_post(__semaphore, 1)) &&
                             (!post_two || __semaphore_sem_post(__semaphore, 1));
        (void)success; // Suppress unused variable warnings in builds with asserts off.
        assert(success);
    #else
        for(; post_amount; --post_amount) {
            auto const success = __semaphore_sem_post(__semaphore, 1);
            (void)success; // Suppress unused variable warnings in builds with asserts off.
            assert(success);
        }
    #endif
#else
        auto const success = __semaphore_sem_post(__semaphore, post_amount);
        (void)success; // Suppress unused variable warnings in builds with asserts off.
        assert(success);
#endif
    }
    __semaphore_no_inline inline bool __try_acquire_for_slow(std::chrono::nanoseconds const& rel_time) {
        auto const result = __semaphore_sem_wait_timed(__semaphore, rel_time);
        if(result)
            __backfill();
        return result;
    }
    __semaphore_sem_t __semaphore;
#ifndef __NO_SEM_FRONT
    std::atomic<ptrdiff_t> __frontbuffer;
#endif
#ifndef __NO_SEM_BACK
    std::atomic<ptrdiff_t> __backbuffer;
#endif

public:
    static constexpr ptrdiff_t max() noexcept {
        return __semaphore_max;
    }

    __semaphore_base(ptrdiff_t count = 0) : __semaphore()
#ifndef __NO_SEM_FRONT
    , __frontbuffer(count)
#endif
#ifndef __NO_SEM_BACK
    , __backbuffer(0)
#endif
    {
        assert(count <= max());
        auto const success =
#ifndef __NO_SEM_FRONT
            __semaphore_sem_init(__semaphore, 0);
#else
            __semaphore_sem_init(__semaphore, count);
#endif
        (void)success; // Suppress unused variable warnings in builds with asserts off.
        assert(success);
    }
    ~__semaphore_base() {
#ifndef __NO_SEM_FRONT
        assert(__frontbuffer.load(std::memory_order_relaxed) >= 0);
#endif
        auto const success = __semaphore_sem_destroy(__semaphore);
        (void)success; // Suppress unused variable warnings in builds with asserts off.
        assert(success);
    }

    __semaphore_base(const __semaphore_base&) = delete;
    __semaphore_base& operator=(const __semaphore_base&) = delete;

    void release(ptrdiff_t update = 1) {
#ifndef __NO_SEM_FRONT
        auto const old = __frontbuffer.fetch_add(update, std::memory_order_acq_rel);
        if(__builtin_expect(old >= 0,1)) // before: non-negative
            return;
        __release_slow(std::min(update, -old));
#else
        __release_slow(update);
#endif
    }
    void acquire() {
        if(__builtin_expect(__try_acquire_fast(),1))
            return;
        auto const success = __semaphore_sem_wait(__semaphore);
        (void)success; // Suppress unused variable warnings in builds with asserts off.
        assert(success);
        __backfill();
    }
    bool try_acquire() noexcept {
        return try_acquire_for(std::chrono::nanoseconds(0));
    }
    template <class Clock, class Duration>
    bool try_acquire_until(std::chrono::time_point<Clock, Duration> const& abs_time) {
        if(__builtin_expect(__try_acquire_fast(),1))
            return true;
        auto const current = std::max(Clock::now(), abs_time);
        auto const result = __try_acquire_for_slow(std::chrono::duration_cast<std::chrono::nanoseconds>(abs_time - current));
        if(!result)
            __try_failed();
        return result;
    }
    template <class Rep, class Period>
    bool try_acquire_for(std::chrono::duration<Rep, Period> const& rel_time) {
        if(__builtin_expect(__try_acquire_fast(),1))
            return true;
        auto const result = __try_acquire_for_slow(std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time));
        if(!result)
            __try_failed();
        return result;
    }
};

#endif //__NO_SEM

namespace std {

template<ptrdiff_t least_max_value>
using semaphore_base =
#ifndef __NO_SEM
    typename std::conditional<least_max_value <= __semaphore_base::max(),
                              __semaphore_base,
                              __atomic_semaphore_base>::type
#else
    __atomic_semaphore_base
#endif
    ;

template<ptrdiff_t least_max_value = INT_MAX>
class counting_semaphore : public semaphore_base<least_max_value> {
    static_assert(least_max_value <= semaphore_base<least_max_value>::max(), "");

public:
    counting_semaphore(ptrdiff_t count = 0) : semaphore_base<least_max_value>(count) { }
    ~counting_semaphore() = default;

    counting_semaphore(const counting_semaphore&) = delete;
    counting_semaphore& operator=(const counting_semaphore&) = delete;
};

#ifdef __NO_SEM

class __binary_semaphore_base {

    __semaphore_no_inline inline bool __acquire_slow_timed(std::chrono::nanoseconds const& rel_time) {

        using __clock = std::conditional<std::chrono::high_resolution_clock::is_steady,
                                         std::chrono::high_resolution_clock,
                                         std::chrono::steady_clock>::type;

        auto const start = __clock::now();
        while (!try_acquire()) {
            auto const elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(__clock::now() - start);
            auto const delta = rel_time - elapsed;
            if(delta <= std::chrono::nanoseconds(0))
                return false;
            auto const sleep = std::min((elapsed.count() >> 2) + 100, delta.count());
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleep));
        }
        return true;
    }
    std::atomic<int> available;

public:
    static constexpr ptrdiff_t max() noexcept { return 1; }

    __binary_semaphore_base(ptrdiff_t available) : available(available) { }

    ~__binary_semaphore_base() = default;

    __binary_semaphore_base(__binary_semaphore_base const&) = delete;
    __binary_semaphore_base& operator=(__binary_semaphore_base const&) = delete;

    void release(ptrdiff_t update = 1) {
        available.store(1, std::memory_order_release);
        atomic_notify_one(&available);
    }
    void acquire() {
        while (!__builtin_expect(try_acquire(), 1))
            atomic_wait_explicit(&available, 0, std::memory_order_relaxed);
    }

    bool try_acquire() noexcept {
        return 1 == available.exchange(0, std::memory_order_acquire);
    }
    template <class Clock, class Duration>
    bool try_acquire_until(std::chrono::time_point<Clock, Duration> const& abs_time) {

        if (__builtin_expect(try_acquire(), 1))
            return true;
        else
            return __acquire_slow_timed(abs_time - Clock::now());
    }
    template <class Rep, class Period>
    bool try_acquire_for(std::chrono::duration<Rep, Period> const& rel_time) {

        if (__builtin_expect(try_acquire(), 1))
            return true;
        else
            return __acquire_slow_timed(rel_time);
    }
};

template<>
class counting_semaphore<1> : public __binary_semaphore_base {
public:
    counting_semaphore(ptrdiff_t count = 0) : __binary_semaphore_base(count) { }
    ~counting_semaphore() = default;

    counting_semaphore(const counting_semaphore&) = delete;
    counting_semaphore& operator=(const counting_semaphore&) = delete;
};

#endif // __NO_SEM

using binary_semaphore = counting_semaphore<1>;

}

#endif
