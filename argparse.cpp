#include "fmt/core.h"
#include "rw/argparse.h"

#include <cstdlib>

namespace rw::argparse {

using namespace std::literals;

namespace detail {

void Reporter::conflict(std::string_view arg1, std::string_view arg2) const {
    fmt::print(stderr, "arg '{}' conflicts with arg '{}'\n",
            arg1, arg2);
}

void Reporter::unexpected(std::string_view name) const {
    fmt::print(stderr, "arg '{}' is not expected\n", name);
}

void Reporter::missing_required_of() const {
    fmt::print(stderr, "required at least one arg\n");
}

void Reporter::missing_required(std::string_view name) const {
    fmt::print(stderr, "arg '{}' is required\n");
}

void Reporter::missing_value(std::string_view name) const {
    fmt::print(stderr, "required value for arg '{}'\n", name);
}

// [[noreturn]]
void Reporter::usage(bool ok, std::string_view text) const {
    // hate to put any functionality in here (so it doesn't need
    // testing), but trimming usage text makes it a little nicer
    // when using multiline strings, without messing up output
    // with extra lines
    std::size_t first = text.find_first_not_of(" \r\n");
    if (first == std::string_view::npos) {
        first = 0;
    }
    std::size_t last = text.find_last_not_of(" \r\n");
    if (last == std::string_view::npos) {
        last = text.size();
    }

    fmt::print(ok? stdout : stderr, text.substr(first, (last+1)-first));
    fmt::print(ok? stdout : stderr, "\n");

    if (ok) {
        std::exit(EXIT_SUCCESS);
    } else {
        std::exit(EXIT_FAILURE);
    }
}

parsed_arg parse_arg(std::string_view arg)
{
    if (arg.size() >= 2 && arg[0] == '-') {
        if (arg[1] == '-') {
            if (arg.size() == 2) {
                // end of named
                return {true, arg, {}};
            } else {
                // long

                // check for space or =
                if (auto idx = arg.find_first_of("= "sv);
                        idx != std::string_view::npos) {
                    return {true, arg.substr(2, idx - 2), arg.substr(idx + 1)};
                } else {
                    return {true, arg.substr(2), {}};
                }
            }
        } else {
            // short
            if (arg.size() > 2) {
                // check for space, =, or following chars
                if (arg[2] == '=' || arg[2] == ' ') {
                    return {false, arg.substr(1, 1), arg.substr(3)};
                } else {
                    return {false, arg.substr(1, 1), arg.substr(2)};
                }
            }
            return {false, arg.substr(1, 1), {}};
        }
    }

    // maybe positional
    return {false, {}, arg};
}

} // namespace detail
} // namespace rw::argparse
