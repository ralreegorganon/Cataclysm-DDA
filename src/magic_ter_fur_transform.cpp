#include "magic_ter_furn_transform.h"

#include "coordinate_conversions.h"
#include "creature.h"
#include "point.h"
#include "game.h"
#include "generic_factory.h"
#include "magic.h"
#include "map.h"
#include "mapdata.h"
#include "mapgendata.h"
#include "messages.h"
#include "type_id.h"

namespace
{
generic_factory<ter_furn_transform> ter_furn_transform_factory( "ter_furn_transform" );
} // namespace

template<>
const ter_furn_transform &string_id<ter_furn_transform>::obj() const
{
    return ter_furn_transform_factory.obj( *this );
}

template<>
bool string_id<ter_furn_transform>::is_valid() const
{
    return ter_furn_transform_factory.is_valid( *this );
}

void ter_furn_transform::load_transform( const JsonObject &jo, const std::string &src )
{
    ter_furn_transform_factory.load( jo, src );
}

void ter_furn_transform::check_consistency()
{
    ter_furn_transform_factory.check();
}

void ter_furn_transform::reset_all()
{
    ter_furn_transform_factory.reset();
}

bool ter_furn_transform::is_valid() const
{
    return ter_furn_transform_factory.is_valid( this->id );
}

const std::vector<ter_furn_transform> &ter_furn_transform::get_all()
{
    return ter_furn_transform_factory.get_all();
}

template<class T>
static void load_transform_results( const JsonObject &jsi, const std::string &json_key,
                                    weighted_int_list<T> &list )
{
    if( jsi.has_string( json_key ) ) {
        list.add( T( jsi.get_string( json_key ) ), 1 );
        return;
    }
    for( const JsonValue entry : jsi.get_array( json_key ) ) {
        if( entry.test_array() ) {
            JsonArray inner = entry.get_array();
            list.add( T( inner.get_string( 0 ) ), inner.get_int( 1 ) );
        } else {
            list.add( T( entry.get_string() ), 1 );
        }
    }
}

extern std::map<std::string, std::vector<std::unique_ptr<mapgen_function_json_nested>>>
nested_mapgen;

void ter_furn_transform_def::deserialize( const JsonObject &jo )
{
    load_transform_results( jo, "result_terrain", result_terrains );
    load_transform_results( jo, "result_furniture", result_furnitures );
    load_transform_results( jo, "result_nested_mapgen", result_nested_mapgens );

    optional( jo, was_loaded, "message", message, "" );
    optional( jo, was_loaded, "message_good", message_good, true );

    optional( jo, was_loaded, "match_terrain", match_terrains );
    optional( jo, was_loaded, "match_furniture", match_furnitures );
    optional( jo, was_loaded, "match_terrain_flags", match_terrain_flags );
    optional( jo, was_loaded, "match_furniture_flags", match_furniture_flags );
}

void ter_furn_transform_def::check( const std::string &name ) const
{
    if( result_terrains.empty() && result_furnitures.empty() && result_nested_mapgens.empty() ) {
        debugmsg( "ter_furn_transform %s has a transformation with no results defined", name );
    }

    if( match_terrains.empty() && match_furnitures.empty() && match_terrain_flags.empty() &&
        match_furniture_flags.empty() ) {
        debugmsg( "ter_furn_transform %s has a transformation with no match criteria defined", name );
    }
}

bool ter_furn_transform_def::is_match( const ter_id &ter, const furn_id &furn ) const
{
    if( !match_terrains.empty() &&
        std::find( match_terrains.begin(), match_terrains.end(), ter.id() ) == match_terrains.end() ) {
        return false;
    }

    if( !match_furnitures.empty() &&
        std::find( match_furnitures.begin(), match_furnitures.end(),
                   furn.id() ) == match_furnitures.end() ) {
        return false;
    }

    if( !match_terrain_flags.empty() &&
        !std::any_of( match_terrain_flags.begin(),
    match_terrain_flags.end(), [&ter]( const std::string & flag ) {
    return ter->has_flag( flag );
    } ) ) {
        return false;
    }

    if( !match_furniture_flags.empty() &&
        !std::any_of( match_furniture_flags.begin(),
    match_furniture_flags.end(), [&furn]( const std::string & flag ) {
    return furn->has_flag( flag );
    } ) ) {
        return false;
    }

    return true;
}

void ter_furn_transform_def::apply( map &m, const tripoint &location ) const
{
    const ter_str_id *ter_result = result_terrains.pick();
    if( ter_result != nullptr ) {
        m.ter_set( location, ter_result->id() );
    }

    const furn_str_id *furn_result = result_furnitures.pick();
    if( furn_result != nullptr ) {
        m.furn_set( location, furn_result->id() );
    }

    const std::string *nested_mapgen_result = result_nested_mapgens.pick();
    if( nested_mapgen_result != nullptr ) {
        const auto iter = nested_mapgen.find( *nested_mapgen_result );
        const auto &ptr = random_entry_ref( iter->second );

        const tripoint abs_ms = m.getabs( location );
        const tripoint abs_omt = ms_to_omt_copy( abs_ms );
        const tripoint abs_sub = ms_to_sm_copy(abs_ms);

        tinymap target_map;
        target_map.load(abs_sub, true);
        const tripoint local_ms = target_map.getlocal(abs_ms);
        mapgendata md( abs_omt, target_map, 0.0f, calendar::start_of_cataclysm, nullptr );
        ptr->nest( md, local_ms.xy() );
        target_map.save();
    }
}

void ter_furn_transform_def::add_message( Creature &caster ) const
{
    caster.add_msg_if_player( message_good ? m_good : m_bad, message );
}

void ter_furn_transform::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "transforms", transforms );
    optional( jo, was_loaded, "fail_message", fail_message, "" );
}

void ter_furn_transform::check() const
{
    if( transforms.empty() ) {
        debugmsg( "ter_furn_transform %s has no transformations defined", id.str() );
    }

    for( const ter_furn_transform_def &t : transforms ) {
        t.check( id.str() );
    }
}

bool ter_furn_transform::transform( map &m, const tripoint &location,
                                    std::function<void( const ter_furn_transform_def & )> success_callback ) const
{
    const ter_id ter_at_loc = m.ter( location );
    const furn_id furn_at_loc = m.furn( location );
    bool success = false;

    for( const ter_furn_transform_def &t : transforms ) {
        if( !t.is_match( ter_at_loc, furn_at_loc ) ) {
            continue;
        }
        success = true;
        t.apply( m, location );
        success_callback( t );
    }

    return success;
}

void ter_furn_transform::transform( map &m, const tripoint &location ) const
{
    transform( m, location, []( const ter_furn_transform_def & ) {} );
}

void ter_furn_transform::transform_with_messages( map &m, const tripoint &location,
        Creature &caster ) const
{
    const bool caster_sees_location = caster.sees( location );

    bool success = false;
    if( caster_sees_location ) {
        success = transform( m, location, [&caster]( const ter_furn_transform_def & t ) {
            t.add_message( caster );
        } );
    } else {
        success = transform( m, location, []( const ter_furn_transform_def & ) {} );
    }

    if( !success ) {
        caster.add_msg_if_player( m_bad, fail_message );
    }
}
