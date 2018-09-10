#include "catch/catch.hpp"

#include "game.h"
#include "weather_gen.h"

#include "test_statistics.h"

struct weather_test_data {
    statistics<double> temp_c;

    weather_test_data() : temp_c( Z95 ) {}
};

struct weather_test_data2 {
    statistics<double> windpower_mph;

    weather_test_data2() : windpower_mph( Z95 ) {}
};

TEST_CASE( "basic_weather_test", "[weather],[balance]" )
{
    const epsilon_threshold temp_thresh = { 30, 1};
    bool temp_thresh_met = false;
    weather_test_data data;

    int max_iterations = 100;

    const weather_generator &wg = g->get_cur_weather_gen();

    do {
        const auto seed = rand();
        const weather_generator &wg = g->get_cur_weather_gen();
        const time_point begin = calendar::turn;
        const time_point end = begin + 1_days;
        for( time_point i = begin; i < end; i += 200_turns ) {
            w_point w = wg.get_weather( tripoint( 0, 0, 0 ), to_turn<int>( i ), seed );
            data.temp_c.add( w.temperature );
        }

        if( !temp_thresh_met ) {
            temp_thresh_met = data.temp_c.test_threshold( temp_thresh );
        }

    } while( !temp_thresh_met && data.temp_c.n() < max_iterations );

    INFO( "Total temps: " << data.temp_c.n() );
    INFO( "Avg temp (C): " << data.temp_c.avg() );
    INFO( "Temp Lower (C): " << data.temp_c.lower() << " Temp Upper (C): " << data.temp_c.upper() );
    INFO( "Temp Thresh: " << temp_thresh.midpoint - temp_thresh.epsilon << " - " <<
          temp_thresh.midpoint + temp_thresh.epsilon );
    INFO( "Margin of error: " << data.temp_c.margin_of_error() );
    CHECK( temp_thresh_met );
}

TEST_CASE( "wind_weather_test", "[weather],[balance]" )
{
    const epsilon_threshold windpower_thresh = { 1013, 1};
    bool windpower_thresh_met = false;
    weather_test_data2 data;

    int max_iterations = 1000000;

    const weather_generator &wg = g->get_cur_weather_gen();

    const auto seed = 4;

    do {
        const weather_generator &wg = g->get_cur_weather_gen();
        const time_point begin = calendar::turn;
        const time_point end = begin + 1_days;
        for( time_point i = begin; i < end; i += 200_turns ) {
            w_point w = wg.get_weather( tripoint( 0, 0, 0 ), to_turn<int>( i ), seed );
            data.windpower_mph.add( w.pressure );
        }

        if( !windpower_thresh_met ) {
            windpower_thresh_met = data.windpower_mph.test_threshold( windpower_thresh );
        }

    } while( !windpower_thresh_met && data.windpower_mph.n() < max_iterations );

    INFO( "Total measurements: " << data.windpower_mph.n() );
    INFO( "Avg pressure (hPa): " << data.windpower_mph.avg() );
    INFO( "Pressure Lower (hPa): " << data.windpower_mph.lower() << " Pressure Upper (hPa): " <<
          data.windpower_mph.upper() );
    INFO( "Pressure Min (hPa): " << data.windpower_mph.min() << " Pressure Max (hPa): " <<
          data.windpower_mph.max() );
    INFO( "Pressure Thresh: " << windpower_thresh.midpoint - windpower_thresh.epsilon << " - " <<
          windpower_thresh.midpoint + windpower_thresh.epsilon );
    INFO( "Margin of error: " << data.windpower_mph.margin_of_error() );
    CHECK( windpower_thresh_met );
}