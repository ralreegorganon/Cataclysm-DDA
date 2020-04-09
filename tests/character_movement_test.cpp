#include "catch/catch.hpp"
#include "game.h"
#include "map_helpers.h"
#include "map.h"
#include "player_helpers.h"
#include "avatar.h"

TEST_CASE("character_water_movement", "[water][movement]")
{
    build_test_lake_map();

    avatar& p = g->u;
    
    clear_character(p);

    const tripoint test_origin(60, 60, 0);
    p.setpos(test_origin);

    REQUIRE(g->m.ter(p.pos()) == ter_id("t_water_dp"));
    REQUIRE(g->m.ter(p.pos() + tripoint(0, 0, -1)) == ter_id("t_water_cube"));
    REQUIRE(g->m.ter(p.pos() + tripoint(0, 0, -2)) == ter_id("t_water_cube"));
    REQUIRE(g->m.ter(p.pos() + tripoint(0,0,-3)) == ter_id("t_lake_bed"));

    // We start out on the surface, swimming along.
    REQUIRE_FALSE(p.is_underwater());
    REQUIRE(p.pos().z == 0);

    // We attempt to dive down.
    g->vertical_move(-1, true);

    // The first time we dive down from the surface, we don't actually go down a z-level but do go underwater in the current tile.
    CHECK(p.is_underwater());
    CHECK(p.pos().z == 0);

    // We attempt to surface.
    g->vertical_move(1, true);

    CHECK(!p.is_underwater());
    CHECK(p.pos().z == 0);

    // We attempt to dive down twice, which should actually move a z-level.
    g->vertical_move(-1, true);
    g->vertical_move(-1, true);

    CHECK(p.is_underwater());
    CHECK(p.pos().z == -1);
}
