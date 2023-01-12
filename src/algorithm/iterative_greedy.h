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
 *   based on code by Jonathan Trummer
 */


#pragma once

#include <algorithm>
#include <vector>

#include "algorithm/disjoint_matching_algorithm.h"

template<bool local_swaps, bool measure_color_ops>
class IterativeGreedy : public DisjointMatchingAlgorithm<measure_color_ops> {
    using algo_base = DisjointMatchingAlgorithm<measure_color_ops>;

    using algo_base::diGraph;
    using algo_base::weights;
    using algo_base::coloring;

public:

    virtual std::string getName() const noexcept override {
        auto name = std::string{"GreedyIt"};
        if constexpr (local_swaps) {
            name += "-local";
        }
        return name;
    }

    virtual std::string getShortName() const noexcept override {
        auto name = std::string{"GrIt"};
        if constexpr (local_swaps) {
            name += "-loc";
        }
        return name;
    }

    virtual void run() override {
        algo_base::reset();
        std::vector<Arc*> arcs_sorted;
        arcs_sorted.reserve(diGraph->getNumArcs(false));
        diGraph->mapArcs([this, &arcs_sorted](Arc *arc) {
            if ((*weights)[arc] > 0) {
                arcs_sorted.push_back(arc);
            }
        });

        std::sort(arcs_sorted.begin(), arcs_sorted.end(), [this](const Arc *lop, const Arc *rop) {
            return (*weights)[lop] > (*weights)[rop];
        });

        auto num_colors = coloring.getNumColors();
        std::vector<Arc*> remaining_arcs;
        std::vector<Arc*> recently_matched;
        remaining_arcs.reserve(arcs_sorted.size());
        if constexpr (local_swaps) {
            recently_matched.reserve(arcs_sorted.size());
        }
        for (auto color = 0u; color < num_colors; ++color) {
            for (const auto arc: arcs_sorted) {
                if (coloring.is_colored(arc)) {
                    continue;
                }
                if (coloring.can_color(arc, color)) {
                    coloring.color(arc, color);
                    if constexpr (local_swaps) {
                        recently_matched.push_back(arc);
                    }
                } else {
                    remaining_arcs.push_back(arc);
                }
            }

            auto swapped = false;
            if constexpr (local_swaps) {
                // TODO: swaps_reverse_sort
                for (auto *arc: recently_matched) {
                    swapped |= coloring.local_swap(arc);
                }
                recently_matched.clear();
            }

            if (!swapped) {
                std::swap(arcs_sorted, remaining_arcs);
            }
            remaining_arcs.clear();
        }

        // TODO: global swaps?

    }

};
