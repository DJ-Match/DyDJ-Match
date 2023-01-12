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

#include "algorithm/disjoint_matching_algorithm.h"
#include "tools/utility.h"

template<bool local_swaps, bool measure_color_ops = false>
class BatchIterativeGreedy : public DisjointMatchingAlgorithm<measure_color_ops> {
    using algo_base = DisjointMatchingAlgorithm<measure_color_ops>;

    using algo_base::diGraph;
    using algo_base::weights;
    using algo_base::coloring;

public:
    virtual std::string getName() const noexcept override {
        std::string name = "batch_greedy";
        if (local_swaps) {
            name += "-loc";
        }
        return name;
    }

    virtual std::string getShortName() const noexcept override {
        std::string name = "bat_gr";
        if (local_swaps) {
            name += "-l";
        }
        return name;
    }

    virtual void reset() override {
        algo_base::reset();
        update_marker.reset();
        arcs_to_process.reset();
    }

    virtual void onPropertyChange(GraphArtifact *artifact,
                                  const EdgeWeight &/*oldValue*/,
                                  const EdgeWeight &newValue) override {
        auto arc = static_cast<Arc*>(artifact);
        // Ensure deleted arcs are being un colored
        if (newValue == 0 && coloring.is_colored(arc)) {
            coloring.uncolor(arc);
        }
        if (!update_marker.is_marked(arc)) {
            arcs_to_process.add(arc);
            if (coloring.is_colored(arc)) {
                coloring.uncolor(arc);
            }
            for (auto endpoint: {arc->getTail(), arc->getHead()}) {
                diGraph->mapIncidentArcs(endpoint, [this](Arc *a) {
                    arcs_to_process.add(a);
                    if (coloring.is_colored(a)) {
                        coloring.uncolor(a);
                    }
                });
            }
        }
    }

    virtual void run() override {
        auto& arcs_vector = arcs_to_process.vector();
        std::sort(arcs_vector.begin(),
                  arcs_vector.end(),
                  [this](Arc* lop, Arc* rop) {
            return (*weights)[lop] > (*weights)[rop];
        });
        // Find the first zero-weight arc
        auto first_zero = std::find_if(arcs_vector.begin(),
                                       arcs_vector.end(),
                                       [this](Arc* val) {
            return (*weights)[val] == 0;
        });
        // Erase all zero-weight arcs
        arcs_vector.erase(first_zero, arcs_vector.end());

        std::vector<Arc*> remaining_arcs;
        std::vector<Arc*> recently_matched;
        remaining_arcs.reserve(arcs_vector.size());
        recently_matched.reserve(arcs_vector.size());
        for (auto col: coloring.color_range()) {
            // Color uncolored arcs greedily
            for (auto arc: arcs_vector) {
                if (coloring.is_colored(arc)) {
                    continue;
                }
                if (coloring.can_color(arc, col)) {
                    coloring.color(arc, col);
                    recently_matched.push_back(arc);
                } else {
                    remaining_arcs.push_back(arc);
                }
            }
            
            // Perform local swaps on arcs matched in this round
            bool swapped = false;
            if constexpr (local_swaps) {
                for (auto arc: recently_matched) {
                    swapped = coloring.local_swap(arc);
                }
            }
            // If no swaps happened, only consider remaining arcs for the next color.
            // Else, continue with all arcs
            if (!swapped) {
                std::swap(arcs_vector, remaining_arcs);
            }

            remaining_arcs.clear();
            recently_matched.clear();
        }

        update_marker.next_round();
        arcs_to_process.next_round();
    }

private:
    // Mark arcs that have been updated in the current Delta
    ArtifactMarker<Arc*> update_marker;
    // Edges to be processed in the batch.
    TimedArtifactSet<Arc*> arcs_to_process;

};
