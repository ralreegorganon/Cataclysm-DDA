#include "mapgen_caves.h"

#include "mapdata.h"
#include "omdata.h"
#include "overmap.h"
#include "overmap_location.h"

#include "mapgen.h"
#include "game_constants.h"
#include "line.h"
#include "rng.h"
#include "map.h"
#include "cata_utility.h"

#include "enums.h"

#include <chrono>
#include <limits>
#include <queue>
#include <vector>
#include <unordered_set>
#include <random>

#include "simple_pathfinding.h"

void mapgen_natural_cave_entrance( map *m, oter_id, mapgendata dat, const time_point &turn,
                                   float density )
{
    mapgen_forest( m, oter_str_id( "forest_thick" ).id(), dat, turn, density );

    rough_circle( m, t_rock, SEEX, SEEY, 8 );

    const int x = SEEX + 4;
    const int y = SEEX + 4;
    const int rad = 8;
    for( int i = x - rad; i <= x + rad; i++ ) {
        for( int j = y - rad; j <= y + rad; j++ ) {
            if( trig_dist( x, y, i, j ) + rng( 0, 3 ) <= rad && m->ter( i, j ) == t_rock ) {
                m->ter_set( i, j, t_dirt );
            }
        }
    }

    square( m, t_slope_down, SEEX - 1, SEEY - 1, SEEX - 1, SEEY - 1 );
}

