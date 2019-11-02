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

TEST_CASE( "mapgen_item_stats" )
{
    sqlite3* db;
    char *sErrMsg;

    sqlite3_open("stats.db", &db);

    sqlite3_exec(db, "PRAGMA synchronous=OFF", NULL, NULL, &sErrMsg);
    sqlite3_exec(db, "PRAGMA count_changes=OFF", NULL, NULL, &sErrMsg);
    sqlite3_exec(db, "PRAGMA journal_mode=MEMORY", NULL, NULL, &sErrMsg);
    sqlite3_exec(db, "PRAGMA temp_store=MEMORY", NULL, NULL, &sErrMsg);
    
    std::string sql = "create table item_stats (iteration int, oter_id text, itype_id text, count int);";
    sqlite3_exec(db, sql.c_str(), callback, 0, &sErrMsg);

    std::vector<std::string> locations = { "s_antique_north", "bar_north", "s_bike_shop_north", "s_bike_shop_1_north", "s_butcher_north", "s_butcher_1_north", "s_butcher_2_north", "cs_sex_shop_north",
    "cs_tire_shop_north", "s_restaurant_3_north", "dispensary_north", "dispensary_1_north", "dispensary_2_north", "dollarstore_north", "dollarstore_1_north",
    "s_garage_north", "s_garage_1_north", "s_garage_2_north", "garage_gas_1_north", "garage_gas_2_north", "garage_gas_3_north", "s_gardening_north", "s_gun_4_north", 
     "headshop_north", "home_improvement_north", "hdwr_large_entrance_north", "hdwr_large_SW_north", "hdwr_large_NW_north", "hdwr_large_NE_north", "hdwr_large_backroom_north", 
    "hdwr_large_1_2_0_north", "hdwr_large_0_2_0_north" ,
       "hdwr_large_1_1_0_north", "hdwr_large_0_1_0_north" ,
       "hdwr_large_1_0_0_north", "hdwr_large_0_0_0_north" ,
       "hdwr_large_1_2_1_north", "hdwr_large_0_2_1_north" ,
       "hdwr_large_1_1_1_north", "hdwr_large_0_1_1_north" ,
       "hdwr_large_1_0_1_north", "hdwr_large_0_0_1_north", "s_jewelry_north",
       "landscapingsupplyco_1a_north", "landscapingsupplyco_1b_north",
       "s_library_north", "s_library_2_north",
       "megastore_0_0_0_north",
"megastore_1_0_0_north",
"megastore_2_0_0_north",
"megastore_parking_north",
"megastore_0_1_0_north",
"megastore_1_1_0_north",
"megastore_2_1_0_north",
"megastore_parking_north",
"megastore_0_2_0_north",
"megastore_1_2_0_north",
"megastore_2_2_0_north",
"megastore_parking_north",
"megastore_parking_north",
"megastore_parking_north",
"megastore_parking_north",
"megastore_parking_north",
"megastore_parking_north",
"megastore_parking_north",
"megastore_parking_north",
"megastore_parking_north",
"megastore_0_0_1_north",
"megastore_1_0_1_north",
"megastore_2_0_1_north",
"megastore_0_1_1_north",
"megastore_1_1_1_north",
"megastore_2_1_1_north",
"megastore_0_2_1_north",
"megastore_1_2_1_north",
"megastore_2_2_1_north",
"megastore_0_0_roof_north",
"megastore_1_0_roof_north",
"megastore_2_0_roof_north",
"megastore_0_1_roof_north",
"megastore_1_1_roof_north",
"megastore_2_1_roof_north",
"megastore_0_2_roof_north",
"megastore_1_2_roof_north",
"megastore_2_2_roof_north",
"mil_surplus_north",
"mil_surplus_1_north",
"mil_surplus_2_north",
"museum_north",
"s_music_north",     
"office_doctor_north",
"office_doctor_1_north",
"office_doctor_2_north",
"pawn_north",
"pawn_1_north",
"pawn_pf_north",
"pawn_pf_under_north",
"s_petstore_north",
"s_petstore_1_north",
"s_petstore_2_north",
"s_pharm_north",
"s_pharm_1_north",
"s_pizza_parlor_north",
"s_pizza_parlor_1_north",
"s_restaurant_north",
"s_restaurant_1_north",
"s_restaurant_2_north",
"s_restaurant_foodplace_north",
"s_restaurant_fast_north",
"s_restaurant_fast_1_north",
"s_reststop_1_north",
"s_reststop_2_north",
"s_bookstore_north",
"s_bookstore_1_north",
"s_bookstore_2_north",
"s_clothes_north",
"s_clothes_1_north",
"s_clothes_2_north",
"s_clothes_3_north",
"s_clothes_4_north",
"s_clothes_5_north",
"s_clothes_6_north",
"s_restaurant_coffee_north",
"s_restaurant_coffee_1_north",
"s_restaurant_coffee_2_north",
"s_electronics_north",
"s_electronics_1_north",
"furniture_north",
"s_gas_north",
"s_gas_1_north",
"s_gas_rural_north",
"s_grocery_north",
"s_grocery_1_north",
"s_gun_north",
"s_gun_1_north",
"s_gun_2_north",
"s_gun_3_north",
"s_hardware_north",
"s_hardware_1_north",
"s_hardware_2_north",
"s_hardware_3_north",
"icecream_shop_north",
"s_liquor_north",
"smoke_lounge_north",
"smoke_lounge_1_north",
"s_sports_north",
"s_teashop_north",
"s_teashop_1_north",
"s_teashop_roof_1_north",
"s_thrift_north",
"veterinarian_north"
          };

    for(auto &ot : locations) {
        for(int i = 0; i < 100; i++) {
            std::map<std::pair<std::string, itype_id>, int> otit;

            oter_id otid = oter_id(ot);
            std::string otidstr = otid->get_type_id().str();
            overmap_buffer.ter_set( {0,0,0}, otid );
            tinymap tmpmap;
            tmpmap.generate( {0,0,0}, calendar::turn );

            for(int tx = 0; tx < SEEX*2; tx++) {
                for(int ty = 0; ty < SEEY *2; ty++) {
                    map_stack ms = tmpmap.i_at({tx, ty, 0});
                    for( item &it : ms ) {
                        otit[{otidstr, it.typeId()}] += 1;

                        if(it.is_container() && !it.is_container_empty()) {
                            const item &cit = it.get_contained();  
                            otit[{otidstr, cit.typeId()}] += 1;  
                        }
                    }
                }
            }

            sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &sErrMsg);

            std::string otisql = "insert into item_stats values (?, ?, ?, ?);";
            sqlite3_stmt * otistmt;
            sqlite3_prepare_v2(db,  otisql.c_str(), otisql.length(), &otistmt, NULL);
            
            for(auto &x : otit) {
                sqlite3_bind_int(otistmt, 1, i);
                sqlite3_bind_text(otistmt, 2, x.first.first.c_str(), -1, SQLITE_TRANSIENT );
                sqlite3_bind_text(otistmt, 3, x.first.second.c_str(), -1, SQLITE_TRANSIENT );
                sqlite3_bind_int(otistmt, 4, x.second);

                sqlite3_step(otistmt);
                sqlite3_reset(otistmt);
            }

            sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &sErrMsg);
            sqlite3_finalize(otistmt);
        }
    }

    sqlite3_close(db);

    int count = 0;
    CHECK(count == 10);
}


