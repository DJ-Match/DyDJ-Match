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
#include "datastructure/kcoloring_extensions.h"
#include "tools/aggregation.h"
#include "tools/utility.h"

template<AggregateType aggregation_type, bool measure_color_ops = false>
class BatchNodeCentered_2 : public DisjointMatchingAlgorithm<measure_color_ops, FreeColorsExtension> {
    using algo_base = DisjointMatchingAlgorithm<measure_color_ops, FreeColorsExtension>;

    using algo_base::diGraph;
    using algo_base::weights;
    using algo_base::coloring;

public:
    BatchNodeCentered_2(double threshold) : threshold(std::clamp(threshold, 0.0, 1.0)) {}

    virtual std::string getName() const noexcept override {
        auto name = std::string{"Batch-NodeCentered-"} +
            aggregate_names[aggregation_type] + "-" +
            to_string_with_precision(threshold, 1);
        return name;
    }

    virtual std::string getShortName() const noexcept override {
        auto name = std::string{"bat-NC-"} +
            aggregate_names[aggregation_type] + "-" +
            to_string_with_precision(threshold, 1);
        return name;
    }

    virtual void reset() override {
        algo_base::reset();
        vertices_to_process.reset();
        incidence_lists.setDefaultValue({});
        incidence_lists.resetAll();
    }

    virtual void onPropertyChange(GraphArtifact *artifact,
                                  const EdgeWeight &/*oldValue*/,
                                  const EdgeWeight &newValue) override {
        const auto arc = static_cast<Arc*>(artifact);
        if (newValue == 0 && coloring.is_colored(arc)) {
            coloring.uncolor(arc);
        }
        vertices_to_process.add(arc->getTail());
        vertices_to_process.add(arc->getHead());
    }

    virtual void run() override {
        global_max = 0;
        nodes.clear();
        incidence_lists.resetAll();

        prepare_nodes();

        std::vector<Arc*> remaining_edges;
        colorHeavyEdges(remaining_edges);
        colorLightEdges(remaining_edges);

        vertices_to_process.next_round();
    }

private:
    void prepare_nodes() {
        nodes.reserve(vertices_to_process.vector().size());
        for (auto vertex: vertices_to_process.vector()) {
            incidence_lists[vertex].reserve(diGraph->getDegree(vertex, false));
            diGraph->mapIncidentArcs(vertex, [this, vertex](Arc *arc) {
                if ((*weights)[arc] > 0) {
                    incidence_lists[vertex].push_back(arc);
                    if (coloring.is_colored(arc)) {
                        coloring.uncolor(arc);
                    }
                }
            });

            if (incidence_lists[vertex].empty()) {
                continue;
            }

            nodes.push_back(vertex);
            std::sort(incidence_lists[vertex].begin(), incidence_lists[vertex].end(), [this](Arc* lop, Arc *rop) {
                return (*weights)[lop] > (*weights)[rop];
            });
            global_max = std::max(global_max, (*weights)[incidence_lists[vertex].front()]);

            node_weights[vertex] = aggregateWeights(incidence_lists[vertex],
                                                    *weights,
                                                    aggregation_type,
                                                    coloring.getNumColors());
        }
        std::sort(nodes.begin(), nodes.end(), [this](Vertex *lop, Vertex *rop) {
            return node_weights[lop] > node_weights[rop];
        });
    }

    void colorHeavyEdges(std::vector<Arc*> &remaining_edges) {
        auto global_threshold = global_max * threshold;
        for (auto v: nodes) {
            for (auto arc: incidence_lists[v]) {
                if (coloring.no_color_free(v)) {
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
    }

    void colorLightEdges(std::vector<Arc*> &remaining_edges) {
        std::sort(remaining_edges.begin(), remaining_edges.end(), [this](Arc *lop, Arc *rop) {
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

private:
    const double threshold;

    TimedArtifactSet<Vertex*> vertices_to_process;
    std::vector<Vertex*> nodes;
    FastPropertyMap<std::vector<Arc*>> incidence_lists;
    FastPropertyMap<EdgeWeight> node_weights;
    EdgeWeight global_max = 0;

};
