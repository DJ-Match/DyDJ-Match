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
 */

#pragma once

#include <limits>
#include <type_traits>

#include "graph/arc.h"

enum AggregateType {SUM,MAX,AVG,MEDIAN,B_SUM};
const std::string aggregate_names[] = {"SUM", "MAX", "AVG", "MEDIAN", "B_SUM"};

typedef unsigned long int EdgeWeight;
// User-defined integer literal for the `EdgeWeight` type.
inline constexpr EdgeWeight operator""_ew(unsigned long long value) {
    return value;
}

using color_t = unsigned int;
constexpr color_t UNCOLORED = std::numeric_limits<color_t>::max();

using arc_color_pair = std::pair<Algora::Arc*, color_t>;

// Helper struct defining a pair of arcs with their total weight.
// Intended for finding heaviest/lightest arc-pairs adjacent to another arc.
struct AdjacentArcWeightPair {
    Algora::Arc *tail_arc = nullptr;
    Algora::Arc *head_arc = nullptr;
    EdgeWeight weight = 0;
};
