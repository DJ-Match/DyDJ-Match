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

#include <cassert>
#include <functional>
#include <tuple>
#include <vector>

#include "boost/range/irange.hpp"

#include "graph/vertex.h"
#include "graph/digraph.h"
#include "property/fastpropertymap.h"
#include "property/modifiableproperty.h"

#include "algorithm/matching_defs.h"
#include "tools/color_set.h"

using namespace Algora;

/*
 * A datastructure to store incomplete edge-colorings with `k` colors.
 */
template<typename... Ext>
class KColoring : public Ext... {

public:
    // Number of extensions
    static constexpr auto num_ext = std::tuple_size_v<std::tuple<Ext...>>;

public:
    KColoring(DiGraph *graph,
              ModifiableProperty<EdgeWeight> *weights,
              color_t num_colors) : graph(graph),
                                    weights(weights),
                                    num_colors(num_colors),
                                    mates_by_color(num_colors, {nullptr}) {
        if (weights != nullptr) {
            weights->onPropertyChange(this, std::bind(&KColoring::onEdgeWeightChange,
                                                    this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3));
        }
    }

    void reset() {
        arc_colors.setDefaultValue(UNCOLORED);
        arc_colors.resetAll();
        mates_by_color.resize(num_colors);
        for(auto &map: mates_by_color) {
            map.setDefaultValue(nullptr);
            map.resetAll();
        }
        total_weight = 0;

        (Ext::reset_impl(), ...);
    }

    // Return `true` if `vertex` does not have an incident arc colored with `color`,
    // `false` otherwise.
    bool is_color_free(Vertex* vertex, color_t color) const {
        return color != UNCOLORED && mates_by_color[color][vertex] == nullptr;
    }

    color_t get_color(Arc* arc) const {
        return arc_colors[arc];
    }

    // Return `true` if the arc has a valid color assigned, `false` otherwise
    bool is_colored(Arc* arc) const {
        assert(graph->containsArc(arc));
        //return arc_colors[arc] < num_colors;
        return arc_colors[arc] < num_colors;
    }

    // Return `true` if `arc` has no mates for `color`,
    // `false` otherwise.
    bool can_color(Arc* arc, color_t color) const {
        return is_color_free(arc->getTail(), color) &&
               is_color_free(arc->getHead(), color);
    }

    // Assign the edge-color `color` to `arc`.
    // Post-condition: `is_colored(arc) == true`
    void color(Arc* arc, color_t color) {
        assert(!is_colored(arc));
        assert(color < num_colors);
        assert(is_color_free(arc->getTail(), color));
        assert(is_color_free(arc->getHead(), color));

        if (arc_colors[arc] == UNCOLORED) {
            total_weight += (*weights)[arc];
        }
        arc_colors.setValue(arc, color);
        mates_by_color[color][arc->getHead()] = arc->getTail();
        mates_by_color[color][arc->getTail()] = arc->getHead();

        (Ext::color_impl(arc, color), ...);

        assert(is_colored(arc));
    }

    // Remove the edge-color assignment from `arc`, making it uncolored.
    // Pre-condition: `is_colored(arc) == true`
    // Post-condition: `is_colored(arc) == false`
    void uncolor(Arc* arc) {
        assert(is_colored(arc));
        auto color = arc_colors[arc];
        arc_colors.setValue(arc, UNCOLORED);
        mates_by_color[color][arc->getHead()] = nullptr;
        mates_by_color[color][arc->getTail()] = nullptr;
        total_weight -= (*weights)[arc];

        (Ext::uncolor_impl(arc, color), ...);

        assert(!is_colored(arc));
    }

