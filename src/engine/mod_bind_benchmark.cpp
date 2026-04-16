/*
Copyright 2026 Paolo Pastori
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)
*/

#include "startup.h"
#include "native.h"
#include "output.h"

#include <ctime>
#include <cstdlib>


/*
 * Report collected data on stderr every 1<<shift tick() calls.
 */
template<unsigned shift>
struct WatchedCounter
{
    static_assert(shift < 32, "");
    static constexpr size_t mask = 1 << shift;
    bool last_masked = false;
    size_t counter = 0;
    clock_t t0;
    size_t exit_count;

    /*
     * Exit program after num reports.
     */
    WatchedCounter(size_t num = 1) : exit_count(num << shift) { t0 = clock(); }
    void tick()
    {
        if (bool(++counter & mask) != last_masked)
        {
            last_masked = !last_masked;
            clock_t t1 = clock();
            double dur = 1000.0 * (t1 - t0) / CLOCKS_PER_SEC;
            err_printf("%ld\t%.3f\n", counter, dur);
        }
        if (counter == exit_count) b2::clean_exit(EXIT_SUCCESS);
    }
};


/*
 * Legacy binding style.
 */
LIST * native_timed(FRAME * frame, int flags)
{
    //out_puts("LEGACY TIMED\n");
    static WatchedCounter<13> wc(2);
    wc.tick();
    return L0;
}

void init_benchmark()
{
    //char const * args[] = { "any", "*", 0 }; // only used to check for call syntax
    char const * * args = nullptr; // do not care of args
    declare_native_rule(
        "benchmark",
        "timed",
        args,
        native_timed,
        1
    );
}


/*
 * New binding style.
 */
namespace b2 {

// NOTE: returning void seems slower
//void timed_no_args()
value_ref timed_no_args()
{
    //out_puts("NEW BINDING TIMED\n");
    static WatchedCounter<13> wc(2);
    wc.tick();
    return value_ref();
}

value_ref timed_any_args(list_cref args)
{
    static WatchedCounter<13> wc(2);
    wc.tick();
    return value_ref();
}

} // namespace b2