void mapgen_natural_cave( map *m, oter_id o, mapgendata dat, const time_point &, float )
{
    constexpr int width = SEEX * 2;
    constexpr int height = SEEY * 2;

    bool should_connect_n = is_ot_match("natural_cave", dat.north(), ot_match_type::prefix);
    bool should_connect_s = is_ot_match( "natural_cave", dat.south(), ot_match_type::prefix);
    bool should_connect_w = is_ot_match( "natural_cave", dat.west(), ot_match_type::prefix);
    bool should_connect_e = is_ot_match( "natural_cave", dat.east(), ot_match_type::prefix);

    std::vector<std::vector<int>> current;

    // generate the rough layout, everything is either solid rock (0) or empty (1)

    if( one_in( 2 ) ) {
        current = rise_automaton( width, height, 45, 5, 3, 12, 1, 1 );
    } else {
        current = go_home_youre_drunk( width, height, 100 );
    }

    std::vector<point> dapoints;
    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            if( current[i][j] == 1 ) {
                dapoints.emplace_back( point( i, j ) );
            }
        }
    }

    // Get the north and south most points.
    auto north_south_most = std::minmax_element( dapoints.begin(),
    dapoints.end(), []( const point & lhs, const point & rhs ) {
        return lhs.y < rhs.y;
    } );

    // Get the west and east most points.
    auto west_east_most = std::minmax_element( dapoints.begin(),
    dapoints.end(), []( const point & lhs, const point & rhs ) {
        return lhs.x < rhs.x;
    } );

    point northmost = *north_south_most.first;
    point southmost = *north_south_most.second;
    point westmost = *west_east_most.first;
    point eastmost = *west_east_most.second;

    const auto route_to = [&current]( const point & src, const point & dest, const int &width,
    const int &height ) {
        const auto estimate = [&]( const pf::node & cur, const pf::node * ) {
            const int dx = std::abs( cur.x - dest.x );
            const int dy = std::abs( cur.y - dest.y );
            const int d = 1;
            const int d2 = 1;
            const int terrain_factor = current[cur.x][cur.y] == 1 ? 1 : rng( 5, 10 );
            const int dist = d * ( dx + dy ) + ( d2 - 2 * d ) * std::min( dx, dy ) + terrain_factor;
            return dist;
        };
        return pf::find_path( src, dest, width, height, estimate );
    };

    // connect up this location with adjacent caves

    if( should_connect_n && current[SEEX][0] == 0 ) {
        point src = northmost;
        point dest = point( SEEX, 0 );
        pf::path path = route_to( src, dest, width, height );
        for( const auto &node : path.nodes ) {
            current[node.x][node.y] = 1;
        }
    }

    if( should_connect_s && current[SEEX][( SEEY * 2 ) - 1] == 0 ) {
        point src = southmost;
        point dest = point( SEEX, ( SEEY * 2 ) - 1 );
        pf::path path = route_to( src, dest, width, height );
        for( const auto &node : path.nodes ) {
            current[node.x][node.y] = 1;
        }
    }

    if( should_connect_w && current[0][SEEY] == 0 ) {
        point src = westmost;
        point dest = point( 0, SEEY );
        pf::path path = route_to( src, dest, width, height );
        for( const auto &node : path.nodes ) {
            current[node.x][node.y] = 1;
        }
    }
    if( should_connect_e && current[( SEEX * 2 ) - 1][SEEY] == 0 ) {

        point src = eastmost;
        point dest = point( ( SEEX * 2 ) - 1, SEEY );
        pf::path path = route_to( src, dest, width, height );
        for( const auto &node : path.nodes ) {
            current[node.x][node.y] = 1;
        }
    }

    // carve out the path to our up and down locations
    const tripoint abs_sub = m->get_abs_sub();
    const tripoint slope_down_location( SEEX - 1, SEEY - 1, abs_sub.z );
    const tripoint slope_up_location( SEEX, SEEY, abs_sub.z );

    if( o == "natural_cave_descent" ) {
        if( current[slope_down_location.x][slope_down_location.y] == 0 ) {
            std::vector<point> targets = closest_points_first( width, slope_down_location.x,
                                         slope_down_location.y );
            for( auto &t : targets ) {
                if( t.x < 0 || t.x >= width || t.y < 0 || t.y >= height ) {
                    continue;
                }

                if( current[t.x][t.y] == 1 ) {
                    point src = point( t.x, t.y );
                    point dest = point( slope_down_location.x, slope_down_location.y );
                    pf::path path = route_to( src, dest, width, height );
                    for( const auto &node : path.nodes ) {
                        current[node.x][node.y] = 1;
                    }
                    break;
                }
            }
        }
    }

    if( is_ot_match( "natural_cave_entrance", dat.above(), ot_match_type::type) ||
        is_ot_match( "natural_cave_descent", dat.above(), ot_match_type::type) ) {
        if( current[slope_up_location.x][slope_up_location.y] == 0 ) {
            std::vector<point> targets = closest_points_first( width, slope_up_location.x,
                                         slope_up_location.y );
            for( auto &t : targets ) {
                if( t.x < 0 || t.x >= width || t.y < 0 || t.y >= height ) {
                    continue;
                }

                if( current[t.x][t.y] == 1 ) {
                    point src = point( t.x, t.y );
                    point dest = point( slope_up_location.x, slope_up_location.y );
                    pf::path path = route_to( src, dest, width, height );
                    for( const auto &node : path.nodes ) {
                        current[node.x][node.y] = 1;
                    }
                    break;
                }
            }
        }
    }



    // make some rivers
    std::vector<std::vector<int>> current_river( width, std::vector<int>( height, 0 ) );
    current_river[rng( 0, width - 1 )][rng( 0, height - 1 )] = 1;

    if( is_ot_match( "natural_cave_river", o, ot_match_type::type) ) {
        // generate the rough layout, everything is either solid rock (0) or empty (1)
        //current_river = rise_automaton(width, height, 40, 5, 2, 16, 1, 2);

        bool should_connect_n_river = is_ot_match( "natural_cave_river", dat.north(), ot_match_type::type);
        bool should_connect_s_river = is_ot_match( "natural_cave_river", dat.south(), ot_match_type::type);
        bool should_connect_w_river = is_ot_match( "natural_cave_river", dat.west(), ot_match_type::type);
        bool should_connect_e_river = is_ot_match( "natural_cave_river", dat.east(), ot_match_type::type);

        std::vector<point> dapoints_river;
        for( int i = 0; i < width; i++ ) {
            for( int j = 0; j < height; j++ ) {
                if( current_river[i][j] == 1 ) {
                    dapoints_river.emplace_back( point( i, j ) );
                }
            }
        }

        // Get the north and south most points.
        auto north_south_most_river = std::minmax_element( dapoints_river.begin(),
        dapoints_river.end(), []( const point & lhs, const point & rhs ) {
            return lhs.y < rhs.y;
        } );

        // Get the west and east most points.
        auto west_east_most_river = std::minmax_element( dapoints_river.begin(),
        dapoints_river.end(), []( const point & lhs, const point & rhs ) {
            return lhs.x < rhs.x;
        } );

        point northmost_river = *north_south_most_river.first;
        point southmost_river = *north_south_most_river.second;
        point westmost_river =  *west_east_most_river.first;
        point eastmost_river =  *west_east_most_river.second;

        constexpr int river_radius_min = 1;
        constexpr int river_radius_max = 2;

        const auto route_to_river = [&current_river]( const point & src, const point & dest,
                                    const int &width,
        const int &height ) {
            const auto estimate = [&]( const pf::node & cur, const pf::node * ) {
                const int dx = std::abs( cur.x - dest.x );
                const int dy = std::abs( cur.y - dest.y );
                const int d = 1;
                const int d2 = 1;
                const int terrain_factor = current_river[cur.x][cur.y] == 1 ? 1 : rng( 2, 4 );
                const int dist = d * ( dx + dy ) + ( d2 - 2 * d ) * std::min( dx, dy ) + terrain_factor;
                return dist;
            };
            return pf::find_path( src, dest, width, height, estimate );
        };

        // connect up this location with adjacent rivers

        if( should_connect_n_river && current_river[SEEX][0] == 0 ) {
            point src = northmost_river;
            point dest = point( SEEX, 0 );
            pf::path path = route_to_river( src, dest, width, height );
            for( const auto &node : path.nodes ) {
                std::vector<point> targets = closest_points_first( rng( river_radius_min, river_radius_max ),
                                             node.x, node.y );
                for( auto &t : targets ) {
                    if( t.x < 0 || t.x >= width || t.y < 0 || t.y >= height ) {
                        continue;
                    }
                    current_river[t.x][t.y] = 1;
                }
            }
        }

        if( should_connect_s_river && current_river[SEEX][( SEEY * 2 ) - 1] == 0 ) {
            point src = southmost_river;
            point dest = point( SEEX, ( SEEY * 2 ) - 1 );
            pf::path path = route_to_river( src, dest, width, height );
            for( const auto &node : path.nodes ) {
                std::vector<point> targets = closest_points_first( rng( river_radius_min, river_radius_max ),
                                             node.x, node.y );
                for( auto &t : targets ) {
                    if( t.x < 0 || t.x >= width || t.y < 0 || t.y >= height ) {
                        continue;
                    }
                    current_river[t.x][t.y] = 1;
                }
            }
        }

        if( should_connect_w_river && current_river[0][SEEY] == 0 ) {
            point src = westmost_river;
            point dest = point( 0, SEEY );
            pf::path path = route_to_river( src, dest, width, height );
            for( const auto &node : path.nodes ) {
                std::vector<point> targets = closest_points_first( rng( river_radius_min, river_radius_max ),
                                             node.x, node.y );
                for( auto &t : targets ) {
                    if( t.x < 0 || t.x >= width || t.y < 0 || t.y >= height ) {
                        continue;
                    }
                    current_river[t.x][t.y] = 1;
                }
            }
        }
        if( should_connect_e_river && current_river[( SEEX * 2 ) - 1][SEEY] == 0 ) {
            point src = eastmost_river;
            point dest = point( ( SEEX * 2 ) - 1, SEEY );
            pf::path path = route_to_river( src, dest, width, height );
            for( const auto &node : path.nodes ) {
                std::vector<point> targets = closest_points_first( rng( river_radius_min, river_radius_max ),
                                             node.x, node.y );
                for( auto &t : targets ) {
                    if( t.x < 0 || t.x >= width || t.y < 0 || t.y >= height ) {
                        continue;
                    }
                    current_river[t.x][t.y] = 1;
                }
            }
        }
    }




    // fill in the rocks/floor

    fill_background( m, t_rock );
    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            if( current[i][j] == 1 ) {
                const tripoint location( i, j, abs_sub.z );
                m->ter_set( location, *dat.region.natural_cave.natural_cave_terrain.pick() );
                // m->ter_set(location, t_rock_floor);
            }
        }
    }

    if(is_ot_match( "natural_cave_river", o, ot_match_type::type) ) {
        // fill in the river
        for( int i = 0; i < width; i++ ) {
            for( int j = 0; j < height; j++ ) {
                if( current_river[i][j] == 1 ) {
                    const tripoint location( i, j, abs_sub.z );
                    m->ter_set( location, t_water_dp_cave );
                }
            }
        }
    }

    // if i am ascent, i need stairs up
    // if i am descent, i need stairs down
    // if above me is descent, i need stairs up
    // fill in the slopes up/down
    if( o == "natural_cave_descent" ) {
        m->ter_set( slope_down_location, t_slope_down );
    }

    if(is_ot_match( "natural_cave_entrance", dat.above(), ot_match_type::type) ||
        is_ot_match( "natural_cave_descent", dat.above(), ot_match_type::type) ) {
        m->ter_set( slope_up_location, t_slope_up );
    }
}

