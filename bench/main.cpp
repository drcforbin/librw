#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "rw/logging.h"

// todo: better way than extern.

extern void bench_map_persistent(ankerl::nanobench::Config& cfg);
extern void bench_map_transient(ankerl::nanobench::Config& cfg);

int main()
{
    rw::logging::get("env")->level(rw::logging::log_level::off);

    auto cfg = ankerl::nanobench::Config();

    bench_map_persistent(cfg);
    bench_map_transient(cfg);
}
