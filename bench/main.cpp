#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "rw/logging.h"
#include "rw/utf8.h"

#define LOGGER() (rw::logging::get("librw-bench"))

// todo: better way than extern.

extern void bench_map_persistent(ankerl::nanobench::Config& cfg);
extern void bench_map_transient(ankerl::nanobench::Config& cfg);
extern void bench_utf8_encoding(ankerl::nanobench::Config& cfg);
extern void bench_utf8_decoding(ankerl::nanobench::Config& cfg);

int main()
{
    rw::utf8::set_locale();

    auto cfg = ankerl::nanobench::Config();

    bench_map_persistent(cfg);
    bench_map_transient(cfg);
    bench_utf8_encoding(cfg);
    bench_utf8_decoding(cfg);
}
