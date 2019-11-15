
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include "rw/logging.h"

int main(int argc, char** argv)
{
    rw::logging::get("env")->level(rw::logging::log_level::info);

    return doctest::Context(argc, argv).run();
}
