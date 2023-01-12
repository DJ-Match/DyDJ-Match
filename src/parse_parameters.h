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

#pragma once

#include <string>
#include <sstream>

#include "extern/argtable3-3.0.3/argtable3.h"

#include "algorithm/disjoint_matching_algorithm.h"
#include "algorithm/matching_defs.h"

// https://stackoverflow.com/a/24386991
std::string base_name(std::string const & path) {
  return path.substr(path.find_last_of("/\\") + 1);
}

int parse_matching_parameters(int argn, char **argv,  MatchingConfig &matching_config,
                          std::string &graph_filename, bool &ret) {

    const char *progname = argv[0];

    struct arg_lit *help = arg_lit0(NULL, "help", "Print help.");
    struct arg_int *seed     = arg_int0(NULL, "seed", NULL, "set seed for RNG");
    struct arg_int *oseed     = arg_int0(NULL, "oseed", NULL, "set seed for RNG used for shuffling the order of algorithms");

    struct arg_str *filename = arg_strn(NULL, NULL, "FILE", 1, 1, "Path to graph file to partition.");
    struct arg_str *outfile = arg_str0(NULL, "results-output", NULL, "Target file for result output");
    struct arg_end *end = arg_end(100);

    void *argtable[] = {
            help,
            filename,
            seed,
            oseed,
            outfile,
            end
    };

    // Parse arguments.
    int nerrors = arg_parse(argn, argv, argtable);
    ret = false;

    // Catch case that help was requested.
    if (help->count > 0) {
        printf("Usage: %s", progname);
        arg_print_syntax(stdout, argtable, " [< configfile]\n");
        arg_print_glossary(stdout, argtable,"  %-40s %s\n");
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        ret = true;
        return 0;
    }

    if (nerrors > 0) {
        arg_print_errors(stderr, end, progname);
        printf("Try '%s --help' for more information.\n",progname);
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 1;
    }


    if (filename->count > 0) {
        graph_filename = filename->sval[0];
        matching_config.graph_filename = base_name(graph_filename);
    }

    if (outfile->count > 0) {
        matching_config.outputFile = outfile->sval[0];
        matching_config.writeOutputfile = true;
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

    return 0;
}
