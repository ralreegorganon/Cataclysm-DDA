#include "mapgen_caves.h"

#include "mapdata.h"
#include "omdata.h"
#include "overmap.h"
#include "mapgen.h"
#include "game_constants.h"
#include "line.h"
#include "rng.h"
#include "map.h"
#include "cata_utility.h"

#include "enums.h"

#include <limits>
#include <queue>
#include <vector>


struct node {
    int x;
    int y;
    int dir;
    int priority;

    node(int x, int y, int dir, int priority = 0) :
        x(x),
        y(y),
        dir(dir),
        priority(priority) {}

    bool operator< (const node &n) const {
        return priority > n.priority;
    }

    point pos() const {
        return point(x, y);
    }
};

struct path {
    std::vector<node> nodes;
};

template<class BinaryPredicate>
path find_path(const point &source, const point &dest, const int width, const int height, BinaryPredicate estimator)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1,  0 };

    const auto inbounds = [width, height](const int x, const int y) {
        return x >= 0 && x < width && y >= 0 && y < height;
    };

    const auto map_index = [width](const int x, const int y) {
        return y * width + x;
    };

    path res;

    if (source == dest) {
        return res;
    }

    const int x1 = source.x;
    const int y1 = source.y;
    const int x2 = dest.x;
    const int y2 = dest.y;

    if (!inbounds(x1, y1) || !inbounds(x2, y2)) {
        return res;
    }

    const node first_node(x1, y1, 5, 1000);
    const size_t map_size = width * height;

    std::vector<bool> closed(map_size, false);
    std::vector<int> open(map_size, 0);
    std::vector<short> dirs(map_size, 0);
    std::priority_queue<node, std::deque<node>> nodes[2];

    int i = 0;
    nodes[i].push(first_node);
    open[map_index(x1, y1)] = std::numeric_limits<int>::max();

    // use A* to find the shortest path from (x1,y1) to (x2,y2)
    while (!nodes[i].empty()) {
        const node mn(nodes[i].top()); // get the best-looking node

        nodes[i].pop();
        // mark it visited
        closed[map_index(mn.x, mn.y)] = true;

        // if we've reached the end, draw the path and return
        if (mn.x == x2 && mn.y == y2) {
            int x = mn.x;
            int y = mn.y;

            res.nodes.reserve(nodes[i].size());

            while (x != x1 || y != y1) {
                const int n = map_index(x, y);
                const int d = dirs[n];
                res.nodes.emplace_back(x, y, d);
                x += dx[d];
                y += dy[d];
            }

            res.nodes.emplace_back(x, y, -1);

            return res;
        }

        for (int d = 0; d < 4; d++) {
            const int x = mn.x + dx[d];
            const int y = mn.y + dy[d];
            const int n = map_index(x, y);
            // don't allow:
            // * out of bounds
            // * already traversed tiles
            if (!inbounds(x, y) || closed[n]) {
                continue;
            }

            node cn(x, y, d);
            cn.priority = estimator(cn, &mn);

            // record direction to shortest path
            if (open[n] == 0 || open[n] > cn.priority) {
                dirs[n] = (d + 2) % 4;

                if (open[n] != 0) {
                    while (nodes[i].top().x != x || nodes[i].top().y != y) {
                        nodes[1 - i].push(nodes[i].top());
                        nodes[i].pop();
                    }
                    nodes[i].pop();

                    if (nodes[i].size() > nodes[1 - i].size()) {
                        i = 1 - i;
                    }
                    while (!nodes[i].empty()) {
                        nodes[1 - i].push(nodes[i].top());
                        nodes[i].pop();
                    }
                    i = 1 - i;
                }
                open[n] = cn.priority;
                nodes[i].push(cn);
            }
        }
    }

    return res;
}

void mapgen_natural_cave_entrance(map *m, oter_id, mapgendata dat, const time_point &turn, float density)
{
    mapgen_forest(m, oter_str_id("forest_thick").id(), dat, turn, density);

    rough_circle(m, t_rock, SEEX, SEEY, 8);

    const int x = SEEX + 4;
    const int y = SEEX + 4;
    const int rad = 8;
    for (int i = x - rad; i <= x + rad; i++) {
        for (int j = y - rad; j <= y + rad; j++) {
            if (trig_dist(x, y, i, j) + rng(0, 3) <= rad && m->ter(i, j) == t_rock) {
                m->ter_set(i, j, t_dirt);
            }
        }
    }

    square(m, t_slope_down, SEEX - 1, SEEY - 1, SEEX - 1, SEEY - 1);
}

