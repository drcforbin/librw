#ifndef RW_ARGPARSE_H
#define RW_ARGPARSE_H

#include "fmt/format.h"

#include <cstdlib>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

// todo: test exclusive args
// todo: test exclusive required args
// todo: test usage

namespace rw::argparse {

using namespace std::literals;

// forward
template <typename T> class parser;

namespace detail {

template <typename T>
struct Arg
{
    template <typename V>
    Arg(V* value, std::string_view longname, std::string_view shortnames) :
        argument(value), longname(longname), shortnames(shortnames)
    {
    }

    template <typename MemberT>
    Arg(MemberT T::*member, std::string_view longname,
            std::string_view shortnames) :
        argument(member), longname(longname), shortnames(shortnames)
    {
    }

    using BoolVariant = std::variant<bool*, bool T::*>;
    using StringVariant = std::variant<std::string_view*, std::string_view T::*, std::string*, std::string T::*>;

    using ArgumentVariant = std::variant< // IntVariant,
            BoolVariant, StringVariant>;

    ArgumentVariant argument;
    bool required = false;
    std::string_view longname;
    std::string_view shortnames;
};

template<typename T>
struct parse_state
{
    parse_state(std::size_t num_args) :
        satisfied(num_args)
    {
    }

    bool end_of_named = false;
    std::optional<std::size_t> waiting_idx;

    std::vector<int> satisfied;

    T o;
};

template <typename Derived, typename T>
class parser_common
{
public:
    template <typename V>
    Derived& optional(V&& pval, std::string_view longname,
            std::string_view shortnames = ""sv) {
        return static_cast<Derived*>(this)->core_optional(pval,
                longname, shortnames);
    }

protected:
    template <typename V>
    Derived& core_optional(V&& pval, std::string_view longname,
            std::string_view shortnames) {
        return *this;
    }
};

template <typename Derived, typename T>
class sub_group : public parser_common<sub_group<Derived, T>, T>
{
public:
    sub_group(parser<T>& p) :
        p(p)
    {
    }

protected:
    friend class detail::parser_common<sub_group<Derived, T>, T>;

    template <typename V>
    sub_group& core_optional(V&& pval, std::string_view longname, std::string_view shortnames)
    {
        p.optional(pval, longname, shortnames);
        group.push_back(p.args.size()-1);
        return *this;
    }

    parser<T>& p;
    std::vector<std::size_t> group;
};

} // namespace detail

template <typename T>
class excl_group : public detail::sub_group<excl_group<T>, T>
{
public:
    excl_group(parser<T>& p, bool required) :
        detail::sub_group<excl_group<T>, T>(p),
        required(required)
    {
    }

protected:
    friend class parser<T>;

    bool satisfied(detail::parse_state<T>& state) {
        std::optional<std::size_t> found_arg;
        for (std::size_t idx = 0; idx < this->group.size(); ++idx) {
            if (state.satisfied[idx]) {
                if (!found_arg) {
                    found_arg = idx;
                } else {
                    // todo: better message! arg name?
                    fmt::print("arg --{} conflicts with arg --{}\n",
                            this->p.args[idx].longname,
                            this->p.args[*found_arg].longname);
                    return false;
                }
            }
        }

        if (required && !found_arg) {
            // todo: include args in message
            fmt::print("required at least one arg\n");
            return false;
        } else {
            return true;
        }
    }

    bool required;
};

template <typename T>
class parser : public detail::parser_common<parser<T>, T>
{
    using Base = detail::parser_common<parser<T>, T>;

public:
    parser& usage(std::string_view usage,
            std::string_view longname = "help"sv,
            std::string_view shortnames = "?h") {
        usage_text = usage;
        return Base::optional(&show_usage, longname, shortnames);
    }

    excl_group<T>& one_of(bool required = false) {
        subs.emplace_back(*this, required);
        return subs.back();
    }

    std::optional<T> parse(const int argc, char** const argv);

protected:
    template <typename V>
    parser& core_optional(V&& pval, std::string_view longname, std::string_view shortnames)
    {
        args.emplace_back(std::forward<V&&>(pval), longname, shortnames);
        return *this;
    }

private:
    friend Base;
    friend class excl_group<T>;
    friend class detail::sub_group<excl_group<T>, T>;

    using Arg = detail::Arg<T>;
    using BoolVariant = typename detail::Arg<T>::BoolVariant;
    using StringVariant = typename detail::Arg<T>::StringVariant;

    // todo: rename
    using arg_pair = std::tuple<bool, std::string_view, std::string_view>;

    arg_pair parse_arg(std::string_view arg)
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

    bool handle_short(detail::parse_state<T>& state, std::string_view name, std::string_view val);
    bool handle_long(detail::parse_state<T>& state, std::string_view name, std::string_view val);

    void set_value(detail::parse_state<T>& state, std::size_t idx,
            std::string_view val);

    std::string_view usage_text;
    std::vector<excl_group<T>> subs; // todo: can't just be excl_group
    std::vector<Arg> args;

