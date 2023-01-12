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

#include <array>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

// Generic formatting
template<typename D>
void format(std::ostream &/*stream*/) {}

template<int width, class T>
struct TableEntry {
    using datatype = T;
    int getWidth() {
        return width;
    }
};

template<bool use_width, class... T>
class DataTable {

public:
    static constexpr size_t num_columns = sizeof...(T);

    DataTable(const std::array<std::string, num_columns> &column_names,
              std::ostream &stream) :
        column_names(column_names), stream(stream) {}

    void addRow(const typename T::datatype... data) {
        table.emplace_back(data...);
    }

    void printHeader() {
        printEachHeaderElement();
        stream << std::endl;
    }

    void flush() {
        for(; current_row < table.size(); ++current_row) {
            printRow(current_row);
        }
    }

private:
    std::tuple<T...> typeinfo;
    std::vector<std::tuple<typename T::datatype...>> table;
    std::array<std::string, num_columns> column_names;

    // Output stream to which the table is written
    std::ostream &stream;

    size_t current_row = 0;

    template<size_t I>
    void formatWidth() {
        if constexpr (use_width) {
            stream << std::setw(std::get<I>(typeinfo).getWidth());
        }
    }

    template<size_t I = 0>
    typename std::enable_if<I == num_columns, void>::type
    printEachHeaderElement() {}

    template<size_t I = 0>
    typename std::enable_if<I < num_columns - 1, void>::type
    printEachHeaderElement() {
        formatWidth<I>();
        stream << column_names[I]
               << ',';  // Hardcoded separator
        printEachHeaderElement<I+1>();
    }

    // Last column header
    template<size_t I = 0>
    typename std::enable_if<I == num_columns - 1, void>::type
    printEachHeaderElement() {
        formatWidth<I>();
        stream << column_names[I];
        printEachHeaderElement<I+1>();
    }

    template<size_t I = 0>
    typename std::enable_if<I == num_columns, void>::type
    printEachRowElement(size_t /*row_index*/) {}

    template<size_t I = 0>
    typename std::enable_if<I < num_columns - 1, void>::type
    printEachRowElement(size_t row_index) {
        formatWidth<I>();
        format<typename std::tuple_element<I, std::tuple<T...>>::type::datatype>(stream);
        stream << std::get<I>(table[row_index])
               << ',';  // Hardcoded separator
        printEachRowElement<I+1>(row_index);
    }

    template<size_t I = 0>
    typename std::enable_if<I == num_columns - 1, void>::type
    printEachRowElement(size_t row_index) {
        formatWidth<I>();
        format<typename std::tuple_element<I, std::tuple<T...>>::type::datatype>(stream);
        stream << std::get<I>(table[row_index]);
        printEachRowElement<I+1>(row_index);
    }

    void printRow(size_t row_index) {
        printEachRowElement(row_index);
        stream << std::endl;
    }

};
