// This file has been generated by Py++.

#include "cbase.h"
// This file has been generated by Py++.

#include "cbase.h"
#include "npcevent.h"
#include "srcpy_entities.h"
#include "bone_setup.h"
#include "baseprojectile.h"
#include "basegrenade_shared.h"
#include "takedamageinfo.h"
#include "c_ai_basenpc.h"
#include "c_basetoggle.h"
#include "c_triggers.h"
#include "soundinfo.h"
#include "saverestore.h"
#include "saverestoretypes.h"
#include "vcollide_parse.h"
#include "iclientvehicle.h"
#include "steam/steamclientpublic.h"
#include "view_shared.h"
#include "c_playerresource.h"
#include "c_breakableprop.h"
#include "nav_area.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "c_smoke_trail.h"
#include "beam_shared.h"
#include "c_hl2wars_player.h"
#include "unit_base_shared.h"
#include "wars_func_unit.h"
#include "hl2wars_player_shared.h"
#include "wars_mapboundary.h"
#include "srcpy_util.h"
#include "c_wars_weapon.h"
#include "wars_flora.h"
#include "unit_baseanimstate.h"
#include "srcpy.h"
#include "tier0/memdbgon.h"
#include "_entities_free_functions_pypp.hpp"

namespace bp = boost::python;

void _entities_register_free_functions(){

    { //::CreateEntityByName
    
        typedef ::C_BaseEntity * ( *CreateEntityByName_function_type )( char const * );
        
        bp::def( 
            "CreateEntityByName"
            , CreateEntityByName_function_type( &::CreateEntityByName )
            , ( bp::arg("className") )
            , bp::return_value_policy< bp::return_by_value >() );
    
    }

    { //::GetMapBoundaryList
    
        typedef ::C_BaseFuncMapBoundary * ( *GetMapBoundaryList_function_type )(  );
        
        bp::def( 
            "GetMapBoundaryList"
            , GetMapBoundaryList_function_type( &::GetMapBoundaryList )
            , bp::return_value_policy< bp::return_by_value >() );
    
    }

    { //::GetPlayerRelationShip
    
        typedef ::Disposition_t ( *GetPlayerRelationShip_function_type )( int,int );
        
        bp::def( 
            "GetPlayerRelationShip"
            , GetPlayerRelationShip_function_type( &::GetPlayerRelationShip )
            , ( bp::arg("p1"), bp::arg("p2") ) );
    
    }

    { //::MapUnits
    
        typedef void ( *MapUnits_function_type )( ::boost::python::api::object );
        
        bp::def( 
            "MapUnits"
            , MapUnits_function_type( &::MapUnits )
            , ( bp::arg("method") ) );
    
    }

    { //::SetPlayerRelationShip
    
        typedef void ( *SetPlayerRelationShip_function_type )( int,int,::Disposition_t );
        
        bp::def( 
            "SetPlayerRelationShip"
            , SetPlayerRelationShip_function_type( &::SetPlayerRelationShip )
            , ( bp::arg("p1"), bp::arg("p2"), bp::arg("rel") ) );
    
    }

}

