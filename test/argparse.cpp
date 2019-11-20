#include "doctest.h"
#include "rw/argparse.h"

using namespace std::literals;

// todo: test int args

enum {
    called_nothing = 0,
    called_conflict = 1,
    called_unexpected = 2,
    called_missing_required_of = 4,
    called_missing_required = 8,
    called_missing_value = 16,
    called_usage = 32,
};

struct TestReporter {
    void conflict(std::string_view arg1, std::string_view arg2) {
        this->arg1 = arg1;
        this->arg2 = arg2;
        calls |= called_conflict;
    }

    void unexpected(std::string_view name) {
        arg1 = name;
        calls |= called_unexpected;
    }

    void missing_required_of() {
        calls |= called_missing_required_of;
    }

    void missing_required(std::string_view name) {
        arg1 = name;
        calls |= called_missing_required;
    }

    void missing_value(std::string_view name) {
        arg1 = name;
        calls |= called_missing_value;
    }

    void usage(bool ok, std::string_view text) {
        // should only call usage once
        REQUIRE((calls & called_usage) == 0);

        argok = ok;
        arg1 = text;
        calls |= called_usage;
    }

    int calls = called_nothing;

    bool argok = false;
    std::string_view arg1, arg2;
};

template <typename T, typename R>
static void require_parse_success(std::optional<T>& o, R& tr) {
    // options should be returned, and no reporting calls
    // should have been made

    REQUIRE(o);
    REQUIRE(tr.calls == called_nothing);
}

template <typename T, typename R>
static void require_parse_err(std::optional<T>& o, R& tr, int expected_calls) {
    // no options should have been returned, all errors should include
    // usage and the expected call, and the usage call should include
    // an error flag

    REQUIRE(!o);
    REQUIRE(tr.calls == (expected_calls | called_usage));
    REQUIRE(!tr.argok);
}

TEST_CASE("excl args")
{
    std::string a, b, c, d;
    TestReporter tr;

    auto p = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{};
    p.one_of()
        .optional(&a, "longa"sv, "a"sv)
        .optional(&b, "longb"sv, "b"sv);
    p.one_of(true)
        .optional(&c, "longc"sv, "c"sv)
        .optional(&d, "longd"sv, "d"sv);

    SUBCASE("conflict optional short") {
        std::array args = {"prog", "-a=abc", "-b=abc"};
        auto o = p.parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_err(o, tr, called_conflict);
    }

    SUBCASE("conflict optional long") {
        std::array args = {"prog", "--longa=abc", "--longb=abc"};
        auto o = p.parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_err(o, tr, called_conflict);
    }

    SUBCASE("conflict required long") {
        std::array args = {"prog", "--longc=abc", "--longd=abc"};
        auto o = p.parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_err(o, tr, called_conflict);
    }

    SUBCASE("required present") {
        std::array args = {"prog", "--longd=abc"};
        auto o = p.parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(d == "abc");
    }

    SUBCASE("required missing") {
        std::array args = {"prog", "--longa=abc"};
        auto o = p.parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_err(o, tr, called_missing_required_of);
    }
}