    // todo: this shouldn't be part of the parser, to allow
    // for parsing multiple times.
    bool show_usage = false;
};

template <typename T>
std::optional<T> parser<T>::parse(const int argc, char** const argv)
{
    detail::parse_state<T> state{args.size()};

    bool ok = true;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (!state.end_of_named) {
                auto [islong, name, val] = parse_arg(argv[i]);
                if (name.size() == 0) {
                    // positional
                    if (state.waiting_idx) {
                        set_value(state, *state.waiting_idx, val);
                        state.waiting_idx.reset();
                    } else {
                        // todo: deal with positional args
                    }
                } else {
                    if (state.waiting_idx) {
                        fmt::print("required value for arg {} / '{}'\n",
                                args[*state.waiting_idx].longname,
                                args[*state.waiting_idx].shortnames);
                        ok = false;
                        break;
                    } else {
                        if (!((islong && handle_long(state, name, val)) ||
                                 (!islong && handle_short(state, name, val)))) {
                            ok = false;
                            break;
                        }
                    }
                }
            } else {
                // todo: deal with positional args
            }
        }

        if (!ok || show_usage) {
            // trim the usage (makes it a little prettier where it's defined)
            // todo: move to a print_usage, or do it in the ctor
            std::size_t first = usage_text.find_first_not_of(" \r\n");
            if (first == std::string_view::npos) {
                first = 0;
            }
            std::size_t last = usage_text.find_last_not_of(" \r\n");
            if (last == std::string_view::npos) {
                last = usage_text.size();
            }

            fmt::print(usage_text.substr(first, (last+1)-first));
            fmt::print("\n");

            if (ok) {
                std::exit(EXIT_SUCCESS);
            } else {
                std::exit(EXIT_FAILURE);
            }
        }

        if (ok && state.waiting_idx) {
            fmt::print("required value for arg {} / '{}'\n",
                    args[*state.waiting_idx].longname,
                    args[*state.waiting_idx].shortnames);
            ok = false;
        }
    }

    for (auto& sub : subs) {
        if (!sub.satisfied(state)) {
            ok = false;
            break;
        }
    }

    if (!ok) {
        return std::nullopt;
    }

    // todo: check for required args
    // todo: if there's an error, caller needs to return
    // EXIT_FAILURE, but if help is requested, EXIT_SUCCESS;
    // how to indicate this?
    // todo: print to stderr
    // fmt::print(stderr, "{}: invalid arg -- '{}'\n",
    //                    argv[0], argv[optind - 1]);

    return state.o;
}

template <typename T>
bool parser<T>::handle_short(detail::parse_state<T>& state, std::string_view name, std::string_view val)
{
    for (std::size_t idx = 0; idx < args.size(); ++idx) {
        auto& arg = args[idx];
        if (arg.shortnames.find(name[0]) != std::string_view::npos) {
            // todo: should consolidate this with handle_long's
            if (std::holds_alternative<StringVariant>(arg.argument)) {
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

    fmt::print("arg -{} could not be found\n", name);
    return false;
}

template <typename T>
bool parser<T>::handle_long(detail::parse_state<T>& state, std::string_view name, std::string_view val)
{
    if (name != "--"sv) {
        for (std::size_t idx = 0; idx < args.size(); ++idx) {
            auto& arg = args[idx];
            if (arg.longname == name) {
                // todo: should consolidate this with handle_short's
                if (std::holds_alternative<StringVariant>(arg.argument)) {
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

        fmt::print("arg --{} could not be found\n", name);
        return false;
    } else {
        state.end_of_named = true;
        return true;
    }
}

template <typename T>
void parser<T>::set_value(detail::parse_state<T>& state, std::size_t idx, std::string_view val)
{
    auto& arg = args[idx];
    std::visit(
            [ &state, &val ](auto&& arg) -> auto {
                using V = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<V, BoolVariant>) {
                    // everthing else (incl empty) is true.
                    bool bval = val != "false" && val != "0";

                    std::visit(
                            [ &state, bval ](auto&& arg) -> auto {
                                using V = std::decay_t<decltype(arg)>;
                                if constexpr (std::is_same_v<V, bool*>) {
                                    *arg = bval;
                                } else if constexpr (std::is_same_v<V, bool T::*>) {
                                    (state.o).*arg = bval;
                                }
                            },
                            arg);
                } else if constexpr (std::is_same_v<V, StringVariant>) {
                    std::visit(
                            [ &state, &val ](auto&& arg) -> auto {
                                using V = std::decay_t<decltype(arg)>;
                                if constexpr (std::is_same_v<V, std::string_view*>) {
                                    *arg = val;
                                } else if constexpr (std::is_same_v<V, std::string_view T::*>) {
                                    (state.o).*arg = val;
                                } else if constexpr (std::is_same_v<V, std::string*>) {
                                    *arg = std::string(val);
                                } else if constexpr (std::is_same_v<V, std::string T::*>) {
                                    (state.o).*arg = std::string(val);
                                }
                            },
                            arg);
                }
            },
            arg.argument);

    state.satisfied[idx] = true;
}

} // namespace rw::argparse

#endif // RW_ARGPARSE_H
