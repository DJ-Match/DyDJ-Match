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

#include <iostream>
#include <memory>
#include <vector>

#include "algorithm/disjoint_matching_algorithm.h"
#include "algorithm/batch_invariant_greedy.h"
#include "algorithm/batch_iterative_greedy.h"
#include "algorithm/batch_node_centered.h"
#include "algorithm/dynamic_greedy.h"
#include "algorithm/greedy_kec_hybrid.h"
#include "algorithm/iterative_greedy.h"
#include "algorithm/k_edge_coloring.h"
#include "algorithm/node_centered.h"
#include "tools/analysis_algo.h"
#include "tools/edge_ranking_analysis_algo.h"

class ConfigReader {

public:
    inline ConfigReader(MatchingConfig &config,
                        std::istream &input,
                        std::vector<std::unique_ptr<AlgorithmBase>> &algos) : config(config),
                                                                              input(input),
                                                                              algos(algos) {}

private:
    static constexpr auto no_update_strategy_name = "none";
    static constexpr auto delay_update_strategy_name = "delay";
    static constexpr auto average_update_strategy_name = "average";
    static constexpr auto matched_fraction_update_strategy_name = "matched_fraction";

    MatchingConfig &config;
    std::istream &input;
    std::vector<std::unique_ptr<AlgorithmBase>> &algos;
    std::string update_strategy_name = "none";
    std::array<std::string, 2> update_strategy_params;

    // Reading a value of type T from an input stream
    // If no value can be read, that's a failure
    template<class T>
    bool read_one(T& t) {
        if (!(input >> t)) {
            std::cerr << "unexpected EOF" << std::endl;
            return false;
        }
        return true;
    }

    template<typename H, typename... T>
    bool read_many(H& h, T&... t) {
        return read_one(h) && read_many(t...);
    }

    template<typename H>
    bool read_many(H& h) {
        return read_one(h);
    }

