#include <fstream>

#include "catch/catch.hpp"
#include "enums.h"
#include "game_constants.h"
#include "overmap_noise.h"

TEST_CASE( "om_noise_layer_forest_raw_export", "[.]" )
{
    std::ofstream testfile;
    testfile.open( "forest-map-raw.pgm", std::ofstream::trunc );
    testfile << "P2" << std::endl;
    testfile << "180 180" << std::endl;
    testfile << "255" << std::endl;

    const om_noise::om_noise_layer_forest f( {0, 0}, 1920237457 );

    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            testfile << static_cast<int>( f.noise_at( {x, y} ) * 255 ) << " ";
        }
        testfile << std::endl;
    }

    testfile.close();
}

TEST_CASE( "om_noise_layer_floodplain_raw_export", "[.]" )
{
    std::ofstream testfile;
    testfile.open( "floodplain-map-raw.pgm", std::ofstream::trunc );
    testfile << "P2" << std::endl;
    testfile << "180 180" << std::endl;
    testfile << "255" << std::endl;

    const om_noise::om_noise_layer_floodplain f( {0, 0}, 1920237457 );

    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            testfile << static_cast<int>( f.noise_at( {x, y} ) * 255 ) << " ";
        }
        testfile << std::endl;
    }

    testfile.close();
}