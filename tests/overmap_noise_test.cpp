#include <fstream>
#include <string>

#include "catch/catch.hpp"
#include "game_constants.h"
#include "overmap_noise.h"

static void export_raw_noise( const std::string &filename, const om_noise::om_noise_layer &noise,
                              int width, int height )
{
    std::ofstream testfile;
    testfile.open( filename, std::ofstream::trunc );
    testfile << "P2" << std::endl;
    testfile << width << " " << height << std::endl;
    testfile << "255" << std::endl;

    for( int x = 0; x < width; x++ ) {
        for( int y = 0; y < height; y++ ) {
            testfile << static_cast<int>( noise.noise_at( {x, y} ) * 255 ) << " ";
        }
        testfile << std::endl;
    }

    testfile.close();
}

static void export_interpreted_noise(
    const std::string &filename, const om_noise::om_noise_layer &noise, int width, int height,
    float threshold )
{
    std::ofstream testfile;
    testfile.open( filename, std::ofstream::trunc );
    testfile << "P2" << std::endl;
    testfile << width << " " << height << std::endl;
    testfile << "255" << std::endl;

    for( int x = 0; x < width; x++ ) {
        for( int y = 0; y < height; y++ ) {
            if( noise.noise_at( {x, y} ) > threshold ) {
                testfile << 255 << " ";
            } else {
                testfile << 0 << " ";
            }
        }
        testfile << std::endl;
    }

    testfile.close();
}


TEST_CASE( "om_noise_layer_forest_export", "[.]" )
{
    const om_noise::om_noise_layer_forest f( {0, 0}, 1920237457 );
    export_raw_noise( "forest-map-raw.pgm", f, OMAPX, OMAPY );
}

TEST_CASE( "om_noise_layer_floodplain_export", "[.]" )
{
    const om_noise::om_noise_layer_floodplain f( {0, 0}, 1920237457 );
    export_raw_noise( "floodplain-map-raw.pgm", f, OMAPX, OMAPY );
}

TEST_CASE( "om_noise_layer_lake_export", "[.]" )
{
    const om_noise::om_noise_layer_lake f( {0, 0}, 1920237457 );
    export_raw_noise( "lake-map-raw.pgm", f, OMAPX * 5, OMAPY * 5 );
    export_interpreted_noise( "lake-map-interp.pgm", f, OMAPX * 5, OMAPY * 5, 0.25 );
}

TEST_CASE( "om_noise_layer_human_population_density_export", "[.]" )
{
    const om_noise::om_noise_layer_human_population_density f1( {0, 0}, 1920237457, 8, 4 );
    const om_noise::om_noise_layer_human_population_density f2( {0, 0}, 1920237457, 8, 1 );
    const om_noise::om_noise_layer_human_population_density f3( {0, 0}, 1920237457, 8, 8 );
    const om_noise::om_noise_layer_human_population_density f4( {0, 0}, 1920237457, 1, 4 );
    const om_noise::om_noise_layer_human_population_density f5( {0, 0}, 1920237457, 1, 1 );
    const om_noise::om_noise_layer_human_population_density f6( {0, 0}, 1920237457, 1, 8 );
    const om_noise::om_noise_layer_human_population_density f7( {0, 0}, 1920237457, 16, 4 );
    const om_noise::om_noise_layer_human_population_density f8( {0, 0}, 1920237457, 16, 1 );
    const om_noise::om_noise_layer_human_population_density f9( {0, 0}, 1920237457, 16, 8 );

    // export_interpreted_noise( "human-population-density-raw-8-4.pgm", f1, OMAPX * 5, OMAPY * 5, 0.5 );
    // export_interpreted_noise( "human-population-density-raw-8-1.pgm", f2, OMAPX * 5, OMAPY * 5, 0.5 );
    // export_interpreted_noise( "human-population-density-raw-8-8.pgm", f3, OMAPX * 5, OMAPY * 5, 0.5 );
    export_raw_noise( "human-population-density-raw-8-4.pgm", f1, OMAPX * 5, OMAPY * 5 );
    export_raw_noise( "human-population-density-raw-8-1.pgm", f2, OMAPX * 5, OMAPY * 5 );
    export_raw_noise( "human-population-density-raw-8-8.pgm", f3, OMAPX * 5, OMAPY * 5 );
    // export_raw_noise( "human-population-density-raw-1-4.pgm", f4, OMAPX * 5, OMAPY * 5 );
    // export_raw_noise( "human-population-density-raw-1-1.pgm", f5, OMAPX * 5, OMAPY * 5 );
    // export_raw_noise( "human-population-density-raw-1-8.pgm", f6, OMAPX * 5, OMAPY * 5 );
    // export_raw_noise( "human-population-density-raw-16-4.pgm", f7, OMAPX * 5, OMAPY * 5 );
    // export_raw_noise( "human-population-density-raw-16-1.pgm", f8, OMAPX * 5, OMAPY * 5 );
    // export_raw_noise( "human-population-density-raw-16-8.pgm", f9, OMAPX * 5, OMAPY * 5 );
}
