#include <memory>
#include <string>
#include <vector>
#include <map>

#include "catch/catch.hpp"
#include "map.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "calendar.h"
#include "common_types.h"
#include "omdata.h"
#include "overmap_types.h"
#include "type_id.h"
#include "game_constants.h"
#include "point.h"
#include "coordinate_conversions.h"

TEST_CASE( "set_and_get_overmap_scents" )
{
    std::unique_ptr<overmap> test_overmap = std::make_unique<overmap>( point_zero );

    // By default there are no scents set.
    for( int x = 0; x < 180; ++x ) {
        for( int y = 0; y < 180; ++y ) {
            for( int z = -10; z < 10; ++z ) {
                REQUIRE( test_overmap->scent_at( { x, y, z } ).creation_time == -1 );
            }
        }
    }

    scent_trace test_scent( 50, 90 );
    test_overmap->set_scent( { 75, 85, 0 }, test_scent );
    REQUIRE( test_overmap->scent_at( { 75, 85, 0} ).creation_time == 50 );
    REQUIRE( test_overmap->scent_at( { 75, 85, 0} ).initial_strength == 90 );
}

TEST_CASE( "default_overmap_generation_always_succeeds" )
{
    int overmaps_to_construct = 10;
    for( const point &candidate_addr : closest_points_first( point_zero, 10 ) ) {
        // Skip populated overmaps.
        if( overmap_buffer.has( candidate_addr ) ) {
            continue;
        }
        overmap_special_batch test_specials = overmap_specials::get_default_batch( candidate_addr );
        overmap_buffer.create_custom_overmap( candidate_addr, test_specials );
        for( const auto &special_placement : test_specials ) {
            auto special = special_placement.special_details;
            INFO( "In attempt #" << overmaps_to_construct
                  << " failed to place " << special->id.str() );
            CHECK( special->occurrences.min <= special_placement.instances_placed );
        }
        if( --overmaps_to_construct <= 0 ) {
            break;
        }
    }
}

TEST_CASE( "default_overmap_generation_has_non_mandatory_specials_at_origin" )
{
    const point origin = point_zero;

    overmap_special mandatory;
    overmap_special optional;

    // Get some specific overmap specials so we can assert their presence later.
    // This should probably be replaced with some custom specials created in
    // memory rather than tying this test to these, but it works for now...
    for( const auto &elem : overmap_specials::get_all() ) {
        if( elem.id == "Cabin" ) {
            optional = elem;
        } else if( elem.id == "Lab" ) {
            mandatory = elem;
        }
    }

    // Make this mandatory special impossible to place.
    mandatory.city_size.min = 999;

    // Construct our own overmap_special_batch containing only our single mandatory
    // and single optional special, so we can make some assertions.
    std::vector<const overmap_special *> specials;
    specials.push_back( &mandatory );
    specials.push_back( &optional );
    overmap_special_batch test_specials = overmap_special_batch( origin, specials );

    // Run the overmap creation, which will try to place our specials.
    overmap_buffer.create_custom_overmap( origin, test_specials );

    // Get the origin overmap...
    overmap *test_overmap = overmap_buffer.get_existing( origin );

    // ...and assert that the optional special exists on this map.
    bool found_optional = false;
    for( int x = 0; x < OMAPX; ++x ) {
        for( int y = 0; y < OMAPY; ++y ) {
            const oter_id t = test_overmap->ter( { x, y, 0 } );
            if( t->id == "cabin" ||
                t->id == "cabin_north" || t->id == "cabin_east" ||
                t->id == "cabin_south" || t->id == "cabin_west" ) {
                found_optional = true;
            }
        }
    }

    INFO( "Failed to place optional special on origin " );
    CHECK( found_optional == true );
}

TEST_CASE( "overmap_generation_statistics" )
{
    std::map<oter_id, int> occurrences;
    std::map<ter_id, int> toc;
    std::map<furn_id, int> foc;
    std::map<itype_id, int> ioc;
    int count = 0;
    for( point p : closest_points_first( 1, point_zero ) ) {
        count++;
        if( !overmap_buffer.has( p ) ) {
            overmap_special_batch test_specials = overmap_specials::get_default_batch( p );
            overmap_buffer.create_custom_overmap( p, test_specials );
        }
        overmap *om = overmap_buffer.get_existing( p );
        int z = 0;
        //for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            for( int x = 0; x < OMAPX; ++x ) {
                for( int y = 0; y < OMAPY; ++y ) {
                    const oter_id t = om->ter( { x, y, z } );
                    occurrences[t] += 1;

                    tinymap tmpmap;
                    tmpmap.generate( omt_to_sm_copy( tripoint(om->global_base_point(), 0) + tripoint(x, y, z ) ), calendar::turn );
                    for(int tx = 0; tx < SEEX*2; tx++) {
                        for(int ty = 0; ty < SEEY *2; ty++) {
                            const ter_id ti = tmpmap.ter({tx, ty});
                            toc[ti] += 1;
                            const furn_id fi = tmpmap.furn({tx, ty});
                            foc[fi] += 1;
                            map_stack ms = tmpmap.i_at({tx, ty, z});
                            
                            for( item &it : ms ) {
                                ioc[it.typeId()] +=1;
                            }
                        }
                    }
                }
            }
        //}
    }

    /*
    for(auto &x : occurrences) {
        std::cout << x.first.id().str() << " , " << x.second << std::endl;
    }
    */

    /*
    for(auto &x : toc) {
        std::cout << x.first.id().str() << " , " << x.second << std::endl;
    }

    std::cout << "END OF TER" << std::endl;

    for(auto &x : foc) {
        std::cout << x.first.id().str() << " , " << x.second << std::endl;
    }
    

    std::cout << "END OF TER" << std::endl;
*/
    for(auto &x : ioc) {
        std::cout << x.first << " , " << x.second << std::endl;
    }
 
    CHECK(count == 10);
}