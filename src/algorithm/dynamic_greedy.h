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
 */

#pragma once

#include <random>

#include "graph.incidencelist/incidencelistvertex.h"

#include "algorithm/disjoint_matching_algorithm.h"
#include "datastructure/kcoloring_extensions.h"

template<bool measure_color_ops = false, bool use_pp_ds = true, int randomized = 3>
class DynamicGreedy : public DisjointMatchingAlgorithm<measure_color_ops, ArcMateExtension, FreeColorsExtension> {

public:
    static constexpr int num_random_reps = randomized;

    using algo_base = DisjointMatchingAlgorithm<measure_color_ops, ArcMateExtension, FreeColorsExtension>;

    using algo_base::diGraph;
    using algo_base::weights;
    using algo_base::coloring;

public:
    DynamicGreedy(int recursion_depth = 1,
                  bool post_process = false,
                  double filter_threshold = 2) : recursion_depth(recursion_depth),
                                                 post_process(post_process),
                                                 update_filter(filter_threshold) {
        // Assert that `post_process == true` if `use_pp_ds == true`.
        assert(!use_pp_ds || post_process);
    }

    std::string getName() const noexcept override {
        auto name = std::string{"dynamic-greedy-"};
        if constexpr (randomized > 0) {
            name += "random" + std::to_string(randomized) + "-";
        }
        name += std::to_string(recursion_depth);
        if (post_process) {
            name += "-p";
            if constexpr (use_pp_ds) {
                name += "+";
            }
        }
        if (update_filter.getUpThreshold() != 1) {
            name += "-ft" + to_string_with_precision(update_filter.getUpThreshold(), 2);
        }
        return name;
    }

    std::string getShortName() const noexcept override {
        auto name = std::string{"dyn-gr-"};
        if constexpr (randomized > 0) {
            name += "r" + std::to_string(randomized) + "-";
        }
        name +=  std::to_string(recursion_depth);
        if (post_process) {
            name += "-p";
            if constexpr (use_pp_ds) {
                name += "+";
            }
        }
        if (update_filter.getUpThreshold() != 1) {
            name += "-ft" + to_string_with_precision(update_filter.getUpThreshold(), 2);
        }
        return name;
    }

    virtual void onPropertyChange(GraphArtifact *artifact,
                                  const EdgeWeight &oldValue,
                                  const EdgeWeight &newValue) override {
        auto arc = static_cast<Arc*>(artifact);
        if (update_filter.test(oldValue, newValue)) {
            if constexpr (use_pp_ds) {
                if (oldValue > newValue && coloring.is_colored(arc)) {
                    register_neighbors_for_post_processing(arc);
                } else if (oldValue < newValue && !coloring.is_colored(arc)) {
                    post_processor.register_arc(arc);
                }
            }
            return;
        }
        if (newValue > oldValue) {
            // Handle weight increases of uncolored arcs.
            if (!coloring.is_colored(arc)) {
                increaseWeight(arc, recursion_depth);
            }
        } else {
            // Handle weight decreases of colored arcs.
            // Note: deletions of colored arcs are handled here as well.
            // Deletions of uncolored arcs need no special treatment.
            if (coloring.is_colored(arc)) {
                decreaseWeight(arc);
            }
        }
    }

    virtual void run() override {
        if (post_process) {
            if constexpr (use_pp_ds) {
                post_processor.perform_post_processing(coloring, weights);
            } else {
                make_coloring_maximal_pq(coloring, diGraph, weights);
            }
        }
    }

private:
    bool attemptMatch(Arc *arc) {
        assert(!coloring.is_colored(arc));
        auto col = coloring.common_free_color(arc->getTail(), arc->getHead());
        if (col != color_set::npos) {
            coloring.color(arc, col);
            return true;
        }
        return false;
    }

    // Attempt to place `arc` in some matching.
    // Pre-condition: `coloring.is_colored(arc) == false`
    void increaseWeight(Arc *arc, int recurse = 0) {
        assert(!coloring.is_colored(arc));

        if (attemptMatch(arc)) {
            return;
        }
        auto [arc_data, max_color] = pick_pair_to_replace(arc); 
        if (arc_data.weight < (*weights)[arc]) {
            // Found a matching where matching `arc` instead of adjacent arcs is beneficial
            for (auto a: {arc_data.tail_arc, arc_data.head_arc}) {
                if (a != nullptr) {
                    coloring.uncolor(a);
                }
            }
            assert(coloring.can_color(arc, max_color));
            coloring.color(arc, max_color);
            if (recurse > 0) {
                for (auto a: {arc_data.tail_arc, arc_data.head_arc}) {
                    if (a != nullptr) {
                        increaseWeight(a, recurse - 1);
                    }
                }
            }
        } else if constexpr (use_pp_ds) {
            // `arc` remains uncolored and it's weight increased,
            // so the post-processing invariant might be violated now.
            post_processor.register_arc(arc);
        }
    }

