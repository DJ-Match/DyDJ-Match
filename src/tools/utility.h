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

#include <cstdio>
#include <functional>
#include <random>
#include <string>

// Mark `GraphArtifact`s of type `T` based on 'rounds'.
// This allows for some simple types of membership testing
// 
// May, for example, be used to process arcs once per Delta.
//
// Template parameter `T` should be a pointer to `GraphArtifact` or a derived type.
template<typename T>
class ArtifactMarker {

public:
    void mark(T arc) {
        marked_in_round[arc] = round;
    }

    void unmark(T arc) {
        marked_in_round[arc] = -1;
    }

    bool is_marked(T arc) {
        return marked_in_round[arc] == round;
    }

    void next_round() {
        round++;
    }

    void reset() {
        marked_in_round.setDefaultValue(-1);
        marked_in_round.resetAll();
    }

private:
    FastPropertyMap<int> marked_in_round{-1}; 
    int round = 0;

};

// Maintains a set of `GraphArtifact`s of type `T`,
// using `ArtifactMarker<T>` for membership testing.
// Allows insertion and clearing. Removing individual elements is not possible.
//
// This is a wrapper around a `std::vector<T>`.
// This vector can be accessed using `vector()`, which  returns a reference.
// Manipulating the vector directly is possible, but should be avoided, since it will screw up membership tests.
//
// Template parameter `T` should be a pointer to `GraphArtifact` or a derived type.
template<typename T, typename... Args>
class TimedArtifactSet {

public:
    // Add `t` to the set, if it's not contained.
    void add(T t) {
        if (!marker.is_marked(t)) {
            elements.push_back(t);
            marker.mark(t);
        }
    }

    // Get a reference to the underlying vector.
    std::vector<T, Args...>& vector() {
        return elements;
    }

    // Move to the next round.
    // This clears the set.
    void next_round() {
        elements.clear();
        marker.next_round();
    }

    // Clear the set and reset the markers.
    void reset() {
        elements.clear();
        marker.reset();
    }

private:
    ArtifactMarker<T> marker;
    std::vector<T, Args...> elements;

};

template<typename T>
inline std::string to_string_with_precision(T value, unsigned int decimal_places) {
    std::string result(16 + decimal_places, '\0');
    std::string fmt = "%." + std::to_string(decimal_places) + "f";
    auto written = std::snprintf(&result[0], result.size(), fmt.c_str(), value);
    result.resize(written);
    return result;
}

inline std::string config_string(std::initializer_list<std::pair<bool, std::string>> list,
                          const std::string &sep = "; ") {
    std::string result = "";
    auto it = list.begin();
    auto end = list.end();
    while (it != end && !it->first) {
        ++it;
    }
    if (it != end) {
        result += it->second;
        ++it;
        while (it != end) {
            if (it->first) {
                result += sep + it->second;
            }
            ++it;
        }
    }
    return result;
}

class random_number_generator {

public:
        inline random_number_generator() : engine(std::random_device{}()) {}
        inline random_number_generator(unsigned seed) : engine(seed) {}

        inline size_t next_index(size_t max) {
            return std::uniform_int_distribution<size_t>{0,max}(engine);
        }

        inline color_t next_color(color_t num_colors) {
            return std::uniform_int_distribution<color_t>{0,num_colors}(engine);
        }

        inline void set_seed(unsigned seed) {
                engine.seed(seed);
        }

private:
        std::mt19937_64 engine;

};
