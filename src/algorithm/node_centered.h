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
#include "datastructure/kcoloring_extensions.h"
#include "tools/aggregation.h"
#include "tools/utility.h"

template<AggregateType aggregation_type, bool measure_color_ops = false>
class NodeCentered : public DisjointMatchingAlgorithm<measure_color_ops, FreeColorsExtension> {
    using algo_base = DisjointMatchingAlgorithm<measure_color_ops, FreeColorsExtension>;

    using algo_base::diGraph;
    using algo_base::weights;
    using algo_base::coloring;

public:
    NodeCentered(double threshold) : threshold(std::clamp(threshold, 0.0, 1.0)) {}

    virtual std::string getName() const noexcept override {
        auto name = std::string{"NodeCentered-"} +
            aggregate_names[aggregation_type] + "-" +
            to_string_with_precision(threshold, 1);
        return name;
    }

    virtual std::string getShortName() const noexcept override {
        auto name = std::string{"NC-"} +
            aggregate_names[aggregation_type] + "-" +
            to_string_with_precision(threshold, 1);
        return name;
    }

    virtual void reset() override {
        algo_base::reset();
        global_max = 0;
        nodes.clear();
        edges.resetAll();
        node_weights.setDefaultValue(0);
        node_weights.resetAll();
    }

    virtual void run() override {
        reset();

        prepare_nodes();
        auto global_threshold = global_max * threshold;

        std::vector<Arc*> remaining_edges;
        remaining_edges.reserve(edges.size());

        for (auto v: nodes) {
            for(auto arc: edges[v]) {
                if (coloring.no_color_free(v)) {
                    // We ran out of colors for this vertex
                    break;
                }
                if (coloring.is_colored(arc)) {
                    continue;
                }
                if ((*weights)[arc] >= global_threshold) {
                    const auto common_color = coloring.common_free_color(arc->getTail(),
                                                                         arc->getHead());
                    if (common_color != color_set::npos) {
                        coloring.color(arc, common_color);
                    }
                } else {
                    remaining_edges.push_back(arc);
                }
            }
        }

        if (!remaining_edges.empty()) {
            std::sort(remaining_edges.begin(), remaining_edges.end(), [this](Arc* lop, Arc* rop){
                return (*weights)[lop] > (*weights)[rop];
            });

            for (auto arc: remaining_edges) {
                if (coloring.no_color_free(arc->getTail()) ||
                        coloring.no_color_free(arc->getHead()) ||
                        coloring.is_colored(arc)) {
                    continue;
                }
                auto common_color = coloring.common_free_color(arc->getTail(),
                                                               arc->getHead());
                if (common_color != color_set::npos) {
                    coloring.color(arc, common_color);
                }       
            }
        }
    }

private:
    void prepare_nodes() {
        nodes.reserve(diGraph->getSize());

        diGraph->mapVertices([this] (Vertex* v) {
            edges[v].reserve(diGraph->getDegree(v, false));
            diGraph->mapIncidentArcs(v, [this,v](Arc *arc) {
                if ((*weights)[arc] > 0) {
                    edges[v].push_back(arc);
                }
            });

            if (edges[v].empty()) {
                return;
            }

            nodes.push_back(v);
            std::sort(edges[v].begin(), edges[v].end(), [this] (Arc * lop, Arc * rop) {
                return weights->getValue(lop) > weights->getValue(rop);
            });
            global_max = std::max(global_max, (*weights)[edges[v].front()]);

            node_weights[v] = aggregateWeights(edges[v],
                                               *weights,
                                               aggregation_type,
                                               coloring.getNumColors());
        });
        std::sort(nodes.begin(), nodes.end(), [this](Vertex* lop, Vertex* rop) {
            return node_weights[lop] > node_weights[rop];
        });

    }


private:
    const double threshold = 0.2;  // TODO: make this a parameter // TODO: ensure threshold >= 0 at configuration time

    EdgeWeight global_max = 0;
    std::vector<Vertex*> nodes;
    FastPropertyMap<std::vector<Arc*>> edges;
    FastPropertyMap<EdgeWeight> node_weights;


};
