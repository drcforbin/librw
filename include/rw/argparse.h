#ifndef RW_ARGPARSE_H
#define RW_ARGPARSE_H

#include "fmt/format.h"

#include <cassert>
#include <cstdlib>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace rw::argparse {

using namespace std::literals;

// forward
template <typename T, typename Reporter> class parser;

namespace detail {

struct Reporter {
    void conflict(std::string_view arg1, std::string_view arg2) const {
        fmt::print(stderr, "arg '{}' conflicts with arg '{}'\n",
                arg1, arg2);
    }

    void unexpected(std::string_view name) const {
        fmt::print(stderr, "arg '{}' is not expected\n", name);
    }

    void missing_required_of() const {
        fmt::print(stderr, "required at least one arg\n");
    }

    void missing_required(std::string_view name) const {
        fmt::print(stderr, "arg '{}' is required\n");
    }

    void missing_value(std::string_view name) const {
        fmt::print(stderr, "required value for arg '{}'\n", name);
    }

    [[noreturn]]
    void usage(bool ok, std::string_view text) const {
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
};

template <typename T>
struct ArgBase
{
    bool required = false;

    bool needs_value() const {
        return std::holds_alternative<String>(arg);
    }

    void set_value(T& o, std::string_view val) const {
        std::visit(
                [ &o, &val ](auto&& arg) -> auto {
                    using V = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<V, Bool>) {
                        // everthing else (incl empty) is true.
                        bool bval = val != "false" && val != "0";

                        std::visit(
                                [ &o, bval ](auto&& arg) -> auto {
                                    using V = std::decay_t<decltype(arg)>;
                                    if constexpr (std::is_same_v<V, bool*>) {
                                        *arg = bval;
                                    } else if constexpr (std::is_same_v<V, bool T::*>) {
                                        o.*arg = bval;
                                    }
                                },
                                arg);
                    } else if constexpr (std::is_same_v<V, String>) {
                        std::visit(
                                [ &o, &val ](auto&& arg) -> auto {
                                    using V = std::decay_t<decltype(arg)>;
                                    if constexpr (std::is_same_v<V, std::string_view*>) {
                                        *arg = val;
                                    } else if constexpr (std::is_same_v<V, std::string_view T::*>) {
                                        o.*arg = val;
                                    } else if constexpr (std::is_same_v<V, std::string*>) {
                                        *arg = std::string(val);
                                    } else if constexpr (std::is_same_v<V, std::string T::*>) {
                                        o.*arg = std::string(val);
                                    }
                                },
                                arg);
                    }
                },
                arg);
    }

protected:
    template <typename V>
    constexpr ArgBase(V* value, bool required) :
        required(required),
        arg(value)
    {
    }

    template <typename MemberT>
    constexpr ArgBase(MemberT T::*member, bool required) :
        required(required),
        arg(member)
    {
    }

private:
    using Bool = std::variant<bool*, bool T::*>;
    using String = std::variant<std::string_view*, std::string_view T::*, std::string*, std::string T::*>;

    std::variant<Bool, String> arg;
};

template <typename T>
struct PosArg : public ArgBase<T>
{
    template <typename V>
    constexpr PosArg(V* value, std::string_view name, bool required) :
        ArgBase<T>(value, required),
        name(name)
    {
    }

    template <typename MemberT>
    constexpr PosArg(MemberT T::*member, std::string_view name, bool required) :
        ArgBase<T>(member, required),
        name(name)
    {
    }

    std::string_view name;
};

template <typename T>
struct Arg : public ArgBase<T>
{
    template <typename V>
    constexpr Arg(V* value, std::string_view longname, std::string_view shortnames,
            bool required) :
        ArgBase<T>(value, required), longname(longname), shortnames(shortnames)
    {
    }

    template <typename MemberT>
    constexpr Arg(MemberT T::*member, std::string_view longname,
            std::string_view shortnames, bool required) :
        ArgBase<T>(member, required), longname(longname), shortnames(shortnames)
    {
    }

    std::string_view longname;
    std::string_view shortnames;
};

template<typename T, typename Reporter>
struct parse_state
{
    parse_state(std::size_t num_args, Reporter& reporter) :
        reporter(reporter),
        pos_satisfied(num_args),
        satisfied(num_args)
    {
    }

    Reporter& reporter;

    bool end_of_named = false;
    std::optional<std::size_t> waiting_idx;

    std::size_t pos_count = 0;

    // todo: combine if args + pos_args combined
    std::vector<int> pos_satisfied;
    std::vector<int> satisfied;

