//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "particles/particles.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Team Color
//-----------------------------------------------------------------------------
class C_OP_TeamColor : public CParticleOperatorInstance
{
	DECLARE_PARTICLE_OPERATOR( C_OP_TeamColor );

	virtual uint32 GetReadInitialAttributes( void ) const
	{
		Msg("C_OP_TeamColor::GetReadInitialAttributes\n");
		return 0; // PARTICLE_ATTRIBUTE_TINT_RGB_MASK;
	}

	virtual uint32 GetWrittenAttributes( void ) const
	{
		Msg("C_OP_TeamColor::GetWrittenAttributes\n");
		return 0; //PARTICLE_ATTRIBUTE_TINT_RGB_MASK | ATTRIBUTES_WHICH_ARE_ROTATION;
	}

	virtual uint32 GetReadAttributes( void ) const
	{
		Msg("C_OP_TeamColor::GetReadAttributes\n");
		return 0; // PARTICLE_ATTRIBUTE_CREATION_TIME_MASK | PARTICLE_ATTRIBUTE_LIFE_DURATION_MASK;
	}

	virtual void InitParams( CParticleSystemDefinition *pDef )
	{
		m_flTeamColor[0] = m_TeamColor[0] / 255.0f;
		m_flTeamColor[1] = m_TeamColor[1] / 255.0f;
		m_flTeamColor[2] = m_TeamColor[2] / 255.0f;
	}

	virtual void Operate( CParticleCollection *pParticles, float flOpStrength, void *pContext ) const;

	Color	m_TeamColor;
	float	m_flTeamColor[3];
};



void C_OP_TeamColor::Operate( CParticleCollection *pParticles, float flStrength,  void *pContext ) const
{

}

class ParticleOpTestDef : public CParticleOperatorDefinition<C_OP_TeamColor>
{
public:
	ParticleOpTestDef( const char *pFactoryName, ParticleOperatorId_t id, bool bIsObsolete ) : CParticleOperatorDefinition<C_OP_TeamColor>( pFactoryName, id, bIsObsolete )
	{

	}

#if 0
	virtual CParticleOperatorInstance *CreateInstance( const DmObjectId_t &id ) const
	{
		CParticleOperatorInstance *pOp = CParticleOperatorDefinition<C_OP_TeamColor>::CreateInstance( id );
		pOp->SetDefinition( this, id );
		Msg("Created C_OP_TeamColor: %d\n" ,pOp );
		return pOp;
	}
#endif // 0

	virtual const DmxElementUnpackStructure_t* GetUnpackStructure() const
	{
		Msg("C_OP_TeamColor GetUnpackStructure\n");
		return CParticleOperatorDefinition<C_OP_TeamColor>::GetUnpackStructure();
	}

	// Editor won't display obsolete operators
	virtual bool IsObsolete() const 
	{ 
		Msg("C_OP_TeamColor IsObsolete\n");
		return CParticleOperatorDefinition<C_OP_TeamColor>::IsObsolete(); 
	}

	virtual uint32 GetFilter() const 
	{ 
		Msg("C_OP_TeamColor GetFilter\n");
		return CParticleOperatorDefinition<C_OP_TeamColor>::GetFilter();
	}
};

DEFINE_PARTICLE_OPERATOR( C_OP_TeamColor, "Color Team", OPERATOR_GENERIC );
//static ParticleOpTestDef s_C_OP_TeamColorFactory( "Color Team", OPERATOR_GENERIC, false );

BEGIN_PARTICLE_OPERATOR_UNPACK( C_OP_TeamColor ) 
	DMXELEMENT_UNPACK_FIELD( "team_color", "255 255 255 255", Color, m_TeamColor )
END_PARTICLE_OPERATOR_UNPACK( C_OP_TeamColor )

void AddWarsParticleOperators( void )
{
	REGISTER_PARTICLE_OPERATOR( FUNCTION_OPERATOR, C_OP_TeamColor );
}

#if 0
CON_COMMAND_F( wars_particle_print_ops, "test", FCVAR_CHEAT )
{
	for( int i = 0; i < PARTICLE_FUNCTION_COUNT; i++ )
	{
		CUtlVector< IParticleOperatorDefinition *> &definitions = g_pParticleSystemMgr->GetAvailableParticleOperatorList( (ParticleFunctionType_t)i );

		for( int j = 0; j < definitions.Count(); j++ )
		{
			Msg("%d: definition %d: %s, obsolete: %d\n", i, j, definitions[j]->GetName(), definitions[j]->IsObsolete() );
		}
	}
}

CON_COMMAND_F( wars_particle_add_ops, "test", FCVAR_CHEAT )
{
	AddWarsParticleOperators();
}

#include <vgui_controls/Panel.h>
#include "EngineInterface.h"
#include "IGameUIFuncs.h"
#include "IEngineVGUI.h"
#include "vgui/IPanel.h"
#include "vgui/IVGui.h"
#include "vgui_int.h"

static vgui::VPANEL FindPanelByName( vgui::VPANEL parent, const char *pName )
{
	for( int i = 0; i < vgui::ipanel()->GetChildCount( parent ); i++ )
	{
		vgui::VPANEL child = vgui::ipanel()->GetChild( parent, i );
		const char *pPanelName = vgui::ipanel()->GetName( child );
		Msg("Testing: %s\n", pPanelName );
		if( V_strcmp( pName, pPanelName ) == 0 )
		{
			return child;
		}

		vgui::VPANEL testPanel = FindPanelByName( child, pName );
		if( testPanel != vgui::INVALID_PANEL )
			return testPanel;
	}

	return vgui::INVALID_PANEL;
}

CON_COMMAND_F( wars_particle_reload_panel, "test", FCVAR_CHEAT )
{
	vgui::VPANEL clientDllPanel = enginevguifuncs->GetPanel( PANEL_GAMEDLL );
	vgui::VPANEL target = FindPanelByName( clientDllPanel, "ParticleSystemPropertiesPanel" );
	Msg("Panel: %d\n" , target );
	vgui::ipanel()->PerformApplySchemeSettings( target );
	vgui::Panel *pPanel = vgui::ipanel()->GetPanel( target, "pet" );
	Msg("Panel 2: %d\n" , pPanel );
	//particlesystempropertiespanel
}
#endif // 0