    // Attempt to replace `arc` by heavier adjacent arcs in its matching.
    // Pre-condition: `coloring.is_colored(arc) == true`
    // Post-condition: `(*weights)[arc] == 0  ==> coloring.is_colored(arc) == false`
    void decreaseWeight(Arc *arc) {
        assert(coloring.is_colored(arc));

        // We know that `arc` is colored. If it's weight is 0,
        // we have to ensure that it is uncolored after this function.
        auto is_deletion = ((*weights)[arc] == 0);

        // Find heavy adjacent arcs that can replace `arc` in its matching.
        auto arc_color = coloring.get_color(arc);
        auto candidate_pair = find_heavy_candidates(arc, arc_color, (*weights)[arc]);
        bool colored_something_else = false;
        // Preemptively uncolor `arc`, so we can color the candidates, if they exist
        // This step also uncolors deleted arcs
        coloring.uncolor(arc);
        if (candidate_pair.tail_arc != nullptr) {
            assert(coloring.can_color(candidate_pair.tail_arc, arc_color));
            coloring.color(candidate_pair.tail_arc, arc_color);
            colored_something_else = true;
        }
        if (candidate_pair.head_arc != nullptr) {
            assert(coloring.can_color(candidate_pair.head_arc, arc_color));
            coloring.color(candidate_pair.head_arc, arc_color);
            colored_something_else = true;
        }
        // Try recoloring `arc` if it was not deleted.
        if (!is_deletion) {
            // If no candidates were colored, color `arc` with its original color.
            // Else, attempt to color it with something else using `increaseWeight`
            // (where the weight is essentially increased by 0).
            if (!colored_something_else) {
                coloring.color(arc, arc_color);
                if constexpr (use_pp_ds) {
                    register_neighbors_for_post_processing(arc);
                }
            } else {
                increaseWeight(arc, 0);
            }
        } else if constexpr (use_pp_ds) {
            register_neighbors_for_post_processing(arc);
        }

        // Assert that deleted arcs are uncolored.
        assert(!is_deletion || !coloring.is_colored(arc));
    }

    virtual void reset() override {
        algo_base::reset();
        rng_engine.seed(algo_base::matching_config->seed);
    }

private:
    const int recursion_depth;
    const bool post_process;
    
    const UpdateFilter update_filter;

    MaximalityPostProcessor<decltype(coloring)> post_processor;
    std::mt19937 rng_engine;
    std::vector<color_t> sampled_colors;