TEST_CASE("arg value formats")
{
    std::string a, b, c, d, e;
    TestReporter tr;

    SUBCASE("short args") {
        std::array args = {
            "prog",
            "-a", "abc",
            "-b=abc",
            "-c=\"abc\"",
            "-dabc",
            "-e\"abc\"",
        };

        auto o = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{}
            .optional(&a, "nota"sv, "a"sv)
            .optional(&b, "notb"sv, "b"sv)
            .optional(&c, "notc"sv, "c"sv)
            .optional(&d, "notd"sv, "d"sv)
            .optional(&e, "note"sv, "e"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(a == "abc"sv);
        REQUIRE(b == "abc"sv);
        REQUIRE(c == "\"abc\""sv);
        REQUIRE(d == "abc"sv);
        REQUIRE(e == "\"abc\""sv);
    }

    SUBCASE("long args") {
        std::array args = {
            "prog",
            "--a", "abc",
            "--b=abc",
            "--c=\"abc\"",
        };

        auto o = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{}
            .optional(&a, "a"sv)
            .optional(&b, "b"sv)
            .optional(&c, "c"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(a == "abc"sv);
        REQUIRE(b == "abc"sv);
        REQUIRE(c == "\"abc\""sv);
    }
}

TEST_CASE("bool args")
{
    bool a = false;
    TestReporter tr;

    SUBCASE("short pointer storage") {
        std::array args = {"prog", "-a"};

        auto o = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{}
            .optional(&a, "nota"sv, "a"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(a);
    }

    SUBCASE("pointer storage") {
        std::array args = {"prog", "--arg1"};

        auto o = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{}
            .optional(&a, "arg1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(a);
    }

    SUBCASE("member storage") {
        struct Opts {
            bool arg1 = false;
        };

        std::array args = {"prog", "--arg1"};

        // todo: allow no usage string, maybe add usage func
        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(o->arg1);
    }
}

TEST_CASE("string args")
{
    // short string args are covered by the value formats test

    TestReporter tr;

    SUBCASE("pointer storage") {
        std::string arg1;

        std::array args = {"prog", "--arg1", "value"};

        auto o = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{}
            .optional(&arg1, "arg1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(arg1 == "value"sv);
    }

    SUBCASE("member storage") {
        struct Opts {
            std::string arg1;
        };

        std::array args = {"prog", "--arg1", "value"};

        // todo: allow no usage string, maybe add usage func
        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(o->arg1 == "value"sv);
    }

    SUBCASE("sv pointer storage") {
        std::string_view arg1;

        std::array args = {"prog", "--arg1", "value"};

        auto o = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{}
            .optional(&arg1, "arg1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

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
        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

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
    TestReporter tr;
    auto o = rw::argparse::parser<Opts, TestReporter>{}
        .optional(&Opts::arg1, "arg1"sv)
        .optional(&Opts::arg2, "arg2"sv)
        .optional(&Opts::arg3, "arg3"sv)
        .parse(args.size(), const_cast<char**>(args.data()), tr);
    require_parse_success(o, tr);

    REQUIRE(o->arg1 == "notval1"sv);
    REQUIRE(o->arg2 == "val2"sv);
    REQUIRE(o->arg3 == "notval3"sv);
}

TEST_CASE("required args")
{
    std::string a, b;
    TestReporter tr;

    auto p = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{}
        .required(&a, "arg1"sv)
        .optional(&b, "arg2"sv);

    SUBCASE("required and optional provided") {
        std::array both_args = {"prog", "--arg1", "abc", "--arg2", "def"};
        auto o = p.parse(both_args.size(), const_cast<char**>(both_args.data()), tr);
        require_parse_success(o, tr);
    }

    SUBCASE("required provided") {
        std::array arg1_only = {"prog", "--arg1", "abc"};
        auto o = p.parse(arg1_only.size(), const_cast<char**>(arg1_only.data()), tr);
        require_parse_success(o, tr);
    }

    SUBCASE("only optional provided") {
        std::array arg2_only = {"prog", "--arg2", "def"};
        auto o = p.parse(arg2_only.size(), const_cast<char**>(arg2_only.data()), tr);
        require_parse_err(o, tr, called_missing_required);
    }

    SUBCASE("no args provided") {
        std::array no_args = {"prog"};
        auto o = p.parse(no_args.size(), const_cast<char**>(no_args.data()), tr);
        require_parse_err(o, tr, called_missing_required);
    }
}

TEST_CASE("usage arg")
{
    std::string a, b;
    TestReporter tr;

    SUBCASE("usage not provided") {
        auto p = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{}
            .optional(&a, "longa"sv, "a"sv)
            .optional(&b, "longb"sv, "b"sv);

        std::array args = {"prog", "--help"};

        auto o = p.parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_err(o, tr, called_unexpected);
    }

    SUBCASE("usage provided") {
        auto p = rw::argparse::parser<rw::argparse::no_options_type, TestReporter>{}
            .optional(&a, "longa"sv, "a"sv)
            .optional(&b, "longb"sv, "b"sv)
            .usage("some kind of usage text."sv);

        // each of these is expected to return no options,
        // to call usage, and the usage call should indicate it
        // was not called in error

        // check w/ long arg
        std::array args = {"prog", "--help"};
        auto o = p.parse(args.size(), const_cast<char**>(args.data()), tr);

        REQUIRE(!o);
        REQUIRE(tr.calls == called_usage);
        REQUIRE(tr.argok);

        // short 1
        args[1] = "-h";
        tr.calls = called_nothing;
        o = p.parse(args.size(), const_cast<char**>(args.data()), tr);

        REQUIRE(!o);
        REQUIRE(tr.calls == called_usage);
        REQUIRE(tr.argok);

        // short 2
        args[1] = "-?";
        tr.calls = called_nothing;
        o = p.parse(args.size(), const_cast<char**>(args.data()), tr);

        REQUIRE(!o);
        REQUIRE(tr.calls == called_usage);
        REQUIRE(tr.argok);
    }
}

TEST_CASE("positional args")
{
    struct Opts {
        std::string arg1 = "val1";
        std::string pos1 = "pos1";
    };

    TestReporter tr;

    SUBCASE("unexpected positional") {
        std::array args = {"prog", "notpos1", "--arg1", "notval1"};

        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_err(o, tr, called_unexpected);
    }

    SUBCASE("positional at end") {
        std::array args = {"prog", "--arg1", "notval1", "notpos1"};

        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .positional(&Opts::pos1, "pos1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(o->arg1 == "notval1"sv);
        REQUIRE(o->pos1 == "notpos1"sv);
    }

    SUBCASE("positional at beginning") {
        std::array args = {
            "prog", "notpos1", "--arg1", "notval1"
        };

        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .positional(&Opts::pos1, "pos1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(o->arg1 == "notval1"sv);
        REQUIRE(o->pos1 == "notpos1"sv);
    }

    SUBCASE("required positional missing") {
        std::array args = {"prog", "--arg1", "notval1"};

        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .positional(&Opts::pos1, "pos1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_err(o, tr, called_missing_required);
    }

    SUBCASE("optional positional missing") {
        std::array args = {"prog", "--arg1", "notval1"};

        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .positional(&Opts::pos1, "pos1"sv, false)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(o->arg1 == "notval1"sv);
        REQUIRE(o->pos1 == "pos1"sv);
    }

}

TEST_CASE("extra args")
{
    struct Opts {
        std::string arg1 = "val1";
        std::string pos1 = "pos1";
        std::vector<std::string> extra_strings;
        std::vector<std::string_view> extra_string_views;
    };

    TestReporter tr;

    std::array args = {
        "prog", "notpos1", "--arg1", "notval1", "extra1", "extra2"
    };

    SUBCASE("unexpected extras") {
        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .positional(&Opts::pos1, "pos1"sv)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_err(o, tr, called_unexpected);
    }

    SUBCASE("extras stored in option class") {
        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .positional(&Opts::pos1, "pos1"sv)
            .extra(&Opts::extra_strings)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(o->arg1 == "notval1"sv);
        REQUIRE(o->pos1 == "notpos1"sv);
        REQUIRE(o->extra_strings.size() == 2);
        REQUIRE(o->extra_strings[0] == "extra1");
        REQUIRE(o->extra_strings[1] == "extra2");

        o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .positional(&Opts::pos1, "pos1"sv)
            .extra(&Opts::extra_string_views)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(o->arg1 == "notval1"sv);
        REQUIRE(o->pos1 == "notpos1"sv);
        REQUIRE(o->extra_string_views.size() == 2);
        REQUIRE(o->extra_string_views[0] == "extra1");
        REQUIRE(o->extra_string_views[1] == "extra2");
    }

    SUBCASE("extras stored in pointer") {
        std::vector<std::string> extra_strings;
        auto o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .positional(&Opts::pos1, "pos1"sv)
            .extra(&extra_strings)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(o->arg1 == "notval1"sv);
        REQUIRE(o->pos1 == "notpos1"sv);
        REQUIRE(extra_strings.size() == 2);
        REQUIRE(extra_strings[0] == "extra1");
        REQUIRE(extra_strings[1] == "extra2");

        std::vector<std::string_view> extra_string_views;
        o = rw::argparse::parser<Opts, TestReporter>{}
            .optional(&Opts::arg1, "arg1"sv)
            .positional(&Opts::pos1, "pos1"sv)
            .extra(&extra_string_views)
            .parse(args.size(), const_cast<char**>(args.data()), tr);
        require_parse_success(o, tr);

        REQUIRE(o->arg1 == "notval1"sv);
        REQUIRE(o->pos1 == "notpos1"sv);
        REQUIRE(extra_string_views.size() == 2);
        REQUIRE(extra_string_views[0] == "extra1");
        REQUIRE(extra_string_views[1] == "extra2");
    }
}
