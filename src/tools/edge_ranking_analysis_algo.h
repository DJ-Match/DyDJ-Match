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

#include <vector>

#include "algorithm/disjoint_matching_algorithm.h"
#include "tools/utility.h"

class RankingAnalysisAlgo : public DisjointMatchingAlgorithm<false> {
    using algo_base = DisjointMatchingAlgorithm<false>;

public:
    std::string getName() const noexcept override {
        return "ranking-analyzer";
    }
    
    std::string getShortName() const noexcept override {
        return getName();
    }

    void reset() override {
        edges.clear();
        total_edge_weight = 0;
        heavy_edge_weight = 0;
    }

    void onPropertyChange(GraphArtifact * /*artifact*/,
                          const EdgeWeight &/*oldValue*/,
                          const EdgeWeight &/*newValue*/) override {
    }

    void run() override {
        edges.clear();
        edges.reserve(diGraph->getNumArcs(false));
        diGraph->mapArcs([this](Arc *arc) {
            total_edge_weight += (*weights)[arc];
            edges.push_back(arc);
        });
        auto num_heavy_arcs = diGraph->getNumArcs(false)/10;
        std::sort(edges.begin(), edges.end(), [this](const Arc *a, const Arc *b) {
            return (*weights)[a] < (*weights)[b];
        });
        edges.resize(num_heavy_arcs);
        heavy_edge_weight = std::accumulate(edges.begin(), edges.end(),
                                            (EdgeWeight)0, [this](const EdgeWeight &w, const Arc *a) {
                                                return w + (*weights)[a];
                                            });
    }

    void custom_output(std::ostream &stream) const override {
        stream << "total_weight: " << total_edge_weight
               << "; heavy_weight: " << heavy_edge_weight
               << "; ranking: ";
        for (auto arc: edges) {
            stream << "(" << arc->getTail()->getId() << "," << arc->getHead()->getId() << ") ";
        }
        stream << std::endl;
    }

private:
    std::vector<Arc*> edges;
    EdgeWeight total_edge_weight = 0;
    EdgeWeight heavy_edge_weight = 0;
};
