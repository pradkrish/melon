#ifndef MELON_STATIC_DIGRAPH_HPP
#define MELON_STATIC_DIGRAPH_HPP

#include <algorithm>
#include <cassert>
#include <ranges>
#include <vector>

namespace fhamonic {
namespace melon {

class StaticDigraph {
public:
    using Node = unsigned int;
    using Arc = unsigned int;

    template <typename T>
    using NodeMap = std::vector<T>;
    template <typename T>
    using ArcMap = std::vector<T>;

private:
    std::vector<Arc> out_arc_begin;
    std::vector<Node> arc_target;

public:
    StaticDigraph(std::vector<Arc> && begins, std::vector<Node> && targets)
        : out_arc_begin(std::move(begins)), arc_target(std::move(targets)) {}

    StaticDigraph() = default;
    StaticDigraph(const StaticDigraph & graph) = default;
    StaticDigraph(StaticDigraph && graph) = default;

    auto nb_nodes() const { return out_arc_begin.size(); }
    auto nb_arcs() const { return arc_target.size(); }

    bool is_valid_node(Node u) const { return u < nb_nodes(); }
    bool is_valid_arc(Arc u) const { return u < nb_arcs(); }

    auto nodes() const {
        return std::views::iota(static_cast<Node>(0),
                                static_cast<Node>(nb_nodes()));
    }
    auto arcs() const {
        return std::views::iota(static_cast<Arc>(0),
                                static_cast<Arc>(nb_arcs()));
    }
    auto out_arcs(const Node u) const {
        assert(is_valid_node(u));
        return std::views::iota(
            out_arc_begin[u],
            (u + 1 < nb_nodes() ? out_arc_begin[u + 1] : nb_arcs()));
    }
    Node source(Arc a) const {  // O(\log |V|)
        assert(is_valid_arc(a));
        auto it =
            std::ranges::lower_bound(out_arc_begin, a, std::less_equal<Arc>());
        return static_cast<Node>(std::distance(out_arc_begin.begin(), --it));
    }
    Node target(Arc a) const {
        assert(is_valid_arc(a));
        return arc_target[a];
    }
    auto out_neighbors(const Node u) const {
        assert(is_valid_node(u));
        return std::ranges::subrange(
            arc_target.begin() + out_arc_begin[u],
            (u + 1 < nb_nodes() ? arc_target.begin() + out_arc_begin[u + 1]
                                : arc_target.end()));
    }

    auto out_arcs_pairs(const Node u) const {
        assert(is_valid_node(u));
        return std::views::transform(
            out_neighbors(u), [u](auto v) { return std::make_pair(u, v); });
    }
    auto arcs_pairs() const {
        return std::views::join(std::views::transform(
            nodes(), [this](auto u) { return out_arcs_pairs(u); }));
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_STATIC_DIGRAPH_HPP