    T o;
};

template <typename Derived, typename T, typename Reporter>
class sub_group
{
public:
    constexpr sub_group(parser<T, Reporter>& p) :
        p(p)
    {
    }

    // todo: add required

    template <typename V>
    Derived& optional(V&& pval, std::string_view longname,
            std::string_view shortnames = ""sv) {
        p.optional(pval, longname, shortnames);
        group.push_back(p.args.size()-1);
        return static_cast<Derived&>(*this);
    }

protected:
    parser<T, Reporter>& p;
    std::vector<std::size_t> group;
};

} // namespace detail

template <typename T, typename Reporter>
class excl_group :
    public detail::sub_group<excl_group<T, Reporter>, T, Reporter>
{
public:
    excl_group(parser<T, Reporter>& p, bool required) :
        detail::sub_group<excl_group<T, Reporter>, T, Reporter>(p),
        required(required)
    {
    }

protected:
    friend class parser<T, Reporter>;

    bool satisfied(const detail::parse_state<T, Reporter>& state) const {
        std::optional<std::size_t> found_arg;
        for (auto idx : this->group) {
            if (state.satisfied[idx]) {
                if (!found_arg) {
                    found_arg = idx;
                } else {
                    // todo: better message! arg name?
                    state.reporter.conflict(
                            this->p.args[idx].longname,
                            this->p.args[*found_arg].longname);
                    return false;
                }
            }
        }

        if (required && !found_arg) {
            // todo: include args in message
            state.reporter.missing_required_of();
            return false;
        } else {
            return true;
        }
    }

    bool required;
};

template <typename T, typename Reporter = detail::Reporter>
class parser
{
public:
    parser& usage(std::string_view usage,
            std::string_view longname = "help"sv,
            std::string_view shortnames = "?h") {
        usage_text = usage;
        return optional(&show_usage, longname, shortnames);
    }

    template <typename V>
    parser& positional(V&& pval, std::string_view name, bool required = true) {
        // name is required
        assert(!name.empty());

        #ifdef NDEBUG
        if (required) {
            for (auto it = pos_args.crbegin(); it != pos_args.crend(); ++it) {
                if (!it->required) {
                    // it doesn't make sense to add a required positional
                    // argument after an optional positional argument. what
                    // do you even expect to happen with the optional one?
                    assert(false);
                }
            }
        }
        #endif

        pos_args.emplace_back(std::forward<V&&>(pval), name, required);
        return *this;
    }

    // todo: add required

    template <typename V>
    parser& optional(V&& pval, std::string_view longname,
            std::string_view shortnames = ""sv) {
        // longname is required
        assert(!longname.empty());

        args.emplace_back(std::forward<V&&>(pval), longname, shortnames, false);
        return *this;
    }

    excl_group<T, Reporter>& one_of(bool required = false) {
        subs.emplace_back(*this, required);
        return subs.back();
    }

    std::optional<T> parse(const int argc, char** const argv, Reporter& reporter) const;

    std::optional<T> parse(const int argc, char** const argv) const {
        Reporter reporter;
        return parse(argc, argv, reporter);
    }

private:
    friend class excl_group<T, Reporter>;
    friend class detail::sub_group<excl_group<T, Reporter>, T, Reporter>;

    using PosArg = detail::PosArg<T>;
    using Arg = detail::Arg<T>;

    // todo: rename
    using arg_pair = std::tuple<bool, std::string_view, std::string_view>;

    arg_pair parse_arg(std::string_view arg) const
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

    bool handle_short(detail::parse_state<T, Reporter>& state, std::string_view name,
            std::string_view val) const;
    bool handle_long(detail::parse_state<T, Reporter>& state, std::string_view name,
            std::string_view val) const;

    void set_pos_value(detail::parse_state<T, Reporter>& state, std::size_t idx,
            std::string_view val) const;
    void set_value(detail::parse_state<T, Reporter>& state, std::size_t idx,
            std::string_view val) const;

    std::string_view usage_text;

    // todo: can't just be excl_group, need to support other groups
    std::vector<excl_group<T, Reporter>> subs;

    // todo: combine, w/ variant?
    std::vector<PosArg> pos_args;
    std::vector<Arg> args;

    // todo: this shouldn't be part of the parser, to allow
    // for parsing multiple times.
    bool show_usage = false;
};