void blooming_booming( int width, int height, std::vector<std::vector<int>> &current )
{
    std::vector<point> candidates;

    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            if( current[i][j] == 1 ) {
                candidates.emplace_back( point( i, j ) );
            }
        }
    }
    static std::default_random_engine eng(
        std::chrono::system_clock::now().time_since_epoch().count() );
    std::shuffle( candidates.begin(), candidates.end(), eng );

    size_t iteration_number = candidates.size();
    for( size_t i = 0; i < iteration_number; i++ ) {

        unsigned random_offset = 0;
        if( one_in( 3 ) ) {
            random_offset = rng( candidates.size() - 5, candidates.size() - 1 );
        } else {
            random_offset = rng( 0, candidates.size() / 2 );
        }

        if( random_offset >= candidates.size() ) {
            random_offset = candidates.size() - 1;
        }

        int radius = 1;

        std::vector<point> targets = closest_points_first( radius, candidates[random_offset] );

        for( auto &t : targets ) {
            int cx = clamp( t.x, 0, width - 1 );
            int cy = clamp( t.y, 0, height - 1 );
            if( current[cx][cy] == 0 ) {
                current[cx][cy] = 1;
                candidates.emplace_back( t );
            }
        }

        candidates.erase( candidates.begin() + random_offset );
    }
    candidates.clear();
};

