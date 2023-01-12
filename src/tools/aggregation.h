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

#include <numeric>

#include "algorithm/matching_defs.h"

inline unsigned long aggregateWeights(std::vector<Algora::Arc*> &edges,
                                      const Algora::ModifiableProperty<EdgeWeight> &weight,
                                      const AggregateType type,
                                      const unsigned b) {
    const auto size = edges.size();
    assert(size > 0);
    if (size == 1) {
        return weight(edges[0]);
    }
    auto weightSum = [&weight](EdgeWeight acc, const Algora::Arc *a) { return std::move(acc) + weight(a); };
    switch(type) {
        case AggregateType::AVG:
            return (std::accumulate(edges.begin(), edges.end(), weight(edges[0]), weightSum) / size);
        case AggregateType::MEDIAN: {
            return size % 2 != 0
                ?  weight(edges[size/2])
                : (weight(edges[size/2]) + weight(edges[size/2-1])) / 2UL;
        }
        case AggregateType::MAX:
            return weight(edges.front());
        case AggregateType::B_SUM:
            return std::accumulate(edges.begin(),
                                    edges.begin() + std::min(size, (size_t)b),
                                    weight(edges[0]),
                                    weightSum);
        case AggregateType::SUM:
        default:
            return std::accumulate(edges.begin(), edges.end(), weight(edges[0]), weightSum);
    }
}
