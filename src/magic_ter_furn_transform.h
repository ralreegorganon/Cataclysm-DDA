#pragma once
#ifndef MAGIC_TER_FURN_TRANSFORM_H
#define MAGIC_TER_FURN_TRANSFORM_H

#include <map>
#include <vector>

#include "optional.h"
#include "type_id.h"
#include "weighted_list.h"

class Creature;
class JsonObject;
struct tripoint;

class ter_furn_transform_def
{
    private:
        weighted_int_list<ter_str_id> result_terrains;
        weighted_int_list<furn_str_id> result_furnitures;
        weighted_int_list<std::string> result_nested_mapgens;

        std::vector<ter_str_id> match_terrains;
        std::vector<furn_str_id> match_furnitures;

        std::vector<std::string> match_terrain_flags;
        std::vector<std::string> match_furniture_flags;

        std::string message;
        bool message_good;

    public:
        bool was_loaded = false;
        void check( const std::string &name ) const;
        void deserialize( const JsonObject &jo );
        bool is_match( const ter_id &ter, const furn_id &furn ) const;
        void apply( map &m, const tripoint &location ) const;
        void add_message( Creature &caster ) const;
};


class ter_furn_transform
{
    private:
        std::string fail_message;
        std::vector<ter_furn_transform_def> transforms;

        bool transform( map &m, const tripoint &location,
                        std::function<void( const ter_furn_transform_def & )> success_callback ) const;

    public:
        ter_furn_transform_id id;
        bool was_loaded = false;

        void transform( map &m, const tripoint &location ) const;
        void transform_with_messages( map &m, const tripoint &location, Creature &caster ) const;

        void check() const;
        void load( const JsonObject &jo, const std::string & );

        static void load_transform( const JsonObject &jo, const std::string &src );
        static void check_consistency();
        static const std::vector<ter_furn_transform> &get_all();
        static void reset_all();
        bool is_valid() const;
};

#endif
