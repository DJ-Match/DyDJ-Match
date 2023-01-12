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

#include <vector>
#include <utility>

#include "graph/arc.h"
#include "property/fastpropertymap.h"

#include "algorithm/matching_defs.h"
#include "tools/color_set.h"

using namespace Algora;

// Store for each vertex and each color the arc to the mate.
class ArcMateExtension {

public:

    std::pair<AdjacentArcWeightPair, color_t> lightest_adjacent_colored_arcs(const Arc *arc, ModifiableProperty<EdgeWeight> *weights) const {
        auto return_value = AdjacentArcWeightPair{nullptr, nullptr, std::numeric_limits<EdgeWeight>::max()};
        color_t min_color = UNCOLORED;
        for (size_t col = 0; col < arcs_to_mates_by_color.size(); ++col) {
            const auto &map = arcs_to_mates_by_color[col];
            auto tail_arc = map[arc->getTail()];
            auto head_arc = map[arc->getHead()];
            EdgeWeight weight = 0;
            for (auto a: {tail_arc, head_arc}) {
                if (a != nullptr) {
                    weight += (*weights)[a];
                }
            }
            if (weight < return_value.weight) {
                return_value = {tail_arc, head_arc, weight};
                min_color = col;
            }
        }
        return {return_value, min_color};
    }

    Arc* getArcToMate(color_t col, Vertex* vertex) const {
        return arcs_to_mates_by_color[col][vertex];
    }

    std::vector<Arc*> getColoredArcs(Vertex* vertex) const {
        std::vector<Arc*> colored_arcs;
        colored_arcs.reserve(arcs_to_mates_by_color.size());
        for (const auto &map: arcs_to_mates_by_color) {
            if (map[vertex] != nullptr) {
                colored_arcs.push_back(map[vertex]);
            }
        }
        return colored_arcs;
    }

    Arc* getLightestColoredEdge(Vertex* vertex, ModifiableProperty<EdgeWeight> *weights) {
        Arc *lightest = nullptr;
        EdgeWeight min_weight = std::numeric_limits<EdgeWeight>::max();
        for (const auto &map: arcs_to_mates_by_color) {
            if (map[vertex] != nullptr && (*weights)[map[vertex]] < min_weight) {
                lightest = map[vertex];
                min_weight = (*weights)[map[vertex]];
            }
        }
        return lightest;
    }

protected:
    void reset_impl() {
        for(auto &map: arcs_to_mates_by_color) {
            map.setDefaultValue(nullptr);
            map.resetAll();
        }
    }

    void color_impl(Arc *arc, color_t color) {
        assert(arcs_to_mates_by_color[color][arc->getHead()] == nullptr);
        assert(arcs_to_mates_by_color[color][arc->getTail()] == nullptr);
        arcs_to_mates_by_color[color][arc->getHead()] = arc;
        arcs_to_mates_by_color[color][arc->getTail()] = arc;
    }

    void uncolor_impl(Arc *arc, color_t pre_color) {
        assert(arcs_to_mates_by_color[pre_color][arc->getHead()] != nullptr);
        assert(arcs_to_mates_by_color[pre_color][arc->getTail()] != nullptr);
        arcs_to_mates_by_color[pre_color][arc->getHead()] = nullptr;
        arcs_to_mates_by_color[pre_color][arc->getTail()] = nullptr;
    }

    void setNumColors_impl(color_t num_colors) {
        arcs_to_mates_by_color.resize(num_colors);
        for (auto &map: arcs_to_mates_by_color) {
            map.setDefaultValue(nullptr);
        }
    }

private:
    // Arcs to mates, for each color
    std::vector<FastPropertyMap<Arc*>> arcs_to_mates_by_color;
};

// Store for each vertex the colors that are still free.
class FreeColorsExtension {

public:
    color_t get_any_free_color(Vertex* v) const {
        return free_colors[v].find_first();
    }

    color_t common_free_color(Vertex* v1, Vertex* v2) const {
        return color_set::common_colors(free_colors[v1], free_colors[v2]).find_first();
    }

    bool any_color_free(Vertex* v1) const {
        return free_colors[v1].any();
    }
    bool all_colors_free(Vertex* v1) const {
        return free_colors[v1].all();
    }
    bool no_color_free(Vertex* v1) const {
        return free_colors[v1].none();
    }

    const color_set& get_free_colors(Vertex* v1) const {
        return free_colors[v1];
    }

protected:
    void reset_impl() {
        free_colors.resetAll();
    }

    void color_impl(Arc *arc, color_t color) {
        assert(free_colors[arc->getTail()][color]);
        assert(free_colors[arc->getHead()][color]);
        free_colors[arc->getTail()].setOff(color);
        free_colors[arc->getHead()].setOff(color);
    }

    void uncolor_impl(Arc *arc, color_t pre_color) {
        assert(!free_colors[arc->getTail()][pre_color]);
        assert(!free_colors[arc->getHead()][pre_color]);
        free_colors[arc->getTail()].setOn(pre_color);
        free_colors[arc->getHead()].setOn(pre_color);
    }

    void setNumColors_impl(color_t num_colors) {
        free_colors.setDefaultValue(color_set{num_colors});
        free_colors.resetAll();
    }

private:
    FastPropertyMap<color_set> free_colors;
};

class ColoringStatsExtension {

public:
    struct color_op_counts {
        int color_count = 0;
        int uncolor_count = 0;
        int recolor_count = 0;
    };

public:
    void compute_coarse_counts_and_reset() {
        coarse_counts = {0, 0, 0};
        for (auto &diff: arc_color_changes) {
            if (diff.first == UNCOLORED && diff.second != UNCOLORED) {
                coarse_counts.color_count++;
            } else if (diff.first != UNCOLORED && diff.second == UNCOLORED) {
                coarse_counts.uncolor_count++;
            } else if (diff.first != diff.second &&
                    diff.first != UNCOLORED &&
                    diff.second != UNCOLORED) {
                coarse_counts.recolor_count++;
            }
            // Make the 'new' color the 'old' color so we can keep track of the next changes
            diff.first = diff.second;
            diff.second = UNCOLORED;
        }
    }

    void reset_arc_diffs() {
        arc_color_changes.setDefaultValue({UNCOLORED, UNCOLORED});
        arc_color_changes.resetAll();
    }

    void reset_fine_counts() {
        fine_counts = {0, 0, 0};
    }

    color_op_counts get_coarse_counts() const {
        return coarse_counts;
    }

    color_op_counts get_fine_counts() const {
        return fine_counts;
    }

protected:
    void reset_impl() {
        fine_counts = {0, 0, 0};
        coarse_counts = {0, 0, 0};
    }

    void color_impl(Arc *arc, color_t color) {
        fine_counts.color_count++;
        arc_color_changes[arc].second = color;
    }

    void uncolor_impl(Arc *arc, color_t /*pre_color*/) {
        fine_counts.uncolor_count++;
        arc_color_changes[arc].second = UNCOLORED;
    }

    void setNumColors_impl(color_t /*num_colors*/) {}

private:
    FastPropertyMap<std::pair<color_t, color_t>> arc_color_changes{{UNCOLORED, UNCOLORED}};

    color_op_counts fine_counts;
    color_op_counts coarse_counts;
};