void mapgen_natural_cave(map *m, oter_id o, mapgendata dat, const time_point &turn, float density)
{
    constexpr int width = SEEX * 2;
    constexpr int height = SEEY * 2;

    bool should_connect_n = is_ot_subtype("natural_cave", dat.north());
    bool should_connect_s = is_ot_subtype("natural_cave", dat.south());
    bool should_connect_w = is_ot_subtype("natural_cave", dat.west());
    bool should_connect_e = is_ot_subtype("natural_cave", dat.east());

    std::vector<std::vector<int>> current;

    if (one_in(2)) {
        current = rise_automaton(width, height, 45, 5, 3, 12);
    }
    else {
        current = go_home_youre_drunk(width, height, 100);
    }

    std::vector<point> dapoints;
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if (current[i][j] == 1) {
                dapoints.emplace_back(point(i, j));
            }
        }
    }

    // Get the north and south most points.
    auto north_south_most = std::minmax_element(dapoints.begin(),
        dapoints.end(), [](const point & lhs, const point & rhs) {
        return lhs.y < rhs.y;
    });

    // Get the west and east most points.
    auto west_east_most = std::minmax_element(dapoints.begin(),
        dapoints.end(), [](const point & lhs, const point & rhs) {
        return lhs.x < rhs.x;
    });

    point northmost = *north_south_most.first;
    point southmost = *north_south_most.second;
    point westmost = *west_east_most.first;
    point eastmost = *west_east_most.second;

    const auto route_to = [&current](const point& src, const point& dest, const int &width, const int &height) {
        const auto estimate = [&](const node & cur, const node * prev) {
            const int dx = std::abs(cur.x - dest.x);
            const int dy = std::abs(cur.y - dest.y);
            const int d = 1;
            const int d2 = 1;
            const int dist = d * (dx + dy) + (d2 - 2 * d) * std::min(dx, dy) + (current[cur.x][cur.y] == 1 ? 1 : 5);
            return dist;
        };
        return find_path(src, dest, width, height, estimate);
    };

    if (should_connect_n && current[SEEX][0] == 0) {
        point src = northmost;
        point dest = point(SEEX, 0);
        path path = route_to(src, dest, width, height);
        for (const auto &node : path.nodes) {
            current[node.x][node.y] = 1;
        }
    }

    if (should_connect_s && current[SEEX][(SEEY * 2) - 1] == 0) {
        point src = southmost;
        point dest = point(SEEX, (SEEY * 2) - 1);
        path path = route_to(src, dest, width, height);
        for (const auto &node : path.nodes) {
            current[node.x][node.y] = 1;
        }
    }

    if (should_connect_w && current[0][SEEY] == 0) {
        point src = westmost;
        point dest = point(0, SEEY);
        path path = route_to(src, dest, width, height);
        for (const auto &node : path.nodes) {
            current[node.x][node.y] = 1;
        }
    }
    if (should_connect_e && current[(SEEX * 2) - 1][SEEY] == 0) {

        point src = eastmost;
        point dest = point((SEEX * 2) -1, SEEY);
        path path = route_to(src, dest, width, height);
        for (const auto &node : path.nodes) {
            current[node.x][node.y] = 1;
        }
    }

    const tripoint abs_sub = m->get_abs_sub();
    fill_background(m, t_rock);
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if (current[i][j] == 1) {
                const tripoint location(i, j, abs_sub.z);
                m->ter_set(location, t_rock_floor);
            }
        }
    }

    if (o == "natural_cave_vertical") {
        square(m, t_slope_down, SEEX - 1, SEEY - 1, SEEX - 1, SEEY - 1);
    }

    if (is_ot_subtype("natural_cave_entrance", dat.above()) || is_ot_subtype("natural_cave_vertical", dat.above())) {
        square(m, t_slope_up, SEEX, SEEY, SEEX, SEEY);
    }
}

