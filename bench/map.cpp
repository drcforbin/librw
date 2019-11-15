#include "../test/map-helpers.h"
#include "nanobench.h"
#include "rw/pdata/map.h"

void bench_map_persistent(ankerl::nanobench::Config& cfg)
{
    std::shared_ptr<rw::pdata::persistent_map<uint64_t, uint64_t>> m;

    auto pairs = randomPairs<uint64_t>(1000);
    cfg.minEpochIterations(10).run("persistent set 1000", [&] {
                                  m = std::make_shared<rw::pdata::persistent_map<uint64_t, uint64_t>>();
                                  m = fillPersistent(m, pairs);
                              })
            .doNotOptimizeAway(&m);

    m.reset();
    auto dupPairs = randomDupPairs(pairs);
    cfg.run("persistent set dup 1000", [&] {
           m = std::make_shared<rw::pdata::persistent_map<uint64_t, uint64_t>>();
           m = fillPersistent(m, pairs);
           m = fillPersistent(m, dupPairs);
       }).doNotOptimizeAway(&m);
}

void bench_map_transient(ankerl::nanobench::Config& cfg)
{
    std::shared_ptr<rw::pdata::persistent_map<uint64_t, uint64_t>> m;

    auto pairs = randomPairs<uint64_t>(1000);
    cfg.run("transient set 1000", [&] {
           m = std::make_shared<rw::pdata::persistent_map<uint64_t, uint64_t>>();
           m = fillTransient(m, pairs);
       }).doNotOptimizeAway(&m);

    m.reset();
    auto dupPairs = randomDupPairs(pairs);
    cfg.run("transient set dup 1000", [&] {
           m = std::make_shared<rw::pdata::persistent_map<uint64_t, uint64_t>>();
           m = fillTransient(m, pairs);
           m = fillTransient(m, dupPairs);
       }).doNotOptimizeAway(&m);

    // todo: is this useful?
    {
        m = std::make_shared<rw::pdata::persistent_map<uint64_t, uint64_t>>();
        m = fillTransient(m, pairs);
        cfg.run("persistent get 1000", [&] {
               check(m, pairs);
           }).doNotOptimizeAway(&m);
    }
}
