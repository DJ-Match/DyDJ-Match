/**
 * Copyright (C) 2022-2023 : Kathrin Hanauer, Lara Ost
 *
 * This file is part of DyDJ Match and was adapted from DJ Match.
 *
 * DyDJ Match is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DyDJ Match is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DyDJ Match.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact information:
 *   https://github.com/DJ-Match/DyDJ-Match
 *
 * Author:
 *   Lara Ost
 *   based on code by Kathrin Hanauer
 */

#pragma once

#ifdef USE_BOOST_HEAP
    #include "boost/heap/d_ary_heap.hpp"
#else
    #include <queue>
#endif

#include "datastructure/kcoloring.h"
#include "datastructure/kcoloring_extensions.h"
#include "tools/utility.h"

// Compute a fan in a `KColoring` of `diGraph`.
// `kcoloring_type` is expected to be a specialization of `KColoring<...>`,
// with (at least) the `ArcMateExtension` and the `FreeColorsExtension`.
//
// This is based on the `quicker_fan` function in the old version of the k-edge-coloring algorithm.
template<typename kcoloring_type>
std::vector<Arc*> compute_fan(const kcoloring_type &coloring,
                              Vertex *x,
                              Arc *xy) {
    std::vector<Arc*> fan;
    fan.push_back(xy);

    auto colored_arcs = coloring.getColoredArcs(x);
    std::vector<Arc*> colored_arcs_other;
    colored_arcs_other.reserve(colored_arcs.size());
    bool extended = false;
    do {
        extended = false;
        colored_arcs_other.clear();
        for (auto arc: colored_arcs) {
            if (coloring.is_color_free(fan.back()->getOther(x), coloring.get_color(arc))) {
                fan.push_back(arc);
                if (coloring.no_color_free(arc->getOther(x))) {
                    extended = false;
                    return fan;
                } else {
                    extended = true;
                }
            } else {
                colored_arcs_other.push_back(arc);
            }
        }
        std::swap(colored_arcs, colored_arcs_other);
    } while(extended);

    return fan;
}

template<typename kcoloring_type>
void rotate_fan(kcoloring_type &coloring,
                const std::vector<Arc*>::const_iterator begin,
                const std::vector<Arc*>::const_iterator end) {
    if (begin == end) {
        return;
    }
    auto previous = *begin;
    for (auto it = begin + 1; it != end; ++it) {
        auto c = coloring.get_color(*it);
        coloring.uncolor(*it);
        coloring.color(previous, c);
        previous = *it;
    }
}

// Invert the `cd`-path starting at `x` in `coloring`.
// `kcoloring_type` is expected to be a specialization of `KColoring<...>`,
// with (at least) the `ArcMateExtension`.
//
// This is based on the `invert_cd_path_it` function in the old version of the k-edge-coloring algorithm.
template<typename kcoloring_type>
void invert_cd_path(kcoloring_type &coloring,
                    Vertex *x,
                    color_t c,
                    color_t d) {
    auto arcToRecolor = coloring.getArcToMate(d, x);
    auto nextColor = c; auto otherColor = d;
    auto nextArc = arcToRecolor;

    // First remove the current color from `arcToRecolor`
    // This color is `d`/`otherColor`.
    coloring.uncolor(arcToRecolor);
    // This loop terminates once there is no `nextArc`.
    while(true) {
        x = arcToRecolor->getOther(x);
        nextArc = coloring.getArcToMate(nextColor, x);

        // If there is a next arc, then we need to uncolor it to keep the coloring valid.
        // In any case we assign `nextColor` to `arcToRecolor`, which is currently uncolored.
        // If no `nextArc` exists, we can stop after coloring `arcToRecolor`.
        if (nextArc != nullptr) {
            coloring.uncolor(nextArc);
            coloring.color(arcToRecolor, nextColor);
            arcToRecolor = nextArc;
            std::swap(nextColor, otherColor);
        } else {
            coloring.color(arcToRecolor, nextColor);
            break;
        }
    }
}

template<typename kcoloring_type>
Arc* find_heaviest_incident_uncolored_arc(const kcoloring_type &coloring,
                                          DiGraph *diGraph,
                                          ModifiableProperty<EdgeWeight> *weights,
                                          Vertex *vertex) {
    Arc* heaviest = nullptr;
    EdgeWeight max_weight = 0;
    diGraph->mapIncidentArcs(vertex, [&](Arc* arc) {
        if (!coloring.is_colored(arc) && (*weights)[arc] > max_weight) {
            heaviest = arc;
            max_weight = (*weights)[arc];
        }
    });
    return heaviest;
}

namespace make_maximal_detail {
    using pq_element_type = std::pair<Arc*, EdgeWeight>;

    struct pq_element_compare_max {
        bool operator()(const pq_element_type &lop, const pq_element_type &rop) const {
            return lop.second < rop.second;
        }
    };

    #ifdef USE_BOOST_HEAP
        using pq_type = boost::heap::d_ary_heap<pq_element_type,
                                                boost::heap::arity<2>,
                                                boost::heap::compare<pq_element_compare_max>>;
    #else
        using pq_type = std::priority_queue<pq_element_type,
                                            std::vector<pq_element_type>,
                                            pq_element_compare_max>;
    #endif

