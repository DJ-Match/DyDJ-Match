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
 */

#pragma once

#include "algorithm/disjoint_matching_algorithm.h"
#include "tools/utility.h"

// A "DisjointMatchingAlgorithm" to compute statistics.
// At the moment just counts the number of updates after filtering.
class AnalysisAlgo : public DisjointMatchingAlgorithm<false> {
    using algo_base = DisjointMatchingAlgorithm<false>;

public:
    std::string getName() const noexcept override {
        return "analyzer" + to_string_with_precision(filter.getUpThreshold(), 1);
    }
    
    std::string getShortName() const noexcept override {
        return getName();
    }

    void reset() override {
        filtered_updates = 0;
        insertions = 0;
        deletions = 0;
        weight_changes = 0;
        reset_counters = false;
    }

    void onPropertyChange(GraphArtifact * /*artifact*/,
                          const EdgeWeight &oldValue,
                          const EdgeWeight &newValue) override {
        if (reset_counters) {
            reset();
        }
        if (oldValue == 0) {
            insertions++;
        } else if (newValue == 0) {
            deletions++;
        } else {
            weight_changes++;
        }
        if (!filter.test(oldValue, newValue)) {
            filtered_updates++;
        }
    }

    void run() override {
        reset_counters = true;
    }

    void custom_output(std::ostream &stream) const override {
        stream << "updates after filtering: " << filtered_updates
               << ", insertions: " << insertions
               << ", deletions: " << deletions
               << ", weight changes: " << weight_changes
               << std::endl;
    }

private:
    UpdateFilter filter{8.0};
    int filtered_updates = 0;
    int insertions = 0;
    int deletions = 0;
    int weight_changes = 0;
    bool reset_counters = false;

};
