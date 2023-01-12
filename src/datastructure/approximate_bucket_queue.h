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
#include <limits>

#include "property/fastpropertymap.h"

#include "algorithm/matching_defs.h"

// A bucket queue which sorts objects of type `Id` (should be a graph artifact) by priority (type `EdgeWeight`).
// Instead of using the actual priority, this queue sorts objects with the same base-2 logarithm into the same buckets.
// Thus, order can be off by a factor of 2.
// Operations are:
//  - push(id, priority): place `id` in the bucket queue with the given priority
//  - erase(id): remove `id` from the bucket queue
//  - update(id, priority): change the priority of `id`
//  - pop_max(): remove the value with highest priority from the queue (may be smaller than the actual highest priority by a factor 2).
// All operations are amortized constant time; amortized due to use of `std::vector<Id>`.
template<typename Id>
class ApproximateBucketQueue {

private:
    using index_type = std::pair<unsigned int, unsigned int>;
    
    static constexpr auto num_buckets = sizeof(EdgeWeight) * 8;
    static constexpr auto all_ones = std::numeric_limits<EdgeWeight>::max();

    constexpr auto bucket_from_priority(EdgeWeight priority) const {
        return __builtin_clzl(priority);
    }

public:

    ApproximateBucketQueue() {
    }

    void push(Id id, EdgeWeight priority) {
        assert(priority != 0);

        auto bucket = bucket_from_priority(priority);
        auto bucket_index = buckets[bucket].size();
        indices[id] = {bucket, bucket_index};
        buckets[bucket].push_back(id);
        filled_mask = filled_mask | (1ul << bucket);
        compute_greatest_nonempty_bucket();
    }

    void erase(Id id) {
        auto index = indices[id];
        auto swapped_element = buckets[index.first].back();
        indices[swapped_element] = index;
        std::swap(buckets[index.first][index.second], buckets[index.first].back());
        buckets[index.first].pop_back();
        if (buckets[index.first].empty()) {
            filled_mask = filled_mask & (all_ones ^ (1ul << index.first));
        } else {
            filled_mask = filled_mask | (1ul << index.first);
        }
        compute_greatest_nonempty_bucket();
    }

    void update(Id id, EdgeWeight priority) {
        assert(priority != 0);

        erase(id);
        push(id, priority);
    }

    bool empty() {
        return filled_mask == 0;
    }

    Id pop_max() {
        auto top = buckets[greatest_nonempty_bucket].back();
        erase(top);
        return top;
    }

    void clear() {
        for (auto &bucket: buckets) {
            bucket.clear();
        }
        indices.resetAll();
        filled_mask = 0;
    }

private:
    std::array<std::vector<Id>, num_buckets> buckets;
    FastPropertyMap<index_type> indices;

    EdgeWeight filled_mask = 0;
    
    unsigned int greatest_nonempty_bucket;

    inline void compute_greatest_nonempty_bucket() {
        greatest_nonempty_bucket = __builtin_ctzl(filled_mask);
    }

};
