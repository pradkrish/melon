#ifndef MELON_CONCEPTS_GRAPH_HPP
#define MELON_CONCEPTS_GRAPH_HPP

#include <concepts>
#include <ranges>

#include "melon/concepts/detail/range_of.hpp"

namespace fhamonic {
namespace melon {
namespace concepts {

// clang-format off
template <typename G>
concept graph = std::semiregular<G> && requires(G g, typename G::vertex u,
                                        typename G::arc a) {
    { g.vertices() } -> detail::range_of<typename G::vertex>;
    { g.arcs() } -> detail::range_of<typename G::arc>;
    { g.source(a) } -> std::same_as<typename G::vertex>;
    { g.target(a) } -> std::same_as<typename G::vertex>;
    { g.arcs_pairs() }
        -> detail::range_of<std::pair<typename G::vertex, typename G::vertex>>;
};

template <typename G>
concept adjacency_list_graph = graph<G> && requires(G g, typename G::vertex u,
                                        typename G::arc a) {
    { g.out_neighbors(u) } -> detail::range_of<typename G::vertex>;
};

template <typename G>
concept incidence_list_graph = graph<G> && requires(G g, typename G::vertex u) {
    { g.out_arcs(u) } -> detail::range_of<typename G::arc>;
};


template <typename G>
concept reversible_adjacency_list_graph = graph<G> && requires(G g,
                                    typename G::vertex u, typename G::arc a) {
    { g.in_neighbors(u) } -> detail::range_of<typename G::vertex>;
};

template <typename G>
concept reversible_incidence_list_graph = graph<G> && requires(G g,
                                        typename G::vertex u) {
    { g.in_arcs(u) } -> detail::range_of<typename G::arc>;
};
// clang-format on

}  // namespace concepts
}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_CONCEPTS_GRAPH_HPP