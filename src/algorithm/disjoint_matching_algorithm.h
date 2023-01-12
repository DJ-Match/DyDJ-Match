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

#include <limits>
#include <type_traits>

#include "algorithm/dynamicweighteddigraphalgorithm.h"

#include "datastructure/kcoloring.h"
#include "datastructure/kcoloring_extensions.h"
#include "algorithm/matching_defs.h"

// Configuration for matching algorithms
struct MatchingConfig {
    std::vector<int> all_bs;
    unsigned b{1};
    bool sanitycheck{false};
    std::string graph_filename;

    std::string outputFile = "";
    bool writeOutputfile{false};

    bool console_log{false};

    // Whether or not to count the color, uncolor and recolor operations per delta
    bool count_coloring_ops{false};

    int seed{123};
    unsigned algorithm_order_seed{0};
};

// Basuc update filtering
struct UpdateFilter {

public:
    // Create a new `UpdateFilter` which filters updates with relative change in `[1/t, t]`
    UpdateFilter(double t) : up_threshold(t), down_threshold(1/t) {}

    bool test(EdgeWeight oldValue, EdgeWeight newValue) const {
        double ratio = (double)newValue/(double)oldValue;
        return (newValue != 0 && oldValue != 0 &&
                ratio >= down_threshold &&
                ratio <= up_threshold);
    }

    double getUpThreshold() const {
        return up_threshold;
    }

private:
    const double up_threshold, down_threshold;
};

// Defines the interface for all coloring/disjoint matching algorithms.
class AlgorithmBase : public DynamicWeightedDiGraphAlgorithm<EdgeWeight> {

public:

    // Set the number of matchings (colors)
    virtual void set_num_matchings(unsigned int) = 0;

    virtual void init() = 0;

    // Returns the current solution weights
    virtual EdgeWeight deliver() = 0;

    // Can be called after `run` for sanity checks and similar purposes
    virtual void post_run() = 0;

    virtual const ColoringStatsExtension::color_op_counts& get_fine_counts() const = 0;
    virtual const ColoringStatsExtension::color_op_counts& get_coarse_counts() const = 0;

    virtual void configure(std::shared_ptr<const MatchingConfig> matching_config) = 0;

    // Function to allow algorithms to output additional information to `stream`.
    // This should be used purely for writing data to `stream`.
    virtual void custom_output(std::ostream &stream) const = 0;

};

template<bool measure_color_ops = false, typename... ColoringExt>
class DisjointMatchingAlgorithm : public AlgorithmBase {

private:
    using super = AlgorithmBase;

public:

    virtual void set_num_matchings(unsigned int b) override {
        coloring.setNumColors(b);
        coloring.reset();
    }

    virtual void init() {
        coloring.reset();
        if constexpr (measure_color_ops) {
            coloring.reset_arc_diffs();
        }
    }

    virtual void reset() {
        coloring.reset();
    }

    virtual EdgeWeight deliver() override final {
        return coloring.getTotalWeight();
    }

    virtual void post_run() final {
        if (matching_config->sanitycheck) {
            coloring.sanityCheck();
        }
        if constexpr (measure_color_ops) {
            coloring.compute_coarse_counts_and_reset();
            fine_counts = coloring.get_fine_counts();
            coarse_counts = coloring.get_coarse_counts();
            coloring.reset_fine_counts();
        }
    }

    virtual void onArcRemove(Arc* arc) override final {
        weights->setValue(arc, 0);
    }

    virtual const ColoringStatsExtension::color_op_counts& get_fine_counts() const final {
        return fine_counts;
    }

    virtual const ColoringStatsExtension::color_op_counts& get_coarse_counts() const final {
        return coarse_counts;
    }

    virtual void configure(std::shared_ptr<const MatchingConfig> matching_config) override final {
        this->matching_config = matching_config;
    }

    // The default implementation for `custom_output` is to do nothing.
    virtual void custom_output(std::ostream &/*stream*/) const override {}

protected:
    std::shared_ptr<const MatchingConfig> matching_config;

    // Enable the `ColoringStatsExtension` only if `measure_color_ops` is true.
    // Otherwise, we only enable extensions given by the template parameters.
    // We only have the overhead from counting coloring operations if it is explicitly requested.
    using coloring_type = std::conditional_t<measure_color_ops,
                                             KColoring<ColoringStatsExtension, ColoringExt...>,
                                             KColoring<ColoringExt...>>;

    coloring_type coloring{nullptr, nullptr, 1};

    // Counting statistics. These are not used if `measure_color_ops == false` and remain 0.
    // We still need to be able to report them (even though it's useless),
    // so we keep these around even if we don't count.
    ColoringStatsExtension::color_op_counts fine_counts, coarse_counts;

private:
    virtual void onDiGraphSet() override {
        super::onDiGraphSet();
        coloring.setGraph(diGraph);
        reset();
    }
    virtual void onWeightsSet() override {
        super::onWeightsSet();
        coloring.setWeights(weights);
        reset();
    }
    virtual void onDiGraphUnset() override {
        super::onDiGraphUnset();
        coloring.unsetGraph();
    }
    virtual void onWeightsUnset() override {
        super::onWeightsUnset();
        coloring.unsetWeights();
    }

};
