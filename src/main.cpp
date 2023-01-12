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
#include <string>
#include <ostream>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <memory>
#include <ratio>
#include <iomanip>
#include <random>

#include "graph.dyn/dynamicweighteddigraph.h"
#include "io/konectnetworkreader.h"

#include "algoraapp_info.h"
#include "parse_configuration.h"
#include "parse_parameters.h"
#include "tools/chronotimer.h"
#include "tools/datatable.h"

// Format specification for doubles
template<>
void format<double>(std::ostream &stream) {
    stream << std::fixed << std::setprecision(6);
}

int main(int argc, char **argv) {
    // Parse command line arguments
    auto config = std::make_shared<MatchingConfig>();
    std::string graph_filename;
    bool do_return;

    auto code = parse_matching_parameters(argc, argv, *config, graph_filename, do_return);
    if (code > 0) {
        return code;
    } else if (do_return) {
        return 0;
    }

    // Read the configuration from standard input
    std::vector<std::unique_ptr<AlgorithmBase>> algos;
    if (!ConfigReader(*config, std::cin, algos).readConfig()) {
        std::cerr << "Error reading configuration from stdin" << std::endl;
        return 1;
    }
    for (auto &algo: algos) {
        algo->configure(config);
    }

    // Configure the output stream to which results are written
    // 'output_stream' either gets set up to write to a file or to std::cout
    std::streambuf *buffer;
    std::ofstream output_file_stream;
    if (config->writeOutputfile) {
        output_file_stream.open(config->outputFile);
        if (!output_file_stream.is_open()) {
            std::cerr << "Failed to open output file " << config->outputFile << std::endl;
            return 1;
        }
        buffer = output_file_stream.rdbuf();
    } else {
        buffer = std::cout.rdbuf();
    }
    std::ostream output_stream(buffer);

    // Print version information
    std::cout << "GIT_DATE: " << Algora::AlgoraAppInfo::GIT_DATE << "\n"
              << "GIT_REVISION: " << Algora::AlgoraAppInfo::GIT_REVISION << "\n"
              << "GIT_TIMESTAMP: " << Algora::AlgoraAppInfo::GIT_TIMESTAMP << "\n";

    // Prepare the graph file
    std::ifstream graph_file;
    graph_file.open(graph_filename);

    if (!graph_file.is_open()) {
        std::cout << "Error! Could not open file " << graph_filename << "\n";
        return 1;
    }

    // Start a timer, this is reused later
    ChronoTimer timer;

    // Graph IO
    Algora::DynamicWeightedDiGraph<EdgeWeight> G(0);
    Algora::KonectNetworkReader reader;
    reader.setInputStream(&graph_file);
    reader.removeNonPositiveWeightedArcs(true);
    reader.provideDynamicWeightedDiGraph(&G);
    graph_file.close();
    std::cout << "Input I/O took " << timer.elapsed() << "s\n";
    std::cout << "%n,m " << G.getConstructedGraphSize() << "," << G.getConstructedArcSize() << "\n";

    // Print command line arguments
    std::cout << "called with params: \n";
    for (auto i = 1; i < argc; i++) {
        std::cout << argv[i] << "\n";
    }

    // Setup defaults
    if (config->all_bs.empty()) {
        config->all_bs.push_back(1);
    }
    if (config->algorithm_order_seed != 0) {
        std::mt19937 rng;
        rng.seed(config->algorithm_order_seed);
        std::shuffle(algos.begin(), algos.end(), rng);
    }

    auto *diGraph = G.getDiGraph();
    auto *weights = G.getArcWeights();

    // Set up the table for managing/printing the results
    DataTable<false,
              TableEntry<3, int>,
              TableEntry<7, int>,
              TableEntry<25, std::string>,
              TableEntry<20, unsigned long>,
              TableEntry<12, double>,
              TableEntry<15, double>,
              TableEntry<15, double>,
              TableEntry<12, long>,
              TableEntry<14, long>,
              TableEntry<14, long>,
              TableEntry<12, long>,
              TableEntry<14, long>,
              TableEntry<14, long>,
              TableEntry<14, long>,
              TableEntry<14, long>> table({"b",
                                           "Delta",
                                           "Algorithm",
                                           "Weight",
                                           "Time (s)",
                                           "Delta-Time (s)",
                                           "Total Time (s)",
                                           "# color/up.",
                                           "# uncolor/up.",
                                           "# recolor/up.",
                                           "# color/D",
                                           "# uncolor/D",
                                           "# recolor/D",
                                           "# edges",
                                           "size of delta"},
                                          output_stream);

    table.printHeader();
    ChronoTimer deltaTimer;
    for (auto b: config->all_bs) {
        config->b = b;
        for (auto &algo: algos) {
            G.resetToBigBang();
            weights->resetAll();
            algo->setGraph(diGraph);
            algo->setWeights(weights);
            algo->set_num_matchings(b);
            algo->init();
            int delta_counter = 0;
            deltaTimer.restart();
            while(G.applyNextDelta()) {
                delta_counter++;
                auto deltaTime = deltaTimer.elapsed<>(); // Measure time of 'applyNextDelta()'
                timer.restart();
                algo->run();
                auto time = timer.elapsed<>();
                algo->post_run();

                table.addRow(b,
                            delta_counter,
                            algo->getName(),
                            algo->deliver(),
                            time,
                            deltaTime,
                            deltaTime + time,
                            algo->get_fine_counts().color_count,
                            algo->get_fine_counts().uncolor_count,
                            algo->get_fine_counts().recolor_count,
                            algo->get_coarse_counts().color_count,
                            algo->get_coarse_counts().uncolor_count,
                            algo->get_coarse_counts().recolor_count,
                            diGraph->getNumArcs(false),
                            G.getSizeOfLastDelta());
                table.flush();

                algo->custom_output(output_stream);

                deltaTimer.restart();
            }
            algo->unsetGraph();
            algo->unsetWeights();
        }
    }

    output_stream.flush();
    output_stream.rdbuf(nullptr);

    return 0;
}
