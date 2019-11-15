#include "doctest.h"
#include "rw/argparse.h"

using namespace std::literals;

    /*
    possible arg types

    bools:
      -x
      --x

      --

      [positional]

    // test cases
    // -c "-a b" -b=c -cde --config "--val1 lslsl" --val2=lsls --val3= --val4 lslslsl -v -c --exe -- -c --config bob --config=bob
    */

/*
std::string_view usage = R"-(
Usage: rwte [options] [-- args]
  -c, --config FILE     overrides config file
  -a, --noalt           disables alt screens
  -f, --font FONT       pango font string
  -g, --geometry GEOM   window geometry; colsxrows, e.g.,
                        \"80x24\" (the default)
  -t, --title TITLE     window title; defaults to rwte
  -n, --name NAME       window name; defaults to $TERM
  -w, --winclass CLASS  overrides window class
  -e, --exe COMMAND     command to execute instead of shell;
                        if specified, any arguments to the
                        command may be specified after a \"--\"
  -o, --out OUT         writes all io to this file;
                        \"-\" means stdout
  -l, --line LINE       use a tty line instead of creating a
                        new pty; LINE is expected to be the
                        device
  -h, --help            show help
  -b, --bench           run config and exit
  -x, --wayland         use wayland rather than xcb

  -v, --version         show version and exit
)-"sv;

int fake_main(int argc, char* argv[]) {
    struct Opts {
        std::string config;
        std::string winclass;
        bool noalt;
        std::string font;
        std::string geometry;
        std::string title;
        std::string name;
        std::string exe;
        std::string out;
        std::string line;
        std::string bench;
        bool wayland;
        bool version;
    };

    auto o = rw::argparse::parser<Opts>{usage}
        .optional(&Opts::config, "config", "c")
        .optional(&Opts::winclass, "winclass", "w")
        .optional(&Opts::noalt, "noalt", "a")
        .optional(&Opts::font, "font", "f")
        .optional(&Opts::geometry, "geometry", "g") // colsxrows, e.g., 80x24
        .optional(&Opts::title, "title", "t")
        .optional(&Opts::name, "name", "n")
        .optional(&Opts::exe, "exe", "e")
        .optional(&Opts::out, "out", "o")
        .optional(&Opts::line, "line", "l")
        .optional(&Opts::bench, "bench", "b")
        .optional(&Opts::wayland, "wayland", "x")
        .usage("help", "h?")
        .optional(&Opts::version, "version", "v")
        .parse(argc, argv);argv
    if (o) {
        // do stuff
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
*/

// todo: test required vs optional
// todo: test int args
// todo: test required vs. optional args
// todo: test conflicting args
// todo: test argument groups, commands
// todo: test positional arguments

TEST_CASE("arg value formats")
{
    SUBCASE("short args") {
        std::string a, b, c, d, e;
        std::array args = {
            "prog",
            "-a", "abc",
            "-b=abc",
            "-c=\"abc\"",
            "-dabc",
            "-e\"abc\"",
        };

        struct NoOpts {};
        auto o = rw::argparse::parser<NoOpts>{}
            .optional(&a, ""sv, "a"sv)
            .optional(&b, ""sv, "b"sv)
            .optional(&c, ""sv, "c"sv)
            .optional(&d, ""sv, "d"sv)
            .optional(&e, ""sv, "e"sv)
            .parse(args.size(), const_cast<char**>(args.data()));

        REQUIRE(o);
        REQUIRE(a == "abc"sv);
        REQUIRE(b == "abc"sv);
        REQUIRE(c == "\"abc\""sv);
        REQUIRE(d == "abc"sv);
        REQUIRE(e == "\"abc\""sv);
    }

    SUBCASE("long args") {
        std::string a, b, c, d, e;
        std::array args = {
            "prog",
            "--a", "abc",
            "--b=abc",
            "--c=\"abc\"",
        };

        struct NoOpts {};
        auto o = rw::argparse::parser<NoOpts>{}
            .optional(&a, "a"sv, ""sv)
            .optional(&b, "b"sv, ""sv)
            .optional(&c, "c"sv, ""sv)
            .parse(args.size(), const_cast<char**>(args.data()));

        REQUIRE(o);
        REQUIRE(a == "abc"sv);
        REQUIRE(b == "abc"sv);
        REQUIRE(c == "\"abc\""sv);
    }
}

