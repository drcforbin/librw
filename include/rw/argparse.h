#ifndef RW_ARGPARSE_H
#define RW_ARGPARSE_H

#include <array>
#include <cassert>
#include <optional>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace rw::argparse {

using namespace std::literals;

// forward
template <typename T, typename Reporter>
class parser;

namespace detail {

struct Reporter
{
    void conflict(std::string_view arg1, std::string_view arg2) const;
    void unexpected(std::string_view name) const;
    void missing_required_of() const;
    void missing_required(std::string_view name) const;
    void missing_value(std::string_view name) const;

    [[noreturn]] void usage(bool ok, std::string_view text) const;
};

template <typename T>
struct ArgBase
{
    bool required = false;

    bool needs_value() const
    {
        return std::holds_alternative<String>(arg);
    }

    void set_value(T& o, std::string_view val) const
    {
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
                            if constexpr (std::is_same_v<V, std::string_view*> ||
                                    std::is_same_v<V, std::string*>) {
                                *arg = val;
                            } else if constexpr (std::is_same_v<V, std::string_view T::*> ||
                                    std::is_same_v<V, std::string T::*>) {
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
        required(required), arg(value)
    {
    }

    template <typename MemberT>
    constexpr ArgBase(MemberT T::*member, bool required) :
        required(required), arg(member)
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
        ArgBase<T>(value, required), name(name)
    {
    }

    template <typename MemberT>
    constexpr PosArg(MemberT T::*member, std::string_view name, bool required) :
        ArgBase<T>(member, required), name(name)
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

template <typename T>
struct ExtraArg
{
    template <typename V>
    constexpr ExtraArg(V* value) :
        arg(value)
    {
    }

    template <typename MemberT>
    constexpr ExtraArg(MemberT T::*member) :
        arg(member)
    {
    }

    void add_value(T& o, std::string_view val) const
    {
        std::visit(
            [ &o, &val ](auto&& arg) -> auto {
                using V = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<V, std::vector<std::string_view>*>) {
                    arg->push_back(val);
                } else if constexpr (std::is_same_v<V, std::vector<std::string_view> T::*>) {
                    (o.*arg).push_back(val);
                } else if constexpr (std::is_same_v<V, std::vector<std::string>*>) {
                    arg->push_back(std::string(val));
                } else if constexpr (std::is_same_v<V, std::vector<std::string> T::*>) {
                    (o.*arg).push_back(std::string(val));
                }
            },
            arg);
    }

private:
    std::variant<std::vector<std::string_view>*,
            std::vector<std::string_view> T::*,
            std::vector<std::string>*,
            std::vector<std::string> T::*>
            arg;
};

template <typename T, typename Reporter>
struct parse_state
{
    parse_state(std::size_t num_args, Reporter& reporter) :
        reporter(reporter),
        pos_satisfied(num_args),
        satisfied(num_args)
    {
    }

    Reporter& reporter;

    bool show_usage = false;

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

    template <typename V>
    Derived& optional(V&& pval, std::string_view longname,
            std::string_view shortnames = ""sv)
    {
        p.optional(pval, longname, shortnames);
        group.push_back(p.args.size() - 1);
        return static_cast<Derived&>(*this);
    }

protected:
    parser<T, Reporter>& p;
    std::vector<std::size_t> group;
};

using parsed_arg = std::tuple<bool, std::string_view, std::string_view>;
parsed_arg parse_arg(std::string_view arg);

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

    bool satisfied(const detail::parse_state<T, Reporter>& state) const
    {
        std::optional<std::size_t> found_arg;
        for (auto idx : this->group) {
            if (state.satisfied[idx]) {
                if (!found_arg) {
                    found_arg = idx;
                } else {
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

struct no_options_type
{};

template <typename T = no_options_type, typename Reporter = detail::Reporter>
class parser
{
public:
    parser& usage(std::string_view usage,
            std::string_view longname = "help"sv,
            std::string_view shortnames = "?h")
    {
        usage_text = usage;
        usage_longname = longname;
        usage_shortnames = shortnames;
        return *this;
    }

    template <typename V>
    parser& positional(V&& pval, std::string_view name, bool required = true)
    {
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

    template <typename V>
    parser& optional(V&& pval, std::string_view longname,
            std::string_view shortnames = ""sv)
    {
        // longname is required
        assert(!longname.empty());

        args.emplace_back(std::forward<V&&>(pval), longname, shortnames, false);
        return *this;
    }

    template <typename V>
    parser& required(V&& pval, std::string_view longname,
            std::string_view shortnames = ""sv)
    {
        // longname is required
        assert(!longname.empty());

        args.emplace_back(std::forward<V&&>(pval), longname, shortnames, true);
        return *this;
    }

    template <typename V>
    parser& extra(V&& pval)
    {
        extra_arg = ExtraArg(std::forward<V&&>(pval));
        return *this;
    }

    excl_group<T, Reporter>& one_of(bool required = false)
    {
        subs.emplace_back(*this, required);
        return subs.back();
    }

    std::optional<T> parse(const int argc, char** const argv, Reporter& reporter) const;

    std::optional<T> parse(const int argc, char** const argv) const
    {
        Reporter reporter;
        return parse(argc, argv, reporter);
    }

private:
    friend class excl_group<T, Reporter>;
    friend class detail::sub_group<excl_group<T, Reporter>, T, Reporter>;

    using PosArg = detail::PosArg<T>;
    using Arg = detail::Arg<T>;
    using ExtraArg = detail::ExtraArg<T>;

    bool handle_short(detail::parse_state<T, Reporter>& state, std::string_view name,
            std::string_view val) const;
    bool handle_long(detail::parse_state<T, Reporter>& state, std::string_view name,
            std::string_view val) const;

    void set_pos_value(detail::parse_state<T, Reporter>& state, std::size_t idx,
            std::string_view val) const;
    void set_value(detail::parse_state<T, Reporter>& state, std::size_t idx,
            std::string_view val) const;

    std::string_view usage_text;
    std::string_view usage_longname;
    std::string_view usage_shortnames;

    // todo: support other kinds of groups
    // scenarios (some/partially implemented):
    //   multiple exclusive groups, each optional
    //   exclusive groups with required selection
    //   optional groups where multiple members are required
    //   nested groups
    //   command groups? (switching on a positional arg)
    std::vector<excl_group<T, Reporter>> subs;

    // todo: combine pos_args and args, maybe w/ variant?
    std::vector<PosArg> pos_args;
    std::vector<Arg> args;
    std::optional<ExtraArg> extra_arg;
};

template <typename T, typename Reporter>
std::optional<T> parser<T, Reporter>::parse(const int argc, char** const argv, Reporter& reporter) const
{
    detail::parse_state<T, Reporter> state{args.size(), reporter};

    bool ok = true;
    if (argc > 1) {
        for (int i = 1; ok && i < argc; ++i) {
            if (!state.end_of_named) {
                auto [islong, name, val] = detail::parse_arg(argv[i]);
                if (name.size() == 0) {
                    // positional
                    if (state.waiting_idx) {
                        set_value(state, *state.waiting_idx, val);
                        state.waiting_idx.reset();
                    } else {
                        if (state.pos_count < pos_args.size()) {
                            set_pos_value(state, state.pos_count++, val);
                        } else if (extra_arg) {
                            extra_arg->add_value(state.o, val);
                        } else {
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
                } else if (extra_arg) {
                    extra_arg->add_value(state.o, argv[i]);
                } else {
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

    // todo: combine pos_args and args, do it in one loop
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

    if (!ok || state.show_usage) {
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
            set_value(state, idx, val);
            return true;
        }
    }

    if (!usage_shortnames.empty() &&
            usage_shortnames.find(name[0]) != std::string_view::npos) {
        state.show_usage = true;
        return true;
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
                set_value(state, idx, val);
                return true;
            }
        }

        if (!usage_longname.empty() && usage_longname == name) {
            state.show_usage = true;
            return true;
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
    if (arg.needs_value()) {
        if (val.empty()) {
            // todo: what about -x= (with no value?)
            state.waiting_idx = idx;
        } else {
            arg.set_value(state.o, val);
            state.satisfied[idx] = true;
        }
    } else {
        // bools can just be set.
        // todo: better true/false/empty handling..
        // --arg= really should be false, not true
        arg.set_value(state.o, "true"sv);
        state.satisfied[idx] = true;
    }
}

} // namespace rw::argparse

#endif // RW_ARGPARSE_H
