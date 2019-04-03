#include <cmath>

#include "game.h"
#include "overmap_noise.h"
#include "simplexnoise.h"

namespace om_noise
{

float om_noise_layer_forest::noise_at( const point &local_omt_pos ) const
{
    const point p = global_omt_pos( local_omt_pos );
    float r = scaled_octave_noise_3d( 4, 0.5, 0.03, 0, 1, p.x, p.y, seed );
    r = std::powf( r, 2 );
    return r;
}

float om_noise_layer_floodplain::noise_at( const point &local_omt_pos ) const
{
    const point p = global_omt_pos( local_omt_pos );
    float r = scaled_octave_noise_3d( 8, 0.5, 0.05, 0, 1, p.x, p.y, seed );
    r = std::powf( r, 2 );
    return r;
}

}
