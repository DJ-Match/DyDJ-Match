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

#include "algorithm/matching_defs.h"

struct color_set {
    using size_type = unsigned int;
    static constexpr size_type npos = -1;

private:
    using bit_type = unsigned long;
    static constexpr bit_type one = 1u;

    bit_type bits;
    bit_type all_bits;
    size_type bit_size;

    inline color_set(bit_type bits, bit_type all, size_type size) : bits(bits),
                                                                    all_bits(all),
                                                                    bit_size(size) {}

    friend color_set operator&(const color_set &a, const color_set &b);

public:
    color_set() : bits(0), all_bits(0), bit_size(0) {}

    // Create a new `color_set`. By default, all colors are included.
    color_set(size_type size) : all_bits((one << size) - 1), // 2^bit_size - 1 -> bit_size many bits are set to 1
                                bit_size(size) {
        bits = all_bits;
    }

    inline size_type find_first() const {
        if (bits == 0) {
            return npos;
        } else {
            return lowestBit(bits);
        }
    }

    inline size_type find_next(size_type pos) const {
        auto shifted = bits >> pos;
        if (shifted == 0) {
            return npos;
        } else {
            return lowestBit(shifted) + pos;
        }
    }

    inline void flip() {
        // Flip all bits, then set unused bits to 0
        bits = (~bits) & all_bits;
    }

    inline void set() {
        bits = all_bits;
    }

    inline void setOn(size_type i) {
        bits = bits | (one << i);
    }

    inline void setOff(size_type i) {
        bits = bits ^ (one << i);
    }

    inline bool none() const {
        return bits == 0;
    }

    inline bool any() const {
        return !none();
    }

    inline bool all() const {
        return bits == all_bits;
    }

    inline size_type count() const {
        size_type count = 0;
        for (size_type i = 0; i < bit_size; ++i) {
            count += ((bits & (one << i)) != 0);
        }
        return count;
    }

    inline size_type size() const {
        return bit_size;
    }

    static inline color_set common_colors(const color_set &a, const color_set &b) {
        return (a & b);
    }

    inline bool operator[](size_type i) const {
        return bits & (one << i);
    }

    friend std::ostream& operator<<(std::ostream &stream, const color_set &c);

private:

    static size_type lowestBit(bit_type b) {
        return __builtin_ctzl(b);
    }

};

inline color_set operator&(const color_set &a, const color_set &b) {
    assert(a.size() == b.size());
    return {a.bits & b.bits, a.all_bits, a.bit_size};
}

inline std::ostream& operator<<(std::ostream &stream, const color_set &c) {
    for (color_set::size_type i = c.bit_size; i > 0; --i) {
        stream << (int)(c[i-1]);
    }
    return stream;
}