TEST_CASE( "overmap_generation_statistics" )
{
    sqlite3* db;
    char *sErrMsg;

    sqlite3_open("stats.db", &db);

    sqlite3_exec(db, "PRAGMA synchronous=OFF", NULL, NULL, &sErrMsg);
    sqlite3_exec(db, "PRAGMA count_changes=OFF", NULL, NULL, &sErrMsg);
    sqlite3_exec(db, "PRAGMA journal_mode=MEMORY", NULL, NULL, &sErrMsg);
    sqlite3_exec(db, "PRAGMA temp_store=MEMORY", NULL, NULL, &sErrMsg);
    
    std::string sql = "create table overmap_terrain (overmap_x int, overmap_y int, x int, y int, z int, oter_id text);";
    sqlite3_exec(db, sql.c_str(), callback, 0, &sErrMsg);

    sql = "create table ot_furniture (overmap_x int, overmap_y int, x int, y int, z int, furn_id text);";
    sqlite3_exec(db, sql.c_str(), callback, 0, &sErrMsg);

    sql = "create table ot_terrain (overmap_x int, overmap_y int, x int, y int, z int, ter_id text);";
    sqlite3_exec(db, sql.c_str(), callback, 0, &sErrMsg);

    sql = "create table ot_item (overmap_x int, overmap_y int, x int, y int, z int, itype_id text);";
    sqlite3_exec(db, sql.c_str(), callback, 0, &sErrMsg);

    // std::map<string_id<oter_type_t>, int> occurrences;
    // std::map<ter_id, int> toc;
    // std::map<furn_id, int> foc;
    // std::map<itype_id, int> ioc;
    // std::map<std::pair<string_id<oter_type_t>, itype_id>, int> otit;

    // Loop through the grid of overmap points
    for( point p : closest_points_first( 0, point_zero ) ) {

        // If we haven't already created this one, then create it (sometimes they get created by spill-over)
        if( !overmap_buffer.has( p ) ) {
            overmap_special_batch test_specials = overmap_specials::get_default_batch( p );
            overmap_buffer.create_custom_overmap( p, test_specials );
        }

        // Get the overmap
        overmap *om = overmap_buffer.get_existing( p );
        
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &sErrMsg);

        std::string isql = "insert into overmap_terrain values (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt * stmt;
        sqlite3_prepare_v2(db,  isql.c_str(), isql.length(), &stmt, NULL);

        std::string otfsql = "insert into ot_furniture values (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt * otfstmt;
        sqlite3_prepare_v2(db,  otfsql.c_str(), otfsql.length(), &otfstmt, NULL);

        std::string ottsql = "insert into ot_terrain values (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt * ottstmt;
        sqlite3_prepare_v2(db,  ottsql.c_str(), ottsql.length(), &ottstmt, NULL);

        std::string otisql = "insert into ot_item values (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt * otistmt;
        sqlite3_prepare_v2(db,  otisql.c_str(), otisql.length(), &otistmt, NULL);

        int z = 0;
        //for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            for( int x = 0; x < OMAPX; ++x ) {
                for( int y = 0; y < OMAPY; ++y ) {

                    // Get the overmap terrain at this xyz
                    const oter_id t = om->ter( { x, y, z } );

                    sqlite3_bind_int(stmt, 1, om->global_base_point().x);
                    sqlite3_bind_int(stmt, 2, om->global_base_point().y);
                    sqlite3_bind_int(stmt, 3, x);
                    sqlite3_bind_int(stmt, 4, y);
                    sqlite3_bind_int(stmt, 5, z);
                    sqlite3_bind_text(stmt, 6, t->get_type_id().c_str(), -1, SQLITE_TRANSIENT);

                    sqlite3_step(stmt);
                    sqlite3_reset(stmt);
                    // Run the mapgen for this overmap terrain
                    tinymap tmpmap;
                    tmpmap.generate( omt_to_sm_copy( tripoint(om->global_base_point(), 0) + tripoint(x, y, z ) ), calendar::turn );

                    // occurrencest.obj().get_type_id() += 1;

                    for(int tx = 0; tx < SEEX*2; tx++) {
                        for(int ty = 0; ty < SEEY *2; ty++) {
                            const ter_id ti = tmpmap.ter({tx, ty});
                            const furn_id fi = tmpmap.furn({tx, ty});

                            sqlite3_bind_int(otfstmt, 1, om->global_base_point().x);
                            sqlite3_bind_int(otfstmt, 2, om->global_base_point().y);
                            sqlite3_bind_int(otfstmt, 3, x);
                            sqlite3_bind_int(otfstmt, 4, y);
                            sqlite3_bind_int(otfstmt, 5, z);
                            sqlite3_bind_text(otfstmt, 6, fi->id.c_str(), -1, SQLITE_TRANSIENT);

                            sqlite3_step(otfstmt);
                            sqlite3_reset(otfstmt);

                            sqlite3_bind_int(ottstmt, 1, om->global_base_point().x);
                            sqlite3_bind_int(ottstmt, 2, om->global_base_point().y);
                            sqlite3_bind_int(ottstmt, 3, x);
                            sqlite3_bind_int(ottstmt, 4, y);
                            sqlite3_bind_int(ottstmt, 5, z);
                            sqlite3_bind_text(ottstmt, 6, ti->id.c_str(), -1, SQLITE_TRANSIENT);

                            sqlite3_step(ottstmt);
                            sqlite3_reset(ottstmt);


                            map_stack ms = tmpmap.i_at({tx, ty, z});
                            for( item &it : ms ) {

                                sqlite3_bind_int(otistmt, 1, om->global_base_point().x);
                                sqlite3_bind_int(otistmt, 2, om->global_base_point().y);
                                sqlite3_bind_int(otistmt, 3, x);
                                sqlite3_bind_int(otistmt, 4, y);
                                sqlite3_bind_int(otistmt, 5, z);
                                sqlite3_bind_text(otistmt, 6, it.typeId().c_str(), -1, SQLITE_TRANSIENT);

                                sqlite3_step(otistmt);
                                sqlite3_reset(otistmt);
                            }
                        }
                    }
                }
            }
        //}

        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &sErrMsg);
        sqlite3_finalize(stmt);
    }


    sqlite3_close(db);

    return;
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
        */
 
    int count = 0;
    CHECK(count == 10);
}