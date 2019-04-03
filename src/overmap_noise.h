#pragma once
#ifndef OVERMAP_NOISE_H
#define OVERMAP_NOISE_H

#include "int_id.h"
#include "omdata.h"

using oter_id = int_id<oter_t>;

namespace om_noise
{
class om_noise_layer
{
    public:
        virtual float noise_at( const point & ) const = 0;
        virtual ~om_noise_layer() = default;
    protected:
        om_noise_layer( const point &global_base_point, const unsigned seed ) :
            om_global_base_point( global_base_point ),
            seed( seed % 32768 ) {
        }

        point global_omt_pos( const point &local_omt_pos ) const {
            return om_global_base_point + local_omt_pos;
        }

        point om_global_base_point;
        unsigned seed;
};


class om_noise_layer_forest : om_noise_layer
{
    public:
        om_noise_layer_forest( const point &global_base_point, unsigned seed )
            : om_noise_layer( global_base_point, seed ) {
        }

        float noise_at( const point &local_omt_pos ) const override;
};

class om_noise_layer_floodplain : om_noise_layer
{
    public:
        om_noise_layer_floodplain( const point &global_base_point, unsigned seed )
            : om_noise_layer( global_base_point, seed ) {
        }

        float noise_at( const point &local_omt_pos ) const override;
};

}

#endif