std::vector<std::vector<int>> go_home_youre_drunk( int width, int height, int limit )
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
        point loc( x, y );
        if( walked.find( loc ) == walked.end() ) {
            walked.insert( loc );
            steps++;

            if( x < min_x ) {
                min_x = x;
            } else if( x > max_x ) {
                max_x = x;
            }

            if( y < min_y ) {
                min_y = y;
            } else if( y > max_y ) {
                max_y = y;
            }
        }
        int dir = rng( 0, 3 );
        switch( dir ) {
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
    } while( steps < limit );

    int shift_x = 0 - min_x;
    int shift_y = 0 - min_y;

    std::vector<std::vector<int>> current( width, std::vector<int>( height, 0 ) );

    for( int x = min_x; x < min_x + width; x++ ) {
        for( int y = min_y; y < min_y + height; y++ ) {
            if( walked.find( { x, y } ) != walked.end() ) {
                current[x + shift_x][y + shift_y] = 1;
            }
        }
    }

    return current;
}

std::vector<std::vector<int>> rise_automaton( int width, int height, int initial_filled,
                           int birth_thresh, int death_thresh, int iterations, int connection_radius_min,
                           int connection_radius_max )
{

    std::vector<std::vector<int>> current( width, std::vector<int>( height, 0 ) );
    std::vector<std::vector<int>> next( width, std::vector<int>( height, 0 ) );

    const auto neighbor_count = [&]( const std::vector<std::vector<int>> &cells, const int x,
    const int y, const int radius ) {
        int neighbors = 0;
        for( int ni = -radius; ni <= radius; ni++ ) {
            for( int nj = -radius; nj <= radius; nj++ ) {
                const int nx = x + ni;
                const int ny = y + nj;

                if( nx >= width || nx < 0 || ny >= height || ny < 0 ) {
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

    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            current[i][j] = x_in_y( initial_filled, 100 );
        }
    }

    for( int iteration = 0; iteration < iterations; iteration++ ) {
        for( int i = 0; i < width; i++ ) {
            for( int j = 0; j < height; j++ ) {
                const int neighbors = neighbor_count( current, i, j, 1 );

                if( current[i][j] == 0 && neighbors >= birth_thresh ) {
                    next[i][j] = 1;
                } else if( current[i][j] == 1 && neighbors >= death_thresh ) {
                    next[i][j] = 1;
                } else {
                    next[i][j] = 0;
                }
            }
        }
        std::swap( current, next );
    }

    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            if( current[i][j] == 0 ) {
                current[i][j] = 1;
            } else {
                current[i][j] = 0;
            }
        }
    }


    std::unordered_set<point> visited;

    const auto get_cave = [&]( point starting_point, std::vector<point> &cave_points ) {
        std::queue<point> to_check;
        to_check.push( starting_point );
        while( !to_check.empty() ) {
            const point current_point = to_check.front();
            to_check.pop();

            // We've been here before, so bail.
            if( visited.find( current_point ) != visited.end() ) {
                continue;
            }

            // This point is out of bounds, so bail.
            bool in_bounds = current_point.x >= 0 && current_point.x < width && current_point.y >= 0 &&
                             current_point.y < height;
            if( !in_bounds ) {
                continue;
            }

            // Mark this point as visited.
            visited.emplace( current_point );

            if( current[current_point.x][current_point.y] == 1 ) {
                cave_points.emplace_back( current_point );
                to_check.push( point( current_point.x, current_point.y + 1 ) );
                to_check.push( point( current_point.x, current_point.y - 1 ) );
                to_check.push( point( current_point.x + 1, current_point.y ) );
                to_check.push( point( current_point.x - 1, current_point.y ) );
            }
        }
        return;
    };

    std::vector<std::vector<point>> caves;

    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            if( current[i][j] == 0 ) {
                continue;
            }

            point seed_point( i, j );

            if( visited.find( seed_point ) != visited.end() ) {
                continue;
            }

            std::vector<point> cave_points;
            get_cave( seed_point, cave_points );
            caves.emplace_back( cave_points );
        }
    }

    if( caves.size() > 1 ) {
        const auto route_to = [&current]( const point & src, const point & dest, const int &width,
        const int &height ) {
            const auto estimate = [&]( const pf::node & cur, const pf::node * ) {
                const int dx = std::abs( cur.x - dest.x );
                const int dy = std::abs( cur.y - dest.y );
                const int d = 1;
                const int d2 = 1;
                const int terrain_factor = current[cur.x][cur.y] == 1 ? 1 : rng( 5, 10 );
                const int dist = d * ( dx + dy ) + ( d2 - 2 * d ) * std::min( dx, dy ) + terrain_factor;
                return dist;
            };
            return pf::find_path( src, dest, width, height, estimate );
        };

        static std::default_random_engine eng(
            std::chrono::system_clock::now().time_since_epoch().count() );
        std::shuffle( caves.begin(), caves.end(), eng );

        std::vector<point> to_connect;

        for( auto &c : caves ) {
            const int pick = rng( 0, c.size() );
            auto it = c.begin();
            std::advance( it, pick );
            to_connect.emplace_back( *it );
        }

        for( auto src = to_connect.begin(), dest = ++to_connect.begin(); dest != to_connect.end();
             src++, dest++ ) {
            pf::path path = route_to( *src, *dest, width, height );
            for( const auto &node : path.nodes ) {
                std::vector<point> targets = closest_points_first( rng( connection_radius_min,
                                             connection_radius_max ), node.x, node.y );
                for( auto &t : targets ) {
                    if( t.x < 0 || t.x >= width || t.y < 0 || t.y >= height ) {
                        continue;
                    }
                    current[t.x][t.y] = 1;
                }
            }
        }
    }

    return current;
}

