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
#include "../matching_algorithm.h"

#include <iostream>
#include <vector>

class MGMatching: public matching_algorithm {

    public:
        MGMatching(MatchingConfig & config)
            : matching_algorithm(config) { }

        virtual void run() override;

        virtual std::string getName() const noexcept override {
            return "MG Matching";
        }

        virtual std::string getShortName() const noexcept override {
            return "mg";
        }

        void sanityCheck();


    private:
        // helper vector whilst building fans
        std::vector<bool> free_color;

        // tmp arrays used throughout
        std::vector<unsigned> touched_free_color;
        std::vector<unsigned> touched_locally_free_color;
        std::vector<Algora::Vertex*> touched_path;

        std::vector<Algora::Arc*> fan;
        Algora::FastPropertyMap<char> fan_marked;

        Algora::FastPropertyMap<char> visited_path;

        void maximal_fan(Algora::Arc * arc);

        void shrink_fan(std::vector<Algora::Vertex*> cdpath, unsigned int c);

        unsigned int getFirstFreeColor(std::vector<bool> & colors) {
            auto i{0ul};
            while (i < colors.size() && !colors[i]) {
                i++;
            }
            return i;
        }

        void invertCdPath(unsigned int c, unsigned int d, Algora::Vertex * start);

        void rotateFan();

};