TEST_CASE("bool args")
{
    SUBCASE("short pointer storage") {
        bool a = false;

        std::array args = {
            "prog", "-a"
        };

        struct NoOpts {};
        auto o = rw::argparse::parser<NoOpts>{}
            .optional(&a, ""sv, "a"sv)
            .parse(args.size(), const_cast<char**>(args.data()));

        REQUIRE(o);
        REQUIRE(a);
    }

    SUBCASE("pointer storage") {
        bool arg1 = false;

        std::array args = {
            "prog", "--arg1"
        };

        struct NoOpts {};
        auto o = rw::argparse::parser<NoOpts>{}
            .optional(&arg1, "arg1"sv, ""sv)
            .parse(args.size(), const_cast<char**>(args.data()));

        REQUIRE(o);
        REQUIRE(arg1);
    }

    SUBCASE("member storage") {
        struct Opts {
            bool arg1 = false;
        };

        std::array args = {
            "prog", "--arg1"
        };

        // todo: allow no usage string, maybe add usage func
        auto o = rw::argparse::parser<Opts>{}
            .optional(&Opts::arg1, "arg1"sv, ""sv)
            .parse(args.size(), const_cast<char**>(args.data()));

        REQUIRE(o);
        REQUIRE(o->arg1);
    }
}

TEST_CASE("string args")
{
    // short string args are covered by the value formats test

    SUBCASE("pointer storage") {
        std::string arg1;

        std::array args = {
            "prog", "--arg1", "value"
        };

        struct NoOpts {};
        auto o = rw::argparse::parser<NoOpts>{}
            .optional(&arg1, "arg1"sv, ""sv)
            .parse(args.size(), const_cast<char**>(args.data()));

        REQUIRE(o);
        REQUIRE(arg1 == "value"sv);
    }

    SUBCASE("member storage") {
        struct Opts {
            std::string arg1;
        };

        std::array args = {
            "prog", "--arg1", "value"
        };

        // todo: allow no usage string, maybe add usage func
        auto o = rw::argparse::parser<Opts>{}
            .optional(&Opts::arg1, "arg1"sv, ""sv)
            .parse(args.size(), const_cast<char**>(args.data()));

        REQUIRE(o);
        REQUIRE(o->arg1 == "value"sv);
    }

    SUBCASE("sv pointer storage") {
        std::string_view arg1;

        std::array args = {
            "prog", "--arg1", "value"
        };

        struct NoOpts {};
        auto o = rw::argparse::parser<NoOpts>{}
            .optional(&arg1, "arg1"sv, ""sv)
            .parse(args.size(), const_cast<char**>(args.data()));

        REQUIRE(o);
        REQUIRE(arg1 == "value"sv);
    }

    SUBCASE("sv member storage") {
        struct Opts {
            std::string_view arg1;
        };

        std::array args = {
            "prog", "--arg1", "value"
        };

        // todo: allow no usage string, maybe add usage func
        auto o = rw::argparse::parser<Opts>{}
            .optional(&Opts::arg1, "arg1"sv, ""sv)
            .parse(args.size(), const_cast<char**>(args.data()));

        REQUIRE(o);
        REQUIRE(o->arg1 == "value"sv);
    }
}

TEST_CASE("default vals")
{
    struct Opts {
        std::string arg1 = "val1";
        std::string arg2 = "val2";
        std::string arg3 = "val3";
    };

    std::array args = {
        "prog", "--arg1", "notval1", "--arg3", "notval3"
    };

    // todo: allow no usage, maybe add usage func
    auto o = rw::argparse::parser<Opts>{}
        .optional(&Opts::arg1, "arg1"sv, ""sv)
        .optional(&Opts::arg2, "arg2"sv, ""sv)
        .optional(&Opts::arg3, "arg3"sv, ""sv)
        .parse(args.size(), const_cast<char**>(args.data()));

    REQUIRE(o);
    REQUIRE(o->arg1 == "notval1"sv);
    REQUIRE(o->arg2 == "val2"sv);
    REQUIRE(o->arg3 == "notval3"sv);
}
