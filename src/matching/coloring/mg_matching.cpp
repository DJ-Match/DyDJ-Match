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

#include "mg_matching.h"


void MGMatching::run() {
    using namespace Algora;
    std::vector<bool> locally_free_color(delta, true);

    std::vector<Arc*> edges;
    edges.reserve(diGraph->getNumArcs(true));
	diGraph->mapArcs([this, &edges] (Arc *arc) {
		if (weights->getValue(arc) > 0) {
			edges.push_back(arc);
		}
	});

	std::sort(edges.begin(), edges.end(), [this](const Arc *lop, const Arc *rop) {
		return weights->getValue(lop) > weights->getValue(rop);
	});


    for (auto *arc : edges) {
        if (edge_color[arc] != UNCOLORED) {
            continue;
        }

        auto v = arc->getTail();
        auto free_edge_colors_am = [&] (Arc * arc) {
            assert(arc->isValid());
            if (edge_color[arc] != UNCOLORED) {
                locally_free_color[edge_color[arc]] = false;
                touched_locally_free_color.push_back(edge_color[arc]);
            }
        };
        diGraph->mapOutgoingArcs(v, free_edge_colors_am);
        diGraph->mapIncomingArcs(v, free_edge_colors_am);

        maximal_fan(arc);
        // todo limit size , try head
        auto c_color = getFirstFreeColor(locally_free_color);
        auto d_color = getFirstFreeColor(free_color);

        if (c_color >= num_matchings || d_color >= num_matchings) {
            continue;
        }

        if (!locally_free_color[d_color]){
            // invert the cd-path
            invertCdPath(d_color, c_color, v);
            // and c becomes locally not free
            locally_free_color[d_color] = true;
            locally_free_color[c_color] = false;
            touched_locally_free_color.push_back(c_color);

            // find w \in F such that d free on w, F'[i,w] is a fan
            shrink_fan(touched_path, c_color);

            for (auto el : touched_path) {
                visited_path[el] = false;
            }
            touched_path.clear();
        }

        auto rot_edge_id = fan.back();
        auto prev = edge_color[rot_edge_id];
        rotateFan();

        // set edge_color[e] = d
        if (prev != UNCOLORED) {
            assert(prev < delta);
            free_color[prev] = true;
        }
        edge_color[rot_edge_id] = d_color;
        // DEBUG_PRINT("setting color of " << n << " -> " << fan.back() << " to " << d_color <<"\n");
        locally_free_color[d_color] = false;

        // housekeeping
        for (auto el : touched_free_color) {
            free_color[el] = true;
        }

        for (auto el : fan) {
            fan_marked[el->getFirst()] = false;
            fan_marked[el->getSecond()] = false;
        }
        fan_marked[v] = false;

        fan.clear();
        touched_free_color.clear();

        touched_locally_free_color.push_back(d_color);
        // done with arc

        // housekeeping
        for (auto el : touched_locally_free_color) {
            locally_free_color[el] = true;
        }
        touched_locally_free_color.clear();
    }

    // write mates to the mate data structure
    // easier than keeping track of mates throughout execution
    max_color = 0;
    diGraph->mapArcs([&] (Arc * arc) {
        if (edge_color[arc] != UNCOLORED) {
            const auto s = arc->getFirst();
            const auto t = arc->getSecond();
            const auto color = edge_color[arc];
            assert(color < delta);
            mate[color][s] = t;
            mate[color][t] = s;
            if (color > max_color) {
                max_color = color;
            }
        }
    });
}

void MGMatching::sanityCheck() {
    matching_algorithm::sanityCheck();
    free_color.assign(delta, true);
    auto err_count{0ul};
    auto am = [&] (Algora::Vertex * v, Algora::Arc * arc) {
        auto color = edge_color[arc];
        if (color != UNCOLORED && !free_color[color]) {
            err_count++;
            std::cerr << "ERROR: " << color << " adjacent to " << v << " used multiple times\n";
        } /*else if(color >= delta) {
            std::cerr << "ERROR: unassigned color for edge " << arc->getHead() << "->" << arc->getTail() << "\n";
            err_count++;
        } */
        else {
            free_color[color] = false;
        }
    };

    auto am_inc = [&] (Algora::Arc * arc) {
        am(arc->getTail(), arc);
    };
    auto am_out = [&] (Algora::Arc * arc) {
        am(arc->getHead(), arc);
    };
    diGraph->mapVertices([&] (Algora::Vertex* v) {
        diGraph->mapIncomingArcs(v, am_inc);
        diGraph->mapOutgoingArcs(v, am_out);

        free_color.assign(delta, true);
    });
}

