#include <memory>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "enums.h"
#include "game_constants.h"
#include "type_id.h"
#include "point.h"
#include "magic_ter_furn_transform.h"

TEST_CASE( "ter_furn_transform_grass_to_dirt" )
{
    clear_map();
    GIVEN( "A t_grass terrain exists" ) {
        const tripoint test_origin( 0, 0, 0 );
        g->u.setpos( test_origin );
        const tripoint grass_point = test_origin + tripoint_east;
        g->m.ter_set( grass_point, ter_id( "t_grass" ) );
        WHEN( "The t_grass terrain is targeted with a ter_furn_transform that turns it into t_dirt" ) {
            const ter_furn_transform_id test_transform( "mapgen_test" );
            test_transform->transform( g->m, grass_point );
            THEN( "The targeted terrain is now t_dirt" ) {
                CHECK( g->m.ter( grass_point ) == ter_id( "t_dirt" ) );
            }
        }
    }
}