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

#include "boost/heap/d_ary_heap.hpp"

#include "algorithm/disjoint_matching_algorithm.h"
#include "datastructure/kcoloring_extensions.h"
#include "datastructure/kcoloring_utilities.h"
#include "datastructure/approximate_bucket_queue.h"
#include "tools/utility.h"

// A greedy batch-algorithm that works by ensuring the invariant for a 1/2-approximation.
// This invariant is as follows:
//   Every uncolored arc `x` has for every color `c` at least one adjacent arc `y` such that `w(x) <= w(y)`.
//
// Edges for which the invariant may have been invalidated by an update are stored in a priority queue.
// After all updates have been applied, the priority queue is processed as in `make_coloring_maximal_pq`.
template<bool measure_color_ops = false>
class InvariantGreedy : public DisjointMatchingAlgorithm<measure_color_ops, ArcMateExtension, FreeColorsExtension> {

private:
    using algo_base = DisjointMatchingAlgorithm<measure_color_ops, ArcMateExtension, FreeColorsExtension>;

    using algo_base::diGraph;
    using algo_base::weights;
    using algo_base::coloring;

//    using pq_type = ApproximateBucketQueue<Arc*>;
    using pq_type = make_maximal_detail::pq_type;

public:

    std::string getName() const noexcept override {
        auto name = std::string{"batch-invariant-greedy"};
        return name;
    }

    std::string getShortName() const noexcept override {
        auto name = std::string{"bat-inv-gr"};
        return name;
    }

    void onPropertyChange(GraphArtifact *artifact,
                          const EdgeWeight &oldValue,
                          const EdgeWeight &newValue) override {
        auto arc = static_cast<Arc*>(artifact);
        if (oldValue < newValue && !coloring.is_colored(arc)) {
            arcs_to_update.add(arc);
        } else if (oldValue > newValue && coloring.is_colored(arc)) {
            for (auto vertex: {arc->getTail(), arc->getHead()}) {
                // Push uncolored arcs adjacent to ` arc` into the queue.
                // `push_to_queue` ensures that no arc is pushed twice.
                diGraph->mapIncidentArcs(vertex, [this, arc](Arc *a) {
                    if (a != arc && !coloring.is_colored(a)) {
                        arcs_to_update.add(a);
                    }
                });
            }
        }
        if (newValue == 0 && coloring.is_colored(arc)) {
            coloring.uncolor(arc);
        }
    }

    void run() override {
        for (auto arc: arcs_to_update.vector()) {
            if (auto arc_weight = (*weights)[arc]; arc_weight > 0) {
                arc_queue.push({arc, arc_weight});
            }
        }
        make_maximal_detail::process_maximal_pq(arc_queue, coloring, weights);
        
        // Reset the helper data-structures
        arcs_to_update.next_round();
    }

    void reset() override {
        algo_base::reset();
        #ifdef USE_BOOST_HEAP
            arc_queue.clear();
        #else
            arc_queue = {};
        #endif
        arcs_to_update.reset();
    }

private:
    pq_type arc_queue;
    TimedArtifactSet<Arc*> arcs_to_update;

};
