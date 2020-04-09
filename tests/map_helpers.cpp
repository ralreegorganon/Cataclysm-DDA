#include "map_helpers.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "monster.h"
#include "npc.h"
#include "field.h"
#include "game_constants.h"
#include "pimpl.h"
#include "type_id.h"
#include "point.h"

void wipe_map_terrain()
{
    const int mapsize = g->m.getmapsize() * SEEX;
    for( int z = 0; z <= OVERMAP_HEIGHT; ++z ) {
        ter_id terrain = z == 0 ? t_grass : t_open_air;
        for( int x = 0; x < mapsize; ++x ) {
            for( int y = 0; y < mapsize; ++y ) {
                g->m.set( { x, y, z}, terrain, f_null );
            }
        }
    }
    for( wrapped_vehicle &veh : g->m.get_vehicles() ) {
        g->m.destroy_vehicle( veh.v );
    }
    g->m.invalidate_map_cache( 0 );
    g->m.build_map_cache( 0, true );
}

void clear_creatures()
{
    // Remove any interfering monsters.
    g->clear_zombies();
}

void clear_npcs()
{
    // Reload to ensure that all active NPCs are in the overmap_buffer.
    g->reload_npcs();
    for( npc &n : g->all_npcs() ) {
        n.die( nullptr );
    }
    g->cleanup_dead();
}

void clear_fields( const int zlevel )
{
    const int mapsize = g->m.getmapsize() * SEEX;
    for( int x = 0; x < mapsize; ++x ) {
        for( int y = 0; y < mapsize; ++y ) {
            const tripoint p( x, y, zlevel );
            std::vector<field_type_id> fields;
            for( auto &pr : g->m.field_at( p ) ) {
                fields.push_back( pr.second.get_field_type() );
            }
            for( field_type_id f : fields ) {
                g->m.remove_field( p, f );
            }
        }
    }
}

void clear_map()
{
    // Clearing all z-levels is rather slow, so just clear the ones I know the
    // tests use for now.
    for( int z = -2; z <= 0; ++z ) {
        clear_fields( z );
    }
    wipe_map_terrain();
    clear_npcs();
    clear_creatures();
    g->m.clear_traps();
}

void clear_map_and_put_player_underground()
{
    clear_map();
    // Make sure the player doesn't block the path of the monster being tested.
    g->u.setpos( { 0, 0, -2 } );
}

monster &spawn_test_monster( const std::string &monster_type, const tripoint &start )
{
    monster *const added = g->place_critter_at( mtype_id( monster_type ), start );
    assert( added );
    return *added;
}

// Remove all vehicles from the map
void clear_vehicles()
{
    VehicleList vehs = g->m.get_vehicles();
    vehicle *veh_ptr;
    for( auto &vehs_v : vehs ) {
        veh_ptr = vehs_v.v;
        g->m.destroy_vehicle( veh_ptr );
    }
}

// Build a map of size MAPSIZE_X x MAPSIZE_Y around tripoint_zero with a given
// terrain, and no furniture, traps, or items.
void build_test_map( const ter_id &terrain )
{
    for( const tripoint &p : g->m.points_in_rectangle( tripoint_zero,
            tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        g->m.furn_set( p, furn_id( "f_null" ) );
        g->m.ter_set( p, terrain );
        g->m.trap_set( p, trap_id( "tr_null" ) );
        g->m.i_clear( p );
    }

    g->m.invalidate_map_cache( 0 );
    g->m.build_map_cache( 0, true );
}

void build_test_lake_map()
{
    const int z_lakebed = -3;

    const furn_id f_null( "f_null" );
    const trap_id tr_null( "tr_null" );
    const ter_id t_open_air( "t_open_air" );
    const ter_id t_water_dp( "t_water_dp" );
    const ter_id t_water_cube( "t_water_cube" );
    const ter_id t_lake_bed( "t_lake_bed" );

    const tripoint from( 0, 0, OVERMAP_HEIGHT );
    const tripoint to( MAPSIZE_X * SEEX, MAPSIZE_Y * SEEY, z_lakebed);

    for( const tripoint &p : g->m.points_in_rectangle( from, to ) ) {
        g->m.furn_set( p, f_null );
        g->m.i_clear( p );

        if( p.z > 0 ) {
            g->m.ter_set( p, t_open_air );
        } else if( p.z == 0 ) {
            g->m.ter_set( p, t_water_dp );
        } else if( p.z < 0 && p.z > z_lakebed) {
            g->m.ter_set( p, t_water_cube );
        } else if( p.z == z_lakebed) {
            g->m.ter_set( p, t_lake_bed );
        }
    }

    const int minz = z_lakebed;
    const int maxz = OVERMAP_HEIGHT;
    for( int z = minz; z <= maxz; z++ ) {
        g->m.invalidate_map_cache( z );
        g->m.build_map_cache( z, true );
    }

    clear_npcs();
    clear_creatures();

    for (wrapped_vehicle& veh : g->m.get_vehicles()) {
        g->m.destroy_vehicle(veh.v);
    }
}