// assumption: always called with directed arc
// from head vertex of arc
void MGMatching::maximal_fan(Algora::Arc * arc) {
    using namespace Algora;
    const auto s = arc->getTail();
    const auto t = arc->getHead();
    fan.clear();

    // determine free colors of t
    auto free_edge_colors_am = [&] (Arc * arc) {
        if (edge_color[arc] != UNCOLORED) {
            free_color[edge_color[arc]] = false;
            touched_free_color.push_back(edge_color[arc]);
        }
    };
    diGraph->mapOutgoingArcs(t, free_edge_colors_am);
    diGraph->mapIncomingArcs(t, free_edge_colors_am);
    fan_marked[t] = true;
    fan.push_back(arc);

    // build maximal fan now
    // finds neighbor target of s, such that target
    // can be added to the fan
    auto fan_am = [&] (Arc * arc) {
        auto target = arc->getTail();
        if (target == s) {
            target = arc->getHead();
        }
        if (fan_marked[target]) {
            return;
        }

        // if edge is colored and its color is free on the previous fan vertex
        if (edge_color[arc] != UNCOLORED && free_color[edge_color[arc]]) {
            // reset the free_color vector
            for (auto el : touched_free_color) {
                free_color[el] = true;
            }
            touched_free_color.clear();
            // and set it to the colors of target
            diGraph->mapIncomingArcs(target, free_edge_colors_am);
            diGraph->mapOutgoingArcs(target, free_edge_colors_am);
            fan.push_back(arc);
            fan_marked[target] = true;
        }
    };

    bool addedEdge{true};
    while (addedEdge) {
        auto sizeBefore = fan.size();
        diGraph->mapOutgoingArcs(s, fan_am);
        diGraph->mapIncomingArcs(s, fan_am);

        // to have a maximal fan, need to continue until no 
        // arc can be added anymore
        addedEdge = sizeBefore < fan.size();
    }

}

void MGMatching::shrink_fan(std::vector<Algora::Vertex*> cdpath, unsigned int c) {
    // cdpath[0] => root node of fan
    // find v+ neighbor of root s.t. fan edge color WAS d, is now c
    auto vindex = 0;
    bool fanEdgeFound{false};

    for (auto i = 0ul; i < fan.size(); i++) {
        if (edge_color[fan[i]] == c) {
            fanEdgeFound = true;
            vindex = i - 1;
            break;
        }
    }
    // if no fan edge has color d
    // no shrinking necessary
    if (!fanEdgeFound) {
        return;
    }

    auto v = fan[vindex]->getTail();
    if (v == cdpath[0]) {
        v = fan[vindex]->getHead();
    }
    // check if v is in the cd-path
    bool isInPath{false};
    for (auto el : cdpath) {
        if (el == v) {
            isInPath = true;
            break;
        }
    }
    // if v is not in the cd path we need to shrink the fan down to <f..v>
    // ie. remove <v+..k>
    if (!isInPath) {
        auto it = fan.begin();
        std::advance(it, vindex + 1);
        // need to reset the marked fan vertices before deleting fan vertices
        for (auto cpy = it; cpy < fan.end(); cpy++) {
            auto t = (*cpy)->getTail();
            if (t == cdpath[0]) {
                t = (*cpy)->getHead();
            }
            fan_marked[t] = false;
        }
        fan.erase(it, fan.end());
    }
    // otherwise the fan can remain as it is
}

void MGMatching::invertCdPath(unsigned int c, unsigned int d, Algora::Vertex * start) {
    visited_path[start] = true;
    touched_path.push_back(start);
    auto am = [&] (Algora::Arc * arc) {
        auto target = arc->getTail();
        if (target == start) {
            target = arc->getHead();
        }
        // edge has color c and we haven't been to target yet, continue
        // cd path there
        if (edge_color[arc] == c && !visited_path[target]) {
            invertCdPath(d, c, target);
            edge_color[arc] = d;
        }
    };
    diGraph->mapOutgoingArcs(start, am);
    diGraph->mapIncomingArcs(start, am);
}

void MGMatching::rotateFan() {
    // assign each edge the color of the fan successor
    for (auto i = 0ul; i < fan.size()-1; i++) {
        edge_color[fan[i]] = edge_color[fan[i+1]];
    }
    // and uncolor the last edge of the fan
    edge_color[fan.back()] = UNCOLORED;
}


