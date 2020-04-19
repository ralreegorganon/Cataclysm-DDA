#include "event_field_transformations.h"

#include <set>

#include "mapdata.h"
#include "mtype.h"
#include "string_id.h"
#include "type_id.h"

static std::vector<cata_variant> species_of_monster( const cata_variant &v )
{
    const std::set<species_id> &species = v.get<mtype_id>()->species;
    std::vector<cata_variant> result;
    result.reserve( species.size() );
    for( const species_id &s : species ) {
        result.push_back( cata_variant( s ) );
    }
    return result;
}

static std::vector<cata_variant> terrain_flag( const cata_variant &v )
{
    const std::set<std::string> &flags = v.get<ter_id>()->get_flags();
    std::vector<cata_variant> result;
    result.reserve( flags.size() );
    for( const std::string &s : flags ) {
        result.push_back( cata_variant( s ) );
    }
    return result;
}

static std::vector<cata_variant> mount_to_mounted( const cata_variant &v )
{
    const mtype_id mount = v.get<mtype_id>();
    std::vector<cata_variant> result = { cata_variant( mount == mtype_id::NULL_ID() )};
    return result;
}

static std::vector<cata_variant> terrain_to_swimming( const cata_variant &v )
{
    const ter_id ter = v.get<ter_id>();
    const bool swimming = ter->has_flag( ter_bitflags::TFLAG_DEEP_WATER ) &&
                          ter->has_flag( ter_bitflags::TFLAG_SWIMMABLE );
    std::vector<cata_variant> result = { cata_variant( swimming ) };
    return result;
}

const std::unordered_map<std::string, EventFieldTransformation> event_field_transformations = {
    { "species_of_monster", species_of_monster },
    { "terrain_flag", terrain_flag },
    { "mount_to_mounted", mount_to_mounted },
    { "terrain_to_swimming", terrain_to_swimming },
};
