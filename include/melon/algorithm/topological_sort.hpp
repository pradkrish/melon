#ifndef MELON_ALGORITHM_TOPOLOGICAL_SORT_HPP
#define MELON_ALGORITHM_TOPOLOGICAL_SORT_HPP

#include <algorithm>
#include <cassert>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "melon/detail/constexpr_ternary.hpp"
#include "melon/graph.hpp"
#include "melon/utility/traversal_iterator.hpp"

namespace fhamonic {
namespace melon {

struct topological_sort_default_traits {
    static constexpr bool store_pred_vertices = false;
    static constexpr bool store_pred_arcs = false;
    static constexpr bool store_distances = false;
};

template <graph G, typename T = topological_sort_default_traits>
    requires outward_incidence_graph<G> && has_vertex_map<G>
class topological_sort {
public:
    using vertex = vertex_t<G>;
    using arc = arc_t<G>;
    using traits = T;

    static_assert(
        !(outward_adjacency_graph<G> && traits::store_pred_arcs),
        "traversal on outward_adjacency_list cannot access predecessor arcs.");

    using reached_map = vertex_map_t<G, bool>;
    using remaining_in_degree_map = vertex_map_t<G, long unsigned int>;
    using pred_vertices_map =
        std::conditional<traits::store_pred_vertices, vertex_map_t<G, vertex>,
                         std::monostate>::type;
    using pred_arcs_map =
        std::conditional<traits::store_pred_arcs, vertex_map_t<G, arc>,
                         std::monostate>::type;
    using distances_map =
        std::conditional<traits::store_distances, vertex_map_t<G, int>,
                         std::monostate>::type;

private:
    std::reference_wrapper<const G> _graph;
    std::vector<vertex> _queue;
    std::vector<vertex>::iterator _queue_current;

    reached_map _reached_map;
    remaining_in_degree_map _remaining_in_degree_map;
    pred_vertices_map _pred_vertices_map;
    pred_arcs_map _pred_arcs_map;
    distances_map _dist_map;

    constexpr void push_start_vertices() noexcept {
        _queue.resize(0);
        _queue_current = _queue.begin();
        _reached_map.fill(false);
        if(has_in_degree<G>) {
            for(auto && u : vertices(_graph.get())) {
                _remaining_in_degree_map[u] = in_degree(_graph.get(), u);
                if(_remaining_in_degree_map[u] == 0) {
                    _queue.push_back(u);
                }
            }
        } else {
            _remaining_in_degree_map.fill(0);
            for(auto && u : vertices(_graph.get())) {
                for(auto && a : out_arcs(_graph.get(), u)) {
                    const vertex & w = arc_target(_graph.get(), a);
                    ++_remaining_in_degree_map[w];
                }
            }
            for(auto && u : vertices(_graph.get())) {
                if(_remaining_in_degree_map[u] == 0) {
                    _queue.push_back(u);
                }
            }
        }
        if constexpr(traits::store_distances) _dist_map.fill(0);
    }

public:
    [[nodiscard]] constexpr explicit topological_sort(const G & g)
        : _graph(g)
        , _queue()
        , _reached_map(create_vertex_map<bool>(g, false))
        , _remaining_in_degree_map(create_vertex_map<long unsigned int>(
              g, std::numeric_limits<unsigned int>::max()))
        , _pred_vertices_map(constexpr_ternary<traits::store_pred_vertices>(
              create_vertex_map<vertex>(g), std::monostate{}))
        , _pred_arcs_map(constexpr_ternary<traits::store_pred_arcs>(
              create_vertex_map<arc>(g), std::monostate{}))
        , _dist_map(constexpr_ternary<traits::store_distances>(
              create_vertex_map<int>(g), std::monostate{})) {
        _queue.reserve(nb_vertices(g));
        push_start_vertices();
    }

    [[nodiscard]] constexpr topological_sort(const topological_sort & bin) =
        default;
    [[nodiscard]] constexpr topological_sort(topological_sort && bin) = default;

    constexpr topological_sort & operator=(const topological_sort &) = default;
    constexpr topological_sort & operator=(topological_sort &&) = default;

public:
    constexpr topological_sort & reset() noexcept {
        _queue.resize(0);
        _queue_current = _queue.begin();
        _reached_map.fill(false);
        return *this;
    }

    [[nodiscard]] constexpr bool finished() const noexcept {
        return _queue_current == _queue.end();
    }

    [[nodiscard]] constexpr vertex current() const noexcept {
        assert(!finished());
        return *_queue_current;
    }

    constexpr void advance() noexcept {
        assert(!finished());
        const vertex & u = *_queue_current;
        ++_queue_current;
        for(auto && a : out_arcs(_graph.get(), u)) {
            const vertex & w = arc_target(_graph.get(), a);
            if(--_remaining_in_degree_map[w] > 0) continue;
            _queue.push_back(w);
            if constexpr(traits::store_pred_vertices) _pred_vertices_map[w] = u;
            if constexpr(traits::store_pred_arcs) _pred_arcs_map[w] = a;
            if constexpr(traits::store_distances)
                _dist_map[w] = _dist_map[u] + 1;
        }
    }

    constexpr void run() noexcept {
        while(!finished()) advance();
    }
    [[nodiscard]] constexpr auto begin() noexcept {
        return traversal_iterator(*this);
    }
    [[nodiscard]] constexpr auto end() const noexcept {
        return traversal_end_sentinel();
    }

    [[nodiscard]] constexpr bool reached(const vertex & u) const noexcept {
        return _reached_map[u];
    }

    [[nodiscard]] constexpr vertex pred_vertex(const vertex & u) const noexcept
        requires(traits::store_pred_vertices)
    {
        assert(reached(u));
        return _pred_vertices_map[u];
    }
    [[nodiscard]] constexpr arc pred_arc(const vertex & u) const noexcept
        requires(traits::store_pred_arcs)
    {
        assert(reached(u));
        return _pred_arcs_map[u];
    }
    [[nodiscard]] constexpr int dist(const vertex & u) const noexcept
        requires(traits::store_distances)
    {
        assert(reached(u));
        return _dist_map[u];
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_ALGORITHM_TOPOLOGICAL_SORT_HPP