    // Read algorithm parameters from an input stream
    bool make_greedy() {
        bool swaps;
        if (read_many(swaps)) {
            if (config.count_coloring_ops) {
                if (swaps) {
                    algos.emplace_back(new IterativeGreedy<true, true>());
                } else {
                    algos.emplace_back(new IterativeGreedy<false, true>());
                }
            } else {
                if (swaps) {
                    algos.emplace_back(new IterativeGreedy<true, false>());
                } else {
                    algos.emplace_back(new IterativeGreedy<false, false>());
                }
            }
            return true;
        } else {
            return false;
        }
    }
    bool make_greedy_b() {
        std::cerr << "greedy_b is not implemented" << std::endl;
        return false;
    }
    bool make_node_centered() {
        double thresh;
        int aggregate;
        if (read_many(aggregate, thresh)) {
            if (aggregate < 0 || aggregate > 4) {
                std::cerr << "aggregate parameter of node_centered should be in [0,4]!" << std::endl;
                return false;
            }
            switch (aggregate) {
                case AggregateType::AVG:
                    config.count_coloring_ops ?
                        algos.emplace_back(new NodeCentered<AggregateType::AVG, true>(thresh)) :
                        algos.emplace_back(new NodeCentered<AggregateType::AVG, false>(thresh));
                    break;
                case AggregateType::B_SUM:
                    config.count_coloring_ops ?
                        algos.emplace_back(new NodeCentered<AggregateType::B_SUM, true>(thresh)) :
                        algos.emplace_back(new NodeCentered<AggregateType::B_SUM, false>(thresh));
                    break;
                case AggregateType::MAX:
                    config.count_coloring_ops ?
                        algos.emplace_back(new NodeCentered<AggregateType::MAX, true>(thresh)) :
                        algos.emplace_back(new NodeCentered<AggregateType::MAX, false>(thresh));
                    break;
                case AggregateType::MEDIAN:
                    config.count_coloring_ops ?
                        algos.emplace_back(new NodeCentered<AggregateType::MEDIAN, true>(thresh)) :
                        algos.emplace_back(new NodeCentered<AggregateType::MEDIAN, false>(thresh));
                    break;
                case AggregateType::SUM:
                    config.count_coloring_ops ?
                        algos.emplace_back(new NodeCentered<AggregateType::SUM, true>(thresh)) :
                        algos.emplace_back(new NodeCentered<AggregateType::SUM, false>(thresh));
                    break;
            }
            return true;
        } else {
            return false;
        }
    }
    bool make_batch_node_centered() {
        double thresh;
        int aggregate;
        if (read_many(aggregate, thresh)) {
            if (aggregate < 0 || aggregate > 4) {
                std::cerr << "aggregate parameter of node_centered should be in [0,4]!" << std::endl;
                return false;
            }
            switch (aggregate) {
                case AggregateType::AVG:
                    config.count_coloring_ops ?
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::AVG, true>(thresh)) :
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::AVG, false>(thresh));
                    break;
                case AggregateType::B_SUM:
                    config.count_coloring_ops ?
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::B_SUM, true>(thresh)) :
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::B_SUM, false>(thresh));
                    break;
                case AggregateType::MAX:
                    config.count_coloring_ops ?
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::MAX, true>(thresh)) :
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::MAX, false>(thresh));
                    break;
                case AggregateType::MEDIAN:
                    config.count_coloring_ops ?
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::MEDIAN, true>(thresh)) :
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::MEDIAN, false>(thresh));
                    break;
                case AggregateType::SUM:
                    config.count_coloring_ops ?
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::SUM, true>(thresh)) :
                        algos.emplace_back(new BatchNodeCentered_2<AggregateType::SUM, false>(thresh));
                    break;
            }
            return true;
        } else {
            return false;
        }
    }
    bool make_gpa() {
        std::cerr << "GPA is not implemented" << std::endl;
        return false;
    }
    template<k_edge_coloring_algo_type type>
    void instantiate_k_edge_coloring(bool common_color,
                                     bool max_rotate,
                                     bool post_process,
                                     bool improved_pp,
                                     double filter_thresh = 2.0,
                                     double thresh = 1.0) {
        constexpr bool cc = true;
        constexpr bool mr = true;
        constexpr bool count_ops = true;
        constexpr bool imp_pp = true;
        if (common_color) {
            if (max_rotate) {
                if (config.count_coloring_ops) {
                    improved_pp ?
                        algos.emplace_back(new KEdgeColoring_2<type, cc, mr, count_ops, imp_pp>(post_process, thresh, filter_thresh)) :
                        algos.emplace_back(new KEdgeColoring_2<type, cc, mr, count_ops, !imp_pp>(post_process, thresh, filter_thresh));
                } else {
                    improved_pp ?
                        algos.emplace_back(new KEdgeColoring_2<type, cc, mr, !count_ops, imp_pp>(post_process, thresh, filter_thresh)) :
                        algos.emplace_back(new KEdgeColoring_2<type, cc, mr, !count_ops, !imp_pp>(post_process, thresh, filter_thresh));
                }
            } else {
                if (config.count_coloring_ops) {
                    improved_pp ?
                        algos.emplace_back(new KEdgeColoring_2<type, cc, !mr, count_ops, imp_pp>(post_process, thresh, filter_thresh)) :
                        algos.emplace_back(new KEdgeColoring_2<type, cc, !mr, count_ops, !imp_pp>(post_process, thresh, filter_thresh));
                } else {
                    improved_pp ?
                        algos.emplace_back(new KEdgeColoring_2<type, cc, !mr, !count_ops, imp_pp>(post_process, thresh, filter_thresh)) :
                        algos.emplace_back(new KEdgeColoring_2<type, cc, !mr, !count_ops, !imp_pp>(post_process, thresh, filter_thresh));
                }
            }
        } else {
            if (max_rotate) {
                if (config.count_coloring_ops) {
                    improved_pp ?
                        algos.emplace_back(new KEdgeColoring_2<type, !cc, mr, count_ops, imp_pp>(post_process, thresh, filter_thresh)) :
                        algos.emplace_back(new KEdgeColoring_2<type, !cc, mr, count_ops, !imp_pp>(post_process, thresh, filter_thresh));
                } else {
                    improved_pp ?
                        algos.emplace_back(new KEdgeColoring_2<type, !cc, mr, !count_ops, imp_pp>(post_process, thresh, filter_thresh)) :
                        algos.emplace_back(new KEdgeColoring_2<type, !cc, mr, !count_ops, !imp_pp>(post_process, thresh, filter_thresh));
                }
            } else {
                if (config.count_coloring_ops) {
                    improved_pp ?
                        algos.emplace_back(new KEdgeColoring_2<type, !cc, !mr, count_ops, imp_pp>(post_process, thresh, filter_thresh)) :
                        algos.emplace_back(new KEdgeColoring_2<type, !cc, !mr, count_ops, !imp_pp>(post_process, thresh, filter_thresh));
                } else {
                    improved_pp ?
                        algos.emplace_back(new KEdgeColoring_2<type, !cc, !mr, !count_ops, imp_pp>(post_process, thresh, filter_thresh)) :
                        algos.emplace_back(new KEdgeColoring_2<type, !cc, !mr, !count_ops, !imp_pp>(post_process, thresh, filter_thresh));
                }
            }
        }
    }
    bool make_k_edge_coloring() {
        bool common_color, max_rotate, post_process;
        if (read_many(common_color, max_rotate, post_process)) {
            instantiate_k_edge_coloring<k_edge_coloring_algo_type::STATIC>(common_color, max_rotate, post_process, false);
            return true;
        } else  {
            return false;
        }
    }
    bool make_dyn_k_edge_coloring() {
        bool common_color, max_rotate, post_process;
        char improved_processing;
        double filter_threshold;
        char mode;  // 'd' for dynamic, 'h' for hybrid
        if (read_many(common_color, max_rotate, post_process, improved_processing, filter_threshold, mode)) {
            bool imp_pp = false;
            if (improved_processing == '+') {
                imp_pp = true;
            } else if (improved_processing == '-') {
                imp_pp = false;
            } else {
                std::cout << "Unknow post-processing mode " << improved_processing << std::endl;
                return false;
            }
            if (mode == 'h') {
                double selector;
                if (!read_one(selector)) {
                    return false;
                }
                instantiate_k_edge_coloring<k_edge_coloring_algo_type::HYBRID>(common_color, max_rotate, post_process, imp_pp, filter_threshold, selector);
            } else if (mode == 'd') {
                instantiate_k_edge_coloring<k_edge_coloring_algo_type::DYNAMIC>(common_color, max_rotate, post_process, imp_pp, filter_threshold);
            }
            return true;
        } else {
            return false;
        }
    }
    bool make_dynamic_greedy() {
        int num_retries;
        bool post_process;
        double filter_threshold;
        char improved_processing;
        int random;
        if (read_many(num_retries, post_process, filter_threshold, improved_processing, random)) {
            if (improved_processing == '+') {
                if (random == 3) {
                    config.count_coloring_ops ?
                        algos.emplace_back(new DynamicGreedy<true, true, 3>(num_retries, post_process, filter_threshold)) :
                        algos.emplace_back(new DynamicGreedy<false, true, 3>(num_retries, post_process, filter_threshold));
                } else if (random == 2) {
                    config.count_coloring_ops ?
                        algos.emplace_back(new DynamicGreedy<true, true, 2>(num_retries, post_process, filter_threshold)) :
                        algos.emplace_back(new DynamicGreedy<false, true, 2>(num_retries, post_process, filter_threshold));
                } else if (random == 1) {
                    config.count_coloring_ops ?
                        algos.emplace_back(new DynamicGreedy<true, true, 1>(num_retries, post_process, filter_threshold)) :
                        algos.emplace_back(new DynamicGreedy<false, true, 1>(num_retries, post_process, filter_threshold));
                } else {
                    config.count_coloring_ops ?
                        algos.emplace_back(new DynamicGreedy<true, true, 0>(num_retries, post_process, filter_threshold)) :
                        algos.emplace_back(new DynamicGreedy<false, true, 0>(num_retries, post_process, filter_threshold));
                }
            } else if (improved_processing == '-') {
                if (random == 3) {
                    config.count_coloring_ops ?
                        algos.emplace_back(new DynamicGreedy<true, false, 3>(num_retries, post_process, filter_threshold)) :
                        algos.emplace_back(new DynamicGreedy<false, false, 3>(num_retries, post_process, filter_threshold));
                } else if (random == 2) {
                    config.count_coloring_ops ?
                        algos.emplace_back(new DynamicGreedy<true, false, 2>(num_retries, post_process, filter_threshold)) :
                        algos.emplace_back(new DynamicGreedy<false, false, 2>(num_retries, post_process, filter_threshold));
                } else if (random == 1) {
                    config.count_coloring_ops ?
                        algos.emplace_back(new DynamicGreedy<true, false, 1>(num_retries, post_process, filter_threshold)) :
                        algos.emplace_back(new DynamicGreedy<false, false, 1>(num_retries, post_process, filter_threshold));
                } else {
                    config.count_coloring_ops ?
                        algos.emplace_back(new DynamicGreedy<true, false, 0>(num_retries, post_process, filter_threshold)) :
                        algos.emplace_back(new DynamicGreedy<false, false, 0>(num_retries, post_process, filter_threshold));
                }
            } else {
                return false;
            }
            return true;
        } else {
            return false;
        }
    }
    bool make_greedy_kec_hybrid() {
        bool common_color, rotate_long;
        int num_retries;
        int random;
        bool post_process;
        char improved_processing;
        double hybrid_threshold;
        double filter_threshold;
        if (read_many(common_color, rotate_long, num_retries, random, post_process, improved_processing, hybrid_threshold, filter_threshold)) {
            constexpr bool cc = true;
            constexpr bool rl = true;
            constexpr bool imp_pp = true;
            constexpr bool count_ops = true;
            constexpr bool randomize = true;
            if (common_color) {
                if (rotate_long) {
                    if (improved_processing) {
                        if (random > 0) {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, rl,  count_ops,  imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, rl, !count_ops,  imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        } else {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, rl,  count_ops,  imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, rl, !count_ops,  imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        }
                    } else {
                        if (random > 0) {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, rl,  count_ops, !imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, rl, !count_ops, !imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        } else {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, rl,  count_ops, !imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, rl, !count_ops, !imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        }
                    }
                } else {
                    if (improved_processing) {
                        if (random > 0) {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, !rl,  count_ops,  imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, !rl, !count_ops,  imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        } else {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, !rl,  count_ops,  imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, !rl, !count_ops,  imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        }
                    } else {
                        if (random > 0) {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, !rl,  count_ops, !imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, !rl, !count_ops, !imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        } else {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, !rl,  count_ops, !imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<cc, !rl, !count_ops, !imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        }
                    }
                }
            } else {
                if (rotate_long) {
                    if (improved_processing) {
                        if (random > 0) {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, rl,  count_ops,  imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, rl, !count_ops,  imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        } else {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, rl,  count_ops,  imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, rl, !count_ops,  imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        }
                    } else {
                        if (random > 0) {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, rl,  count_ops, !imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, rl, !count_ops, !imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        } else {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, rl,  count_ops, !imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, rl, !count_ops, !imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        }
                    }
                } else {
                    if (improved_processing) {
                        if (random > 0) {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, !rl,  count_ops,  imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, !rl, !count_ops,  imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        } else {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, !rl,  count_ops,  imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, !rl, !count_ops,  imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        }
                    } else {
                        if (random > 0) {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, !rl,  count_ops, !imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, !rl, !count_ops, !imp_pp,  randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        } else {
                            config.count_coloring_ops ?
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, !rl,  count_ops, !imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold)) :
                                algos.emplace_back(new DynGreedyKEdgeColoringHybrid<!cc, !rl, !count_ops, !imp_pp, !randomize>(post_process, hybrid_threshold, num_retries, random, filter_threshold));
                        }
                    }
                }
            }
            return true;
        } else {
            return false;
        }
    }
    bool make_batch_greedy() {
        bool do_local_swaps; 
        if (read_one(do_local_swaps)) {
            if (do_local_swaps) {
                config.count_coloring_ops ?
                    algos.emplace_back(new BatchIterativeGreedy<true, true>()) :
                    algos.emplace_back(new BatchIterativeGreedy<true, false>());
            } else {
                config.count_coloring_ops ?
                    algos.emplace_back(new BatchIterativeGreedy<false, true>()) :
                    algos.emplace_back(new BatchIterativeGreedy<false, false>());
            }
            return true;
        } else {
            return false;
        }
    }
    bool make_invariant_greedy() {
        config.count_coloring_ops ?
            algos.emplace_back(new InvariantGreedy<true>()) :
            algos.emplace_back(new InvariantGreedy<false>());
        return true;
    }

    // Choose the algorithm construction based on the algorithm name
    bool readAlgo() {
        std::string algo_name;
        if (!read_one<std::string>(algo_name)) {
            return false;
        }
        bool success;
        if (algo_name == "greedy") {
            success = make_greedy();
        } else if (algo_name == "greedy_b") {
            success = make_greedy_b();
        } else if (algo_name == "node_centered") {
            success = make_node_centered();
        } else if (algo_name == "batch_node_centered") {
            success = make_batch_node_centered();
        } else if (algo_name == "gpa") {
            success = make_gpa();
        } else if (algo_name == "k_edge_coloring") {
            success = make_k_edge_coloring();
        }  else if (algo_name == "dyn_k_edge_coloring") {
            success = make_dyn_k_edge_coloring();
        }else if (algo_name == "dyn_greedy") {
            success = make_dynamic_greedy();
        } else if (algo_name == "greedy_kec_hybrid") {
            success = make_greedy_kec_hybrid();
        } else if (algo_name == "batch_greedy") {
            success = make_batch_greedy();
        } else if (algo_name == "invariant_greedy") {
            success = make_invariant_greedy();
        } else {
            std::cerr << "Invalid algorithm '" << algo_name << "'!" << std::endl;
            return false;
        }
        std::cout << "Algorithm " << algo_name << std::endl;
        return success;
    }