    template<typename kcoloring_type>
    void process_maximal_pq(pq_type& queue,
                            kcoloring_type &coloring,
                            ModifiableProperty<EdgeWeight> *weights) {
        while (!queue.empty()) {
            Arc* arc = queue.top().first;
            EdgeWeight arc_weight = queue.top().second;
            queue.pop();

            auto col = coloring.common_free_color(arc->getTail(), arc->getHead());
            if (col != color_set::npos) {
                coloring.color(arc, col);
                continue;
            }
            // Now check if the invariant holds for all colors.
            // For each color, the uncolored arc `arc` should have at least one neighbor that's heavier.
            // If we find a color where this does not hold, we uncolor the adjacent colored edges,
            // and color `arc` instead.
            for (auto color: coloring.color_range()) {
                auto atm_tail = coloring.getArcToMate(color, arc->getTail());
                auto atm_head = coloring.getArcToMate(color, arc->getHead());
                bool one_heavier_neighbor = false;
                EdgeWeight sum_weight = 0;
                if (atm_tail != nullptr) {
                    const auto tail_weight = (*weights)[atm_tail];
                    one_heavier_neighbor |= (tail_weight >= arc_weight);
                    sum_weight += tail_weight;
                }
                if (atm_head != nullptr) {
                    const auto head_weight = (*weights)[atm_head];
                    one_heavier_neighbor |= (head_weight >= arc_weight);
                    sum_weight += head_weight;
                }
                if (!one_heavier_neighbor && sum_weight < arc_weight) {
                    for (auto a: {atm_tail, atm_head}) {
                        if (a != nullptr) {
                            coloring.uncolor(a);
                            queue.push({a, (*weights)[a]});
                        }
                    }
                    coloring.color(arc, color);
                    break;
                }
            }
        }
    }
}

template<typename kcoloring_type>
void make_coloring_maximal_pq(kcoloring_type &coloring,
                              DiGraph *diGraph,
                              ModifiableProperty<EdgeWeight> *weights) {
    using namespace make_maximal_detail;

    auto queue = pq_type{};
    diGraph->mapArcs([&](Arc *arc) {
        if (!coloring.is_colored(arc)) {
            queue.push({arc, (*weights)[arc]});
        }
    });

    process_maximal_pq(queue, coloring, weights);
}

template<typename kcoloring_type>
void make_coloring_maximal_fixpoint(kcoloring_type &coloring,
                                    DiGraph *diGraph,
                                    ModifiableProperty<EdgeWeight> *weights) {
    std::vector<Arc*> arcs_to_process, next_arcs_to_process;
    arcs_to_process.reserve(diGraph->getNumArcs(false));
    next_arcs_to_process.reserve(diGraph->getNumArcs(false));

    diGraph->mapArcs([&](Arc *arc) {
        if (!coloring.is_colored(arc)) {
            arcs_to_process.push_back(arc);
        }
    });

    while (!arcs_to_process.empty()) {
        std::sort(arcs_to_process.begin(), arcs_to_process.end(), [&](Arc *a, Arc *b) {
            return (*weights)[a] > (*weights)[b];
        });
        for (auto arc: arcs_to_process) {
            auto arc_weight = (*weights)[arc];
            auto col = coloring.common_free_color(arc->getTail(), arc->getHead());
            if (col != color_set::npos) {
                coloring.color(arc, col);
                continue;
            }
            // Now check if the invariant holds for all colors.
            // For each color, the uncolored arc `arc` should have at least one neighbor that's heavier.
            // If we find a color where this does not hold, we uncolor the adjacent colored edges,
            // and color `arc` instead.
            for (auto color: coloring.color_range()) {
                auto atm_tail = coloring.getArcToMate(color, arc->getTail());
                auto atm_head = coloring.getArcToMate(color, arc->getHead());
                bool one_heavier_neighbor = false;
                EdgeWeight sum_weight = 0;
                if (atm_tail != nullptr) {
                    const auto tail_weight = (*weights)[atm_tail];
                    one_heavier_neighbor |= (tail_weight >= arc_weight);
                    sum_weight += tail_weight;
                }
                if (atm_head != nullptr) {
                    const auto head_weight = (*weights)[atm_head];
                    one_heavier_neighbor |= (head_weight >= arc_weight);
                    sum_weight += head_weight;
                }
                if (!one_heavier_neighbor && sum_weight < arc_weight) {
                    for (auto a: {atm_tail, atm_head}) {
                        if (a != nullptr) {
                            coloring.uncolor(a);
                            next_arcs_to_process.push_back(a);
                        }
                    }
                    coloring.color(arc, color);
                    break;
                }
            }
        }
        std::swap(arcs_to_process, next_arcs_to_process);
        next_arcs_to_process.clear();
    }
}

template<typename kcoloring_type>
class MaximalityPostProcessor {

public:
    void register_arc(Arc* arc) {
        arcs_to_process.add(arc);
    }

    void perform_post_processing(kcoloring_type &coloring, ModifiableProperty<EdgeWeight>* weights) {
        using namespace make_maximal_detail;

        priority_queue = {};
        for (auto arc: arcs_to_process.vector()) {
            if ((*weights)[arc] > 0 && !coloring.is_colored(arc)) {
                priority_queue.push({arc, (*weights)[arc]});
            }
        } 
        process_maximal_pq(priority_queue, coloring, weights);
        arcs_to_process.next_round();
    }

private:
    TimedArtifactSet<Arc*> arcs_to_process;
    make_maximal_detail::pq_type priority_queue;

};
