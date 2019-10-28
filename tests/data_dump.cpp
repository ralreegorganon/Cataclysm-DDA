#include <memory>
#include <string>
#include <vector>
#include <map>

#include <sqlite3.h>

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

int callback(void *, int, char **, char **) {
    return 0;
}


TEST_CASE( "overmap_generation_statistics" )
{
    sqlite3* db;
    sqlite3_open("stats.db", &db);
    char *zErrMsg = 0;
    std::string sql = "create table overmap_terrain (overmap_x int, overmap_y int, x int, y int, oter_id text);";
    sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
    sqlite3_close(db);

    return;

    std::map<string_id<oter_type_t>, int> occurrences;
    std::map<ter_id, int> toc;
    std::map<furn_id, int> foc;
    std::map<itype_id, int> ioc;
    std::map<std::pair<string_id<oter_type_t>, itype_id>, int> otit;

    // Loop through the grid of overmap points
    for( point p : closest_points_first( 0, point_zero ) ) {

        // If we haven't already created this one, then create it (sometimes they get created by spill-over)
        if( !overmap_buffer.has( p ) ) {
            overmap_special_batch test_specials = overmap_specials::get_default_batch( p );
            overmap_buffer.create_custom_overmap( p, test_specials );
        }

        // Get the overmap
        overmap *om = overmap_buffer.get_existing( p );
        
        int z = 0;
        //for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            for( int x = 0; x < OMAPX; ++x ) {
                for( int y = 0; y < OMAPY; ++y ) {

                    // Get the overmap terrain at this xyz
                    const oter_id t = om->ter( { x, y, z } );

                    // Run the mapgen for this overmap terrain
                    tinymap tmpmap;
                    tmpmap.generate( omt_to_sm_copy( tripoint(om->global_base_point(), 0) + tripoint(x, y, z ) ), calendar::turn );

                    occurrences[t.obj().get_type_id()] += 1;

                    for(int tx = 0; tx < SEEX*2; tx++) {
                        for(int ty = 0; ty < SEEY *2; ty++) {
                            const ter_id ti = tmpmap.ter({tx, ty});
                            toc[ti] += 1;
                            const furn_id fi = tmpmap.furn({tx, ty});
                            foc[fi] += 1;
                            map_stack ms = tmpmap.i_at({tx, ty, z});
                            
                            for( item &it : ms ) {
                                ioc[it.typeId()] +=1;
                                otit[{t.obj().get_type_id(), it.typeId()}] +=1; 
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
    // for(auto &x : ioc) {
    //     std::cout << x.first << " , " << x.second << std::endl;
    // }

    for(auto &x : otit) {
        std::cout << x.first.first.str() << " , " << x.first.second << " , " << x.second << std::endl;
    }

    std::cout << "END OF ITEM" << std::endl;

    for(auto &x : occurrences) {
        std::cout << x.first.str() << " , " << x.second << std::endl;
    }
 
    int count = 0;
    CHECK(count == 10);
}