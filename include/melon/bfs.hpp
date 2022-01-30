#ifndef MELON_BFS_HPP
#define MELON_BFS_HPP

#include <algorithm>
#include <cassert>
#include <ranges>
#include <type_traits>  // underlying_type, conditional
#include <variant>      // monostate
#include <vector>

namespace fhamonic {
namespace melon {

enum BFSBehavior : unsigned char {
    TRACK_NONE = 0b00000000,
    TRACK_PRED_NODES = 0b00000001,
    TRACK_PRED_ARCS = 0b00000010,
    TRACK_DISTANCES = 0b00000100
};

template <typename GR,
          std::underlying_type_t<BFSBehavior> BH =
              (BFSBehavior::TRACK_PRED_NODES | BFSBehavior::TRACK_DISTANCES)>
class BFS {
public:
    using Node = GR::Node;
    using Arc = GR::Arc;

    using VisitedMap = typename GR::NodeMap<bool>;

    static constexpr bool track_predecessor_nodes =
        static_cast<bool>(BH & BFSBehavior::TRACK_PRED_NODES);
    static constexpr bool track_predecessor_arcs =
        static_cast<bool>(BH & BFSBehavior::TRACK_PRED_ARCS);
    static constexpr bool track_distances =
        static_cast<bool>(BH & BFSBehavior::TRACK_DISTANCES);

    using PredNodesMap =
        std::conditional<track_predecessor_nodes, typename GR::NodeMap<Node>,
                         std::monostate>::type;
    using PredArcsMap =
        std::conditional<track_predecessor_arcs, typename GR::NodeMap<Arc>,
                         std::monostate>::type;
    using DistancesMap =
        std::conditional<track_distances, typename GR::NodeMap<Value>,
                         std::monostate>::type;

private:
    const GR & graph;
    std::vector<Node> queue;
    std::vector<Node>::iterator front, back;

    VisitedMap queued_map;

    PredNodesMap pred_nodes_map;
    PredArcsMap pred_arcs_map;
    DistancesMap dist_map;

public:
    BFS(const GR & g) : graph(g), queue(), queued_map(g.nb_nodes(), false) {
        queue.reserve(g.nb_nodes());
        front = back = queue.begin();
        if constexpr(track_predecessor_nodes)
            pred_nodes_map.resize(g.nb_nodes());
        if constexpr(track_predecessor_arcs) pred_arcs_map.resize(g.nb_nodes());
        if constexpr(track_distances) dist_map.resize(g.nb_nodes());
    }

    BFS & reset() noexcept {
        queue.resize(0);
        std::ranges::fill(queued_map, false);
        return *this;
    }
    BFS & addSource(Node s, Value dist = BFSSemiringTraits::zero) noexcept {
        assert(!queued_map[s]);
        pushNode(s);
        if constexpr(track_predecessor_nodes) pred_nodes_map[s] = s;
        return *this;
    }

    bool emptyQueue() const noexcept { return front == back; }
    void pushNode(Node u) noexcept {
        *back = s;
        ++back;
        queued_map[u] = true;
    }
    Node popNode() noexcept {
        Node u = *front;
        ++front;
        return u;
    }

    Node processNextNode() noexcept {
        const Node u = popNode();
        for(Arc a : graph.out_arcs(u)) {
            Node w = graph.target(a);
            if(queued_map[w]) continue;
            pushNode(w);
            if constexpr(track_predecessor_nodes) pred_nodes_map[w] = u;
            if constexpr(track_predecessor_arcs) pred_arcs_map[w] = a;
            if constexpr(track_distances) dist_map[w] = dist_map[u] + 1;
        }
        return u;
    }

    void run() noexcept {
        while(!emptyQueue()) {
            processNextNode();
        }
    }

    Node pred_node(const Node u) const noexcept
        requires(track_predecessor_nodes) {
        assert(queued_map[u]);
        return pred_nodes_map[u];
    }
    Arc pred_arc(const Node u) const noexcept requires(track_predecessor_arcs) {
        assert(queued_map[u]);
        return pred_arcs_map[u];
    }
    Value dist(const Node u) const noexcept requires(track_distances) {
        assert(queued_map[u]);
        return dist_map[u];
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_BFS_HPP