void overmap::build_natural_cave_network()
{
    const string_id<overmap_location>
    potential_cave_network_surface_entrances( "potential_cave_network_surface_entrances" );
    const oter_id empty_rock( "empty_rock" );
    const oter_id natural_cave_entrance( "natural_cave_entrance" );
    const oter_id natural_cave_unremarkable( "natural_cave_unremarkable" );
    const oter_id natural_cave_river( "natural_cave_river" );
    const oter_id natural_cave_descent( "natural_cave_descent" );

    std::vector<tripoint> surface_candidiates;
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            const tripoint l( i, j, 0 );
            const tripoint lsub1( i, j, -1 );
            const tripoint lsub2( i, j, -2 );
            if( potential_cave_network_surface_entrances->test( ter( l ) ) && ter( lsub1 ) == empty_rock &&
                ter( lsub2 ) == empty_rock ) {
                surface_candidiates.emplace_back( l );
            }
        }
    }

    std::vector<tripoint> surface_locations;
    int max_random_surface_locations = settings.natural_cave.surface_loctions;
    int random_surface_location_count = 0;
    static std::default_random_engine eng(
        std::chrono::system_clock::now().time_since_epoch().count() );
    std::shuffle( surface_candidiates.begin(), surface_candidiates.end(), eng );
    for( auto &random_point : surface_candidiates ) {
        if( random_surface_location_count >= max_random_surface_locations ) {
            break;
        }
        random_surface_location_count++;
        surface_locations.emplace_back( random_point );
    }


    const auto cave_route_to = [&]( const tripoint & src, const tripoint & dest, const int &width,
    const int &height, const bool rock_only ) {
        const int z = src.z;
        const auto estimate = [&]( const pf::node & cur, const pf::node * ) {
            static const oter_str_id empty_rock( "empty_rock" );
            static const oter_id natural_cave_unremarkable( "natural_cave_unremarkable" );
            static const oter_id natural_cave_river( "natural_cave_river" );
            const auto &id( get_ter( cur.x, cur.y, z ) );
            const bool is_rock = id == empty_rock;
            const bool is_natural_cave_subtype = id == natural_cave_unremarkable || id == natural_cave_river;

            if( rock_only && !is_rock ) {
                return pf::rejected;
            }

            if( !rock_only && !( is_rock || is_natural_cave_subtype ) ) {
                return pf::rejected;
            }

            const int dx = std::abs( cur.x - dest.x );
            const int dy = std::abs( cur.y - dest.y );
            const int d = 1;
            const int d2 = 1;
            const int terrain_factor = rng( 1, 5 );
            const int dist = d * ( dx + dy ) + ( d2 - 2 * d ) * std::min( dx, dy ) + terrain_factor;
            return dist;
        };
        return pf::find_path( point( src.x, src.y ), point( dest.x, dest.y ), width, height, estimate );
    };

    std::vector<tripoint> next;

    for( auto &l : surface_locations ) {
        ter( l ) = natural_cave_entrance;
        next.emplace_back( l + tripoint( 0, 0, -1 ) );
    }

    while( !next.empty() ) {
        std::vector<tripoint> dropandcontinue;
        for( size_t i = 0; i < next.size(); ++i ) {
            int closest = -1;
            int k = 0;
            for( size_t j = i + 1; j < next.size(); j++ ) {
                const int distance = trig_dist( next[i].x, next[i].y, next[j].x, next[j].y );
                if( distance < closest || closest < 0 ) {
                    closest = distance;
                    k = j;
                }
            }
            if( closest > 0 ) {
                const bool is_river = one_in( 8 ) && next[i].z < -1;
                pf::path path = cave_route_to( next[i], next[k], OMAPX, OMAPY, !is_river );
                size_t dropdivisor = std::max( 8 + next[i].z, 1 );
                size_t droppoint = path.nodes.size() / dropdivisor;
                size_t returnpoint = path.nodes.size() - droppoint;
                bool dodrops = droppoint > 8;
                bool skip = false;
                for( const auto &node : path.nodes ) {
                    if( dodrops && node.pos() == path.nodes[droppoint].pos() ) {
                        ter( node.x, node.y, next[i].z ) = natural_cave_descent;
                        dropandcontinue.emplace_back( tripoint( node.x, node.y, next[i].z - 1 ) );
                        skip = true;
                        continue;
                    } else if( dodrops && node.pos() == path.nodes[returnpoint].pos() ) {
                        ter( node.x, node.y, next[i].z ) = natural_cave_descent;
                        dropandcontinue.emplace_back( tripoint( node.x, node.y, next[i].z - 1 ) );
                        skip = false;
                        continue;
                    } else if( dodrops && skip ) {
                        continue;
                    }

                    std::vector<point> targets = closest_points_first( rng( 0, 1 ), node.x, node.y );
                    for( auto &t : targets ) {
                        if( t.x < 0 || t.x >= OMAPX || t.y < 0 || t.y >= OMAPY ) {
                            continue;
                        }
                        oter_id &cur_t = ter( tripoint( t.x, t.y, next[i].z ) );

                        if( is_river && t.x == node.x && t.y == node.y ) {
                            cur_t = natural_cave_river;
                            continue;
                        }

                        if( cur_t == empty_rock ) {
                            if( one_in( 10 ) && next[i].z > -OVERMAP_DEPTH ) {
                                cur_t = natural_cave_descent;
                            } else  {
                                cur_t = natural_cave_unremarkable;
                            }
                        }
                    }
                }
            }
        }

        next = dropandcontinue;
    }

    for( int z = -1; z > -OVERMAP_DEPTH; z-- ) {
        std::vector<tripoint> descents;
        for( int i = 0; i < OMAPX; i++ ) {
            for( int j = 0; j < OMAPY; j++ ) {
                const tripoint l( i, j, z );
                if( ter( l ) == natural_cave_descent ) {
                    descents.emplace_back( l + tripoint( 0, 0, -1 ) );
                }
            }
        }

        for( size_t i = 0; i < descents.size(); ++i ) {
            int closest = -1;
            int k = 0;
            for( size_t j = i + 1; j < descents.size(); j++ ) {
                const int distance = trig_dist( descents[i].x, descents[i].y, descents[j].x, descents[j].y );
                if( distance < closest || closest < 0 ) {
                    closest = distance;
                    k = j;
                }
            }
            if( closest > 0 ) {
                const bool is_river = one_in( 8 ) && descents[i].z < -1;
                pf::path path = cave_route_to( descents[i], descents[k], OMAPX, OMAPY, !is_river );
                for( const auto &node : path.nodes ) {
                    std::vector<point> targets = closest_points_first( rng( 0, 1 ), node.x, node.y );
                    for( auto &t : targets ) {
                        if( t.x < 0 || t.x >= OMAPX || t.y < 0 || t.y >= OMAPY ) {
                            continue;
                        }
                        oter_id &cur_t = ter( tripoint( t.x, t.y, descents[i].z ) );

                        if( ( cur_t == empty_rock || cur_t == natural_cave_unremarkable ) && is_river && t.x == node.x &&
                            t.y == node.y ) {
                            cur_t = natural_cave_river;
                            continue;
                        }

                        if( cur_t == empty_rock ) {
                            if( one_in( 25 ) && descents[i].z > -OVERMAP_DEPTH ) {
                                cur_t = natural_cave_descent;
                            } else {
                                cur_t = natural_cave_unremarkable;
                            }
                        }
                    }
                }
            }
        }
    }

    for( int z = -4; z > -OVERMAP_DEPTH; z-- ) {
        std::vector<tripoint> targets;
        std::vector<tripoint> sources;
        for( int i = 1; i < OMAPX - 1; i++ ) {
            for( int j = 1; j < OMAPY - 1; j++ ) {
                const tripoint l( i, j, z );
                const tripoint l_n( i, j - 1, z );
                const tripoint l_s( i, j + 1, z );
                const tripoint l_e( i + 1, j, z );
                const tripoint l_w( i - 1, j, z );

                const oter_id cur_t = ter( l );
                if( is_ot_match( "lab", cur_t, ot_match_type::contains) ) {
                    if( ter( l_n ) == empty_rock ) {
                        targets.push_back( l_n );
                    } else if( ter( l_s ) == empty_rock ) {
                        targets.push_back( l_s );
                    } else if( ter( l_e ) == empty_rock ) {
                        targets.push_back( l_e );
                    } else if( ter( l_w ) == empty_rock ) {
                        targets.push_back( l_w );
                    }
                }
                if( cur_t == natural_cave_unremarkable ) {
                    sources.push_back( l );
                }
            }
        }

        if( !targets.empty() && !sources.empty() ) {
            const tripoint target = random_entry( targets, invalid_tripoint );
            const tripoint source = random_entry( sources, invalid_tripoint );
            const bool is_river = one_in( 8 );
            pf::path path = cave_route_to( source, target, OMAPX, OMAPY, false );
            for( const auto &node : path.nodes ) {
                std::vector<point> targets = closest_points_first( rng( 0, 1 ), node.x, node.y );
                for( auto &t : targets ) {
                    if( t.x < 0 || t.x >= OMAPX || t.y < 0 || t.y >= OMAPY ) {
                        continue;
                    }
                    oter_id &cur_t = ter( tripoint( t.x, t.y, target.z ) );

                    if( ( cur_t == empty_rock || cur_t == natural_cave_unremarkable ) && is_river && t.x == node.x &&
                        t.y == node.y ) {
                        cur_t = natural_cave_river;
                        continue;
                    }

                    if( cur_t == empty_rock ) {
                        if( one_in( 25 ) && target.z > -OVERMAP_DEPTH ) {
                            cur_t = natural_cave_descent;
                        } else {
                            cur_t = natural_cave_unremarkable;
                        }
                    }
                }
            }
        }
    }

    std::vector<tripoint> unremarkable;
    for( int z = -1; z > -OVERMAP_DEPTH; z-- ) {
        for( int i = 1; i < OMAPX - 1; i++ ) {
            for( int j = 1; j < OMAPY - 1; j++ ) {
                const tripoint l( i, j, z );
                const oter_id cur_t = ter( l );
                if( cur_t == natural_cave_unremarkable ) {
                    unremarkable.push_back( l );
                }
            }
        }
    }

    std::shuffle( unremarkable.begin(), unremarkable.end(), eng );

    const oter_id natural_cave_remarkable_cross( "natural_cave_remarkable_cross" );
    const oter_id natural_cave_remarkable_tee_north( "natural_cave_remarkable_tee_north" );
    const oter_id natural_cave_remarkable_tee_east( "natural_cave_remarkable_tee_east" );
    const oter_id natural_cave_remarkable_tee_south( "natural_cave_remarkable_tee_south" );
    const oter_id natural_cave_remarkable_tee_west( "natural_cave_remarkable_tee_west" );
    const oter_id natural_cave_remarkable_line_north( "natural_cave_remarkable_line_north" );
    const oter_id natural_cave_remarkable_line_east( "natural_cave_remarkable_line_east" );
    const oter_id natural_cave_remarkable_elbow_north( "natural_cave_remarkable_elbow_north" );
    const oter_id natural_cave_remarkable_elbow_east( "natural_cave_remarkable_elbow_east" );
    const oter_id natural_cave_remarkable_elbow_south( "natural_cave_remarkable_elbow_south" );
    const oter_id natural_cave_remarkable_elbow_west( "natural_cave_remarkable_elbow_west" );
    const oter_id natural_cave_remarkable_end_north( "natural_cave_remarkable_end_north" );
    const oter_id natural_cave_remarkable_end_east( "natural_cave_remarkable_end_east" );
    const oter_id natural_cave_remarkable_end_south( "natural_cave_remarkable_end_south" );
    const oter_id natural_cave_remarkable_end_west( "natural_cave_remarkable_end_west" );

    size_t max_remarkable = unremarkable.size() * 0.05;
    for( size_t i = 0; i < max_remarkable; i++ ) {
        const tripoint l = unremarkable[i];

        const bool has_n = is_ot_match( "natural_cave", ter( tripoint( l.x, l.y - 1, l.z ) ), ot_match_type::prefix);
        const bool has_s = is_ot_match( "natural_cave", ter( tripoint( l.x, l.y + 1, l.z ) ), ot_match_type::prefix);
        const bool has_e = is_ot_match( "natural_cave", ter( tripoint( l.x + 1, l.y, l.z ) ), ot_match_type::prefix);
        const bool has_w = is_ot_match( "natural_cave", ter( tripoint( l.x - 1, l.y, l.z ) ), ot_match_type::prefix);

        oter_id &cur_t = ter( tripoint( l ) );

        if( has_n && has_e && has_s && has_w ) {
            cur_t = natural_cave_remarkable_cross;
        } else if( has_n && has_e && has_s ) {
            cur_t = natural_cave_remarkable_tee_north;
        } else if( has_e && has_s && has_w ) {
            cur_t = natural_cave_remarkable_tee_east;
        } else if( has_s && has_w && has_n ) {
            cur_t = natural_cave_remarkable_tee_south;
        } else if( has_w && has_n && has_e ) {
            cur_t = natural_cave_remarkable_tee_west;
        } else if( has_n && has_s ) {
            cur_t = natural_cave_remarkable_line_north;
        } else if( has_e && has_w ) {
            cur_t = natural_cave_remarkable_line_east;
        } else if( has_n && has_e ) {
            cur_t = natural_cave_remarkable_elbow_north;
        } else if( has_e && has_s ) {
            cur_t = natural_cave_remarkable_elbow_east;
        } else if( has_s && has_w ) {
            cur_t = natural_cave_remarkable_elbow_south;
        } else if( has_w && has_n ) {
            cur_t = natural_cave_remarkable_elbow_west;
        } else if( has_n ) {
            cur_t = natural_cave_remarkable_end_north;
        } else if( has_e ) {
            cur_t = natural_cave_remarkable_end_east;
        } else if( has_s ) {
            cur_t = natural_cave_remarkable_end_south;
        } else if( has_w ) {
            cur_t = natural_cave_remarkable_end_west;
        }
    }
}
