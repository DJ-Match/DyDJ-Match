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

#ifndef CHRONOTIMER_H
#define CHRONOTIMER_H

#include <chrono>

class ChronoTimer {
        public:
                ChronoTimer() : start_time(std::chrono::steady_clock::now()) { }

                void restart() {
                    start_time = std::chrono::steady_clock::now();
                }

                template<typename time_unit = std::chrono::seconds>
                long long int elapsed_integral() {
                    static std::chrono::steady_clock::time_point stop_time;
                    stop_time = std::chrono::steady_clock::now();
                    return std::chrono::duration_cast<time_unit>(stop_time - start_time).count();
                }

                template<typename unit = std::ratio<1>>
                double elapsed() {
                    static std::chrono::steady_clock::time_point stop_time;
                    stop_time = std::chrono::steady_clock::now();
                    std::chrono::duration<double, unit> delta = stop_time - start_time;
                    return delta.count();
                }

        private:
                std::chrono::steady_clock::time_point start_time;
};

#endif /* CHRONOTIMER_H */