    AdjacentArcWeightPair find_heavy_candidates(Arc *arc, color_t arc_color, EdgeWeight weight_to_beat) {
        const auto arc_tail = arc->getTail(), arc_head = arc->getHead();
        std::vector<Arc*> candidates_tail, candidates_head;
        if constexpr (randomized > 0) {
            // Randomize the selection of candidates, i.e., just pick a few at random and hope that that's good engouh.
            for (auto [endpoint, cand]: {std::pair{arc_tail, candidates_tail},
                                        std::pair{arc_head, candidates_head}}) {
                cand.reserve(num_random_reps);
                const auto endpoint_vertex = static_cast<Algora::IncidenceListVertex*>(endpoint);
                const auto out_deg = endpoint_vertex->getOutDegree();
                const auto in_deg = endpoint_vertex->getInDegree();
                const auto out_prob = (double)out_deg/(double)(in_deg + out_deg);
                for (int i = 0; i < num_random_reps; ++i) {
                    Arc *cand_arc = nullptr;
                    if (std::uniform_real_distribution<double>{0,1}(rng_engine) < out_prob) {
                        auto index = std::uniform_int_distribution<size_t>{0,out_deg-1}(rng_engine);
                        cand_arc = endpoint_vertex->outgoingArcAt(index);
                    } else {
                        auto index = std::uniform_int_distribution<size_t>{0, in_deg-1}(rng_engine);
                        cand_arc = endpoint_vertex->incomingArcAt(index);
                    }
                    if (cand_arc != arc &&
                            !coloring.is_colored(cand_arc) &&
                            coloring.is_color_free(cand_arc->getOther(endpoint), arc_color)) {
                        cand.push_back(cand_arc);
                    }
                }
            }
        } else {
            // Select the candidates deterministically, i.e., consider all possible candidates.
            candidates_tail.reserve(diGraph->getDegree(arc_tail, false));
            candidates_head.reserve(diGraph->getDegree(arc_head, false));
            // Find all uncolored arcs that are potential candidates,
            // i.e., uncolored arcs where `arc_color` is free at the 'other' end.
            for (auto [endpoint, cand]: {std::pair{arc_tail, candidates_tail},
                                        std::pair{arc_head, candidates_head}}) {
                diGraph->mapIncidentArcs(endpoint, [this,
                                                    arc,
                                                    arc_color,
                                                    endpoint = endpoint,
                                                    &cand = cand](Arc *a) {
                    if (a != arc &&
                            !coloring.is_colored(a) &&
                            coloring.is_color_free(a->getOther(endpoint), arc_color)) {
                        cand.push_back(a);
                    }
                });
            }
        }
        // Sort candidates by non-increasing weight
        for (auto &cand: std::array{candidates_tail, candidates_head}) {
            std::sort(cand.begin(), cand.end(), [this](const Arc *lop, const Arc *rop) {
                return (*weights)[lop] > (*weights)[rop];
            });
        }

        AdjacentArcWeightPair return_value{nullptr, nullptr, 0};
        // find heaviest 'single' candidate
        bool found_heavy_pair = false;
        if (!candidates_tail.empty()) {
            return_value.tail_arc = candidates_tail.front();
            return_value.weight = (*weights)[return_value.tail_arc];
        }
        if (!candidates_head.empty() && (*weights)[candidates_head.front()] > return_value.weight) {
            // Check if the `tail_arc` doesn't overlap with the heaviest 'head candidate'.
            // If not, then we already found a valid pair of arcs to color.
            // If they overlap, then we remove `tail_arc` from the pair.
            if (return_value.tail_arc != nullptr &&
                return_value.tail_arc->getOther(arc_tail) != return_value.head_arc->getOther(arc_head)) {
                found_heavy_pair = true;
            } else {
                return_value.tail_arc = nullptr;
            }
            return_value.head_arc = candidates_head.front();
            return_value.weight = (*weights)[return_value.head_arc];
        }

        // We can return early if we found a heavy pair,
        // or there can't be a heavy pair since one of the candidate-lists is empty.
        // This avoids the expensive loops below.
        if (found_heavy_pair || candidates_tail.empty() || candidates_head.empty()) {
            return return_value;
        }

        // Find a heaviest pair of candidate arcs that is
        // 1. non-overlapping
        // 2. heavier than the weight over which we want to improve (`weight_to_beat`)
        for (size_t t_i = 0; t_i < candidates_tail.size(); ++t_i) {
            auto tail_cand_weight = (*weights)[candidates_tail[t_i]];
            // The inner-loop terminates as soon as the pair of candidates is
            // 1. lighter than the weight to beat
            // 2. lighter than the current best
            // In both cases we cannot find a better pair by continuing, since weights are sorted.
            for (size_t h_i = 0; h_i < candidates_head.size() &&
                                    tail_cand_weight + (*weights)[candidates_head[h_i]] > weight_to_beat &&
                                    tail_cand_weight + (*weights)[candidates_head[h_i]] > return_value.weight;
                                 ++h_i) {
                // Check if tail and head-candidates end in the same vertex,
                // otherwise, they are the new "best" pair
                if (candidates_tail[t_i]->getOther(arc_tail) != candidates_head[h_i]->getOther(arc_head)) {
                    return_value.tail_arc = candidates_tail[t_i];
                    return_value.head_arc = candidates_head[h_i];
                    return_value.weight = tail_cand_weight + (*weights)[candidates_head[h_i]];
                }
            }
        }

        return return_value;
    }

    std::pair<AdjacentArcWeightPair, color_t> pick_pair_to_replace(Arc *arc) {
        if constexpr (randomized > 0) {
            return pick_lightest_of_random_colors(num_random_reps, arc);
        } else {
            return coloring.lightest_adjacent_colored_arcs(arc, weights); 
        }
    }

    std::pair<AdjacentArcWeightPair, color_t> pick_lightest_of_random_colors(color_t /*num_choices*/, Arc *arc) {
//        auto color_range = coloring.color_range();
//        sampled_colors.clear();
//        std::sample(color_range.begin(),
//                    color_range.end(),
//                    std::back_inserter(sampled_colors),
//                    num_choices,
//                    rng_engine);
        auto min_color = UNCOLORED;
        AdjacentArcWeightPair result;
        result.weight = std::numeric_limits<EdgeWeight>::max();
 //       for (const auto &col: sampled_colors) {
        for (int i = 0; i < num_random_reps; ++i) {
            auto col = std::uniform_int_distribution<color_t>{0, coloring.getNumColors() - 1}(rng_engine);
            auto tail_mate = coloring.getArcToMate(col, arc->getTail());
            auto head_mate = coloring.getArcToMate(col, arc->getHead());
            EdgeWeight w = 0;
            for (const auto &a: {tail_mate, head_mate}) {
                if (a != nullptr) {
                    w += (*weights)[a];
                }
            }
            if (w < result.weight) {
                min_color = col;
                result.head_arc = head_mate;
                result.tail_arc = tail_mate;
                result.weight = w;
            }
        }
        return {result, min_color};
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