void blooming_booming(int width, int height, std::vector<std::vector<int>> &current) {
    std::vector<point> candidates;

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if (current[i][j] == 1) {
                candidates.emplace_back(point(i, j));
            }
        }
    }

    std::random_shuffle(candidates.begin(), candidates.end());

    size_t iteration_number = candidates.size();
    for (size_t i = 0; i < iteration_number; i++) {

        unsigned random_offset = 0;
        if (one_in(3)) {
            random_offset = rng(candidates.size() - 5, candidates.size() - 1);
        }
        else {
            random_offset = rng(0, candidates.size() / 2);
        }

        if (random_offset >= candidates.size()) {
            random_offset = candidates.size() - 1;
        }

        int radius = 1;

        std::vector<point> targets = closest_points_first(radius, candidates[random_offset]);

        for (auto &t : targets) {
            int cx = clamp(t.x, 0, width - 1);
            int cy = clamp(t.y, 0, height - 1);
            if (current[cx][cy] == 0) {
                current[cx][cy] = 1;
                candidates.emplace_back(t);
            }
        }

        candidates.erase(candidates.begin() + random_offset);
    }
    candidates.clear();
};

std::vector<std::vector<int>> go_home_youre_drunk(int width, int height, int limit)
{
    std::unordered_set<point> walked;

    int steps = 0;
    int x = 0;
    int y = 0;
    int min_x = x;
    int max_x = x;
    int min_y = y;
    int max_y = y;

    do {
        point loc(x, y);
        if (walked.find(loc) == walked.end()) {
            walked.insert(loc);
            steps++;

            if (x < min_x) {
                min_x = x;
            }
            else if (x > max_x) {
                max_x = x;
            }

            if (y < min_y) {
                min_y = y;
            }
            else if (y > max_y) {
                max_y = y;
            }
        }
        int dir = rng(0, 3);
        switch (dir) {
        case 0:
            x += 1;
            break;
        case 1:
            x -= 1;
            break;
        case 2:
            y += 1;
            break;
        case 3:
            y -= 1;
            break;
        }
    } while (steps < limit);

    int shift_x = 0 - min_x;
    int shift_y = 0 - min_y;

    std::vector<std::vector<int>> current(width, std::vector<int>(height, 0));

    for (int x = min_x; x < min_x + width; x++) {
        for (int y = min_y; y < min_y + height; y++) {
            if (walked.find({ x,y }) != walked.end()) {
                current[x + shift_x][y + shift_y] = 1;
            }
        }
    }

    return current;
}

std::vector<std::vector<int>> rise_automaton(int width, int height, int initial_filled, int birth_thresh, int death_thresh, int iterations) {

    std::vector<std::vector<int>> current(width, std::vector<int>(height, 0));
    std::vector<std::vector<int>> next(width, std::vector<int>(height, 0));

    const auto neighbor_count = [&](const std::vector<std::vector<int>> &cells, const int x,
        const int y, const int radius) {
        int neighbors = 0;
        for (int ni = -radius; ni <= radius; ni++) {
            for (int nj = -radius; nj <= radius; nj++) {
                const int nx = x + ni;
                const int ny = y + nj;

                if (nx >= width || nx < 0 || ny >= height || ny < 0) {
                    // edges are alive
                    neighbors += 1;
                    continue;
                }

                neighbors += cells[nx][ny];
            }
        }
        neighbors -= cells[x][y];

        return neighbors;
    };

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            current[i][j] = x_in_y(initial_filled, 100);
        }
    }

    for (int iteration = 0; iteration < iterations; iteration++) {
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                const int neighbors = neighbor_count(current, i, j, 1);

                if (current[i][j] == 0 && neighbors >= birth_thresh) {
                    next[i][j] = 1;
                }
                else if (current[i][j] == 1 && neighbors >= death_thresh) {
                    next[i][j] = 1;
                }
                else {
                    next[i][j] = 0;
                }
            }
        }
        std::swap(current, next);
    }

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if (current[i][j] == 0) {
                current[i][j] = 1;
            }
            else {
                current[i][j] = 0;
            }
        }
    }

    return current;
}