    // Perform a local swap on `arc`.
    // This attempts to swap `arc` by two adjacent arcs that are heavier in combination.
    // Returns `true` is successful, `false` otherwise.
    bool local_swap(Arc * const arc) {
        assert(is_colored(arc));

        auto tail = arc->getTail();
        auto head = arc->getHead();
        auto arc_color = arc_colors[arc];
        EdgeWeight tail_weight = 0, head_weight = 0;
        Arc *tail_arc = nullptr, *head_arc = nullptr;
        Vertex *tail_arc_target = nullptr;

        // Find the heaviest free arc incident to the tail of `arc`
        graph->mapIncidentArcs(tail, [&](Arc *candidate){
            if (candidate == arc || is_colored(candidate)) { return; }
            auto t2 = candidate->getOther(tail);
            if (is_color_free(t2, arc_color)) {
                auto weight = (*weights)[candidate];
                if (weight > tail_weight) {
                    tail_arc = candidate;
                    tail_weight = weight;
                    tail_arc_target = t2;
                }
            }
        });
        // Find the heaviest free arc incident to the head of `arc` that doesn't overlap with `tail_arc`.
        graph->mapIncidentArcs(head, [&](Arc *candidate) {
            if (candidate == arc || is_colored(candidate)) { return; }
            auto t2 = candidate->getOther(head);
            if (is_color_free(t2, arc_color) && t2 != tail_arc_target) {
                auto weight = (*weights)[candidate];
                if (weight > head_weight) {
                    head_arc = candidate;
                    head_weight = weight;
                }
            }
        });

        if (head_weight + tail_weight > (*weights)[arc]) {
            uncolor(arc);
            if (tail_arc != nullptr) {
                color(tail_arc, arc_color);
            }
            if (head_arc != nullptr) {
                color(head_arc, arc_color);
            }
            return true;
        }

        return false;
    }


    auto color_range() const {
        return boost::irange<color_t>(0, num_colors);
    }


    void setGraph(DiGraph *graph) {
        this->graph = graph;
    }

    void setWeights(ModifiableProperty<EdgeWeight> *weights) {
        this->weights = weights;
        this->weights->onPropertyChange(this, std::bind(&KColoring::onEdgeWeightChange,
                                                        this,
                                                        std::placeholders::_1,
                                                        std::placeholders::_2,
                                                        std::placeholders::_3));
    }

    void unsetGraph() {
        this->graph = nullptr;
    }

    void unsetWeights() {
        if (this->weights != nullptr) {
            this->weights->removeOnPropertyChange(this);
            this->weights = nullptr;
        }
    }

    void setNumColors(color_t num_colors) {
        this->num_colors = num_colors;

        (Ext::setNumColors_impl(num_colors), ...);
    }

    color_t getNumColors() const {
        return num_colors;
    }

    EdgeWeight getTotalWeight() const {
        return total_weight;
    }

    void sanityCheck() {
        checkIncidentEdges();
        checkSolutionWeight();
    };

protected:

    // Update the total weight when the weight of an arc changes
    void onEdgeWeightChange(GraphArtifact* artifact, EdgeWeight old_value, EdgeWeight new_value) {
        auto arc = static_cast<Arc*>(artifact);
        assert(graph->containsArc(arc));
        if (is_colored(arc)) {
            total_weight -= old_value;
            total_weight += new_value;
        }
    }

    void checkIncidentEdges() {
        graph->mapVertices([this](Vertex* vertex) {
            color_set unused_colors{num_colors};
            graph->mapIncidentArcs(vertex, [this, &unused_colors, vertex](Arc* arc){
                if (is_colored(arc)) {
                    auto c = arc_colors[arc];
                    if (!unused_colors[c]) {
                        std::cout << "Color " << c << " used at least twice on vertex " << vertex << ":" << std::endl;
                        graph->mapIncidentArcs(vertex, [this, c](Arc* a) {
                            if (arc_colors[a] == c) {
                                std::cout << "  on arc " << a << std::endl;
                            }
                        });
                    }
                    unused_colors.setOff(c);
                }
            });
        });
    }

    void checkSolutionWeight() {
        EdgeWeight check_weight = 0;
        graph->mapVertices([this, &check_weight](Vertex* vertex) {
            // Map outgoing arcs only so we don't count arcs twice.
            // This is also the reason for not using `mapArcs` directly.
            graph->mapOutgoingArcs(vertex, [this, &check_weight](Arc* arc) {
                if (is_colored(arc)) {
                    check_weight += (*weights)[arc];
                }
            });
        });
        if (check_weight != total_weight) {
            std::cout << "Solution weight is " << total_weight << ", but the true weight is " << check_weight << std::endl;
        }
    }

private:
    // The graph underlying this coloring
    DiGraph* graph;
    // The edge-weights associated with `graph`
    ModifiableProperty<EdgeWeight>* weights;

    // How many colors can be used
    color_t num_colors;

    // Map from edges to colors.
    // Values are either in `[0,num_colors-1]`, or `UNCOLORED`
    FastPropertyMap<color_t> arc_colors{UNCOLORED};

    // Sum of weights of all colored edges
    EdgeWeight total_weight = 0;

    std::vector<FastPropertyMap<Vertex*>> mates_by_color;

};