public:

    // Read a configuration file
    bool readConfig() {
        std::string config_str;
        bool success = true;
        while(input >> config_str) {
            if (config_str.front() == '#') {
                input.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
            } else if (config_str == "algo") {
                success = readAlgo();
            } else if (config_str == "b") {
                success = read_one<unsigned>(config.b);
                config.all_bs.push_back(config.b);
                std::cout << "b[" << config.all_bs.size() << "] = " << config.b << std::endl;
            } else if (config_str == "sanitycheck") {
                config.sanitycheck = true;
                std::cout << "Sanity check is enabled" << std::endl;
            } else if (config_str == "console_log") {
                config.console_log = true;
                std::cout << "Logging is enabled" << std::endl;
            } else if (config_str == "seed") {
                success = read_one<int>(config.seed);
                std::cout << "Random seed: " << config.seed << std::endl;
            } else if (config_str == "algorithm_order_seed") {
                success = read_one<unsigned>(config.algorithm_order_seed);
                std::cout << "Seed for random algorithm order: " << config.algorithm_order_seed << std::endl;
            } else if (config_str == "count_color_ops") {
                config.count_coloring_ops = true;
                success = true;
                std::cout << "Counting coloring operations is enabled." << std::endl;
            } else if (config_str == "analysis") {
                algos.emplace_back(new AnalysisAlgo());
                success = true;
                std::cout << "Analysis enabled." << std::endl;
            } else if (config_str == "ranking_analysis") {
                algos.emplace_back(new RankingAnalysisAlgo());
                success = true;
                std::cout << "Analysing edge rankings." << std::endl;
            } else {
                std::cout << "Unknown option " << config_str << std::endl;
                return false;
            }
            if (!success) {
                std::cout << "There is an error in the configuration" << std::endl;
                return false;
            }
        }
        return true;
    }

};
