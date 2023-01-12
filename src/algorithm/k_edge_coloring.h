/**
 * Copyright (C) 2022-2023 : Kathrin Hanauer, Lara Ost
 *
 * This file is part of DyDJ Match.
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

#include "algorithm/disjoint_matching_algorithm.h"
#include "datastructure/kcoloring_extensions.h"
#include "datastructure/kcoloring_utilities.h"

#include "tools/utility.h"

enum struct k_edge_coloring_algo_type {
    STATIC,
    DYNAMIC,
    HYBRID
};

template<k_edge_coloring_algo_type algo_type, bool common_color, bool rotate_long, bool measure_color_ops = false, bool use_pp_ds = false>
class KEdgeColoring_2 : public DisjointMatchingAlgorithm<measure_color_ops, ArcMateExtension, FreeColorsExtension> {
    using algo_base = DisjointMatchingAlgorithm<measure_color_ops, ArcMateExtension, FreeColorsExtension>;

    using algo_base::diGraph;
    using algo_base::weights;
    using algo_base::coloring;

public:
    KEdgeColoring_2(bool post_process = false,
                    double hybrid_threshold = 1,
                    double filter_threshold = 2) : post_process(post_process),
                                                   hybrid_threshold(hybrid_threshold),
                                                   update_filter(filter_threshold) {
        // Assert that `post_process == true` if `use_pp_ds == true`.
        assert(!use_pp_ds || post_process);
    }

    std::string getName() const noexcept override {
        std::string name = "k-EdgeColoring-";
        switch (algo_type) {
            case k_edge_coloring_algo_type::STATIC:
                name += "static";
                break;
            case k_edge_coloring_algo_type::HYBRID:
                name += "h-" + to_string_with_precision(hybrid_threshold, 2);
                break;
            case k_edge_coloring_algo_type::DYNAMIC:
                name += "dynamic";
                break;
        }
        if constexpr (algo_type == k_edge_coloring_algo_type::HYBRID || algo_type == k_edge_coloring_algo_type::DYNAMIC) {
            if (update_filter.getUpThreshold() != 1) {
                name += "-ft" + to_string_with_precision(update_filter.getUpThreshold(), 2);
            }
        }
        if (post_process) {
            name += "-p";
            if constexpr (use_pp_ds) {
                name += "+";
            }
        }
        return name;
    }

    std::string getShortName() const noexcept override {
        std::string name = "k-EdgeColoring-";
        switch (algo_type) {
            case k_edge_coloring_algo_type::STATIC:
                name += "s";
                break;
            case k_edge_coloring_algo_type::HYBRID:
                name += "h-" + to_string_with_precision(hybrid_threshold, 1);
                break;
            case k_edge_coloring_algo_type::DYNAMIC:
                name += "d";
                break;
        }
        if constexpr (algo_type == k_edge_coloring_algo_type::HYBRID || algo_type == k_edge_coloring_algo_type::DYNAMIC) {
            if (update_filter.getUpThreshold() != 1) {
                name += "-ft" + to_string_with_precision(update_filter.getUpThreshold(), 2);
            }
        }
        if (post_process) {
            name += "p";
        }
        return name;
    }

    virtual void reset() override {
        algo_base::reset();
        compute_from_scratch = false;
        update_count = 0;
        delta_over = false;
    }

    virtual void onPropertyChange(GraphArtifact *artifact,
                                  const EdgeWeight &oldValue,
                                  const EdgeWeight &newValue) override {
        if constexpr (algo_type == k_edge_coloring_algo_type::STATIC) {
            return;
        }
        
        if (update_filter.test(oldValue, newValue)) {
            if constexpr (use_pp_ds) {
                auto arc = static_cast<Arc*>(artifact);
                if (oldValue > newValue && coloring.is_colored(arc)) {
                    register_neighbors_for_post_processing(arc);
                } else if (oldValue < newValue && !coloring.is_colored(arc)) {
                    post_processor.register_arc(arc);
                }
            }
            return;
        }

        if constexpr (algo_type == k_edge_coloring_algo_type::HYBRID) {
            update_count++;
            if (delta_over) {
                compute_from_scratch = (update_count/(double)diGraph->getSize() >= hybrid_threshold);
                update_count = 0;
                delta_over = false;
            }
            if (compute_from_scratch) {
                return;
            }
        }

        auto arc = static_cast<Arc*>(artifact);
        if (newValue > oldValue && !coloring.is_colored(arc)) {
            [[maybe_unused]] auto arc_got_colored = attempt_match(arc);
            if constexpr (use_pp_ds) {
                if (!arc_got_colored) {
                    post_processor.register_arc(arc);
                }
            }
        } else if (newValue < oldValue && coloring.is_colored(arc)) {
            if (newValue == 0) {
                coloring.uncolor(arc);
            }
            auto heaviest_tail_arc = find_heaviest_incident_uncolored_arc(coloring, diGraph, weights, arc->getTail());
            auto heaviest_head_arc = find_heaviest_incident_uncolored_arc(coloring, diGraph, weights, arc->getHead());
            for (auto a : {heaviest_tail_arc, heaviest_head_arc}) {
                if (a != nullptr) {
                    attempt_match(a);
                }
            }
            if constexpr (use_pp_ds) {
                register_neighbors_for_post_processing(arc);
            }
        }
    }

    virtual void run() override {
        if constexpr (algo_type == k_edge_coloring_algo_type::STATIC) {
            reset();
            compute_edge_coloring();
        }
        if constexpr (algo_type == k_edge_coloring_algo_type::HYBRID) {
            delta_over = true;
            if (compute_from_scratch) {
                algo_base::reset();
                compute_edge_coloring();
            }
        }
        if (post_process) {
            if constexpr (algo_type == k_edge_coloring_algo_type::STATIC) {
                make_coloring_maximal_pq(coloring, diGraph, weights);
            }
            if constexpr (algo_type == k_edge_coloring_algo_type::HYBRID) {
                if (compute_from_scratch) {
                    make_coloring_maximal_pq(coloring, diGraph, weights);
                } else {
                    if constexpr (use_pp_ds) {
                        post_processor.perform_post_processing(coloring, weights);
                    } else {
                        make_coloring_maximal_pq(coloring, diGraph, weights);
                    }
                }
            }
            if constexpr (algo_type == k_edge_coloring_algo_type::DYNAMIC) {
                if constexpr (use_pp_ds) {
                    post_processor.perform_post_processing(coloring, weights);
                } else {
                    make_coloring_maximal_pq(coloring, diGraph, weights);
                }
            }
        }
    }

private:
    const bool post_process;
    const double hybrid_threshold;

    const UpdateFilter update_filter;

    MaximalityPostProcessor<decltype(coloring)> post_processor;

    bool compute_from_scratch = false;
    int update_count = 0;
    bool delta_over = false;

    // Color edge `xy` with `x` as the "center" for computing the fan.
    color_t color_edge(Arc *xy, Vertex *x) {
        if constexpr (common_color) {
            auto col = coloring.common_free_color(xy->getTail(), xy->getHead());
            if (col != color_set::npos) {
                coloring.color(xy, col);
                return col;
            }
        }

        auto c = coloring.get_any_free_color(x);
        if (c == color_set::npos) {
            return UNCOLORED;
        }

        auto fan = compute_fan(coloring, x, xy);
        assert(!fan.empty());
        auto d = coloring.get_any_free_color(fan.back()->getOther(x));
        if (d == color_set::npos) {
            return UNCOLORED - 1;
        }


        bool inverted = false;
        if (!coloring.is_color_free(x, d) && c != d) {
            invert_cd_path(coloring, x, c, d);
            inverted = true;
        }
        // By default initialize `w` with the last element of the fan
        auto fan_end = fan.end();
        auto w = fan.back();
        // If we don't rotate long, then we obtain a new `w`
        if (!rotate_long || inverted) {
            fan_end = std::find_if(fan.begin(), fan.end(), [&, this](Arc* a) {
                return coloring.is_color_free(a->getOther(x), d);
            });
            w = *fan_end;
            fan_end++;
        }
        rotate_fan(coloring, fan.begin(), fan_end);
        // Color the last edge in the rotated prefix of the fan
        coloring.color(w, d);
        return std::max(c, d);
    }

    // Compute an edge-coloring from scratch
    void compute_edge_coloring() {
        color_t colors = 0;

        std::vector<Arc*> edges;
        edges.reserve(diGraph->getNumArcs(false));
        diGraph->mapArcs([this,&edges](Arc *arc) {
            if ((*weights)[arc] > 0) {
                edges.push_back(arc);
            }
        });

        std::sort(edges.begin(), edges.end(), [this](const Arc* lop, const Arc* rop) {
            return (*weights)[lop] > (*weights)[rop];
        });

        for (auto arc: edges) {
            if (coloring.any_color_free(arc->getTail()) &&
                    coloring.any_color_free(arc->getHead())) {
                auto c = color_edge(arc, arc->getTail());
                if (c == UNCOLORED - 1) {
                    c = color_edge(arc, arc->getHead());
                }
                if (c < coloring.getNumColors()) {
                    colors = std::max(colors, c+1);
                }

                // TODO: implement `lightest_color`?
            }
        }
    }

    bool attempt_match(Arc *arc) {
        Arc *lightest_tail_arc = nullptr;
        Arc *lightest_head_arc = nullptr;
        EdgeWeight replace_weight = 0;
        color_t tail_arc_color, head_arc_color;
        if (coloring.no_color_free(arc->getTail())) {
            lightest_tail_arc = coloring.getLightestColoredEdge(arc->getTail(), weights);
            replace_weight += (*weights)[lightest_tail_arc];
            tail_arc_color = coloring.get_color(lightest_tail_arc);
        }
        if (coloring.no_color_free(arc->getHead())) {
            lightest_head_arc = coloring.getLightestColoredEdge(arc->getHead(), weights);
            replace_weight += (*weights)[lightest_head_arc];
            head_arc_color = coloring.get_color(lightest_head_arc);
        }
        if ((*weights)[arc] > replace_weight) {
            // Free colors at the endpoints of `arc` if coloring it is beneficial
            for (auto a: {lightest_tail_arc, lightest_head_arc}) {
                if (a != nullptr) {
                    coloring.uncolor(a);
                }
            }
        } else {
            // Matching e instead of `lightest_tail_arc` and `lightest_head_arc`
            // does not improve the matching.
            return false;
        }

        color_edge(arc, arc->getTail());
        if (!coloring.is_colored(arc)) {
            color_edge(arc, arc->getHead());
        }
        if (!coloring.is_colored(arc)) {
            // `arc` cannot be colored; reinstate the colors of the lightest adjacent arcs.
            if (lightest_tail_arc != nullptr) {
                coloring.color(lightest_tail_arc, tail_arc_color);
            }
            if (lightest_head_arc != nullptr) {
                coloring.color(lightest_head_arc, head_arc_color);
            }
            return false;
        } else {
            // See if we can color the lightest adjacent arcs (in the simplest way possible)
            for (auto a: {lightest_tail_arc, lightest_head_arc}) {
                if (a != nullptr) {
                    auto col = coloring.common_free_color(a->getTail(),
                                                          a->getHead());
                    if (col != color_set::npos) {
                        coloring.color(a, col);
                    }
                }
            }
            return true;
        }

    } 

    void register_neighbors_for_post_processing(Arc *arc) {
        if constexpr (use_pp_ds) {
            if (!coloring.is_colored(arc)) {
                for (auto vertex: {arc->getTail(), arc->getHead()}) {
                    diGraph->mapIncidentArcs(vertex, [this](Arc* a) {
                        post_processor.register_arc(a);
                    });
                }
            }
        }
    }

};