template <typename T, typename Reporter>
std::optional<T> parser<T, Reporter>::parse(const int argc, char** const argv, Reporter& reporter) const
{
    detail::parse_state<T, Reporter> state{args.size(), reporter};

    bool ok = true;
    if (argc > 1) {
        for (int i = 1; ok && i < argc; ++i) {
            if (!state.end_of_named) {
                auto [islong, name, val] = parse_arg(argv[i]);
                if (name.size() == 0) {
                    // positional
                    if (state.waiting_idx) {
                        set_value(state, *state.waiting_idx, val);
                        state.waiting_idx.reset();
                    } else {
                        if (state.pos_count < pos_args.size()) {
                            set_pos_value(state, state.pos_count++, val);
                        } else {
                            // todo: positional args "rest"
                            state.reporter.unexpected(val);
                            ok = false;
                        }
                    }
                } else {
                    if (state.waiting_idx) {
                        state.reporter.missing_value(args[*state.waiting_idx].longname);
                        ok = false;
                    } else {
                        if (!((islong && handle_long(state, name, val)) ||
                                 (!islong && handle_short(state, name, val)))) {
                            ok = false;
                        }
                    }
                }
            } else {
                if (state.pos_count < pos_args.size()) {
                    set_pos_value(state, state.pos_count++, argv[i]);
                } else {
                    // todo: positional args "rest"
                    state.reporter.unexpected(argv[i]);
                    ok = false;
                }
            }
        }

        if (ok && state.waiting_idx) {
            state.reporter.missing_value(args[*state.waiting_idx].longname);
            ok = false;
        }
    }

    for (std::size_t idx = 0; idx < pos_args.size(); ++idx) {
        if (pos_args[idx].required && !state.pos_satisfied[idx]) {
            state.reporter.missing_required(pos_args[idx].name);
            ok = false;
        }
    }

    for (std::size_t idx = 0; idx < args.size(); ++idx) {
        if (args[idx].required && !state.satisfied[idx]) {
            state.reporter.missing_required(args[idx].longname);
            ok = false;
        }
    }

    if (ok) {
        for (auto& sub : subs) {
            if (!sub.satisfied(state)) {
                ok = false;
                break;
            }
        }
    }

    if (!ok || show_usage) {
        state.reporter.usage(ok, usage_text);

        // the reporter usage call may exit, maybe not.
        return std::nullopt;
    }

    return state.o;
}

template <typename T, typename Reporter>
bool parser<T, Reporter>::handle_short(detail::parse_state<T, Reporter>& state,
        std::string_view name, std::string_view val) const
{
    for (std::size_t idx = 0; idx < args.size(); ++idx) {
        auto& arg = args[idx];
        if (!arg.shortnames.empty() &&
                arg.shortnames.find(name[0]) != std::string_view::npos) {
            // todo: should consolidate this with handle_long's
            if (arg.needs_value()) {
                if (val.empty()) {
                    // todo: what about -x= (with no value?)
                    state.waiting_idx = idx;
                } else {
                    set_value(state, idx, val);
                }
            } else {
                // bools can just be set.
                // todo: better true/false/empty handling..
                // --arg= really should be false, not true
                set_value(state, idx, "true"sv);
            }
            return true;
        }
    }

    state.reporter.unexpected(name);
    return false;
}

template <typename T, typename Reporter>
bool parser<T, Reporter>::handle_long(detail::parse_state<T, Reporter>& state,
        std::string_view name, std::string_view val) const
{
    if (name != "--"sv) {
        for (std::size_t idx = 0; idx < args.size(); ++idx) {
            auto& arg = args[idx];
            if (!arg.longname.empty() && arg.longname == name) {
                // todo: should consolidate this with handle_short's
                if (arg.needs_value()) {
                    if (val.empty()) {
                        // todo: what about --xzy= (with no value?)
                        state.waiting_idx = idx;
                    } else {
                        set_value(state, idx, val);
                    }
                } else {
                    // bools can just be set.
                    // todo: better true/false/empty handling..
                    // --arg= really should be false, not true
                    set_value(state, idx, "true"sv);
                }
                return true;
            }
        }

        state.reporter.unexpected(name);
        return false;
    } else {
        state.end_of_named = true;
        return true;
    }
}

template <typename T, typename Reporter>
void parser<T, Reporter>::set_pos_value(detail::parse_state<T, Reporter>& state, std::size_t idx,
        std::string_view val) const
{
    auto& arg = pos_args[idx];
    arg.set_value(state.o, val);
    state.pos_satisfied[idx] = true;
}

template <typename T, typename Reporter>
void parser<T, Reporter>::set_value(detail::parse_state<T, Reporter>& state, std::size_t idx,
        std::string_view val) const
{
    auto& arg = args[idx];
    arg.set_value(state.o, val);
    state.satisfied[idx] = true;
}

} // namespace rw::argparse

#endif // RW_ARGPARSE_H
