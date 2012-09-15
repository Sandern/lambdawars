#include "cbase.h"
#include "nb_select_level_entry.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_bitmapbutton.h"
#include "KeyValues.h"
#include "missionchooser/iasw_mission_chooser.h"
#include "missionchooser/iasw_mission_chooser_source.h"
#include "filesystem.h"
#include "nb_select_level_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CNB_Select_Level_Entry::CNB_Select_Level_Entry( vgui::Panel *parent, const char *name, const char *levelname ) : BaseClass( parent, name )
{
	// == MANAGED_MEMBER_CREATION_START: Do not edit by hand ==
	m_pImage = new CBitmapButton( this, "Image", "" );
	m_pName = new vgui::Label( this, "Name", "" );
	// == MANAGED_MEMBER_CREATION_END ==

	m_pImage->AddActionSignalTarget( this );
	m_pImage->SetCommand( "LevelClicked" );

	m_pData = NULL;
	SetMap( levelname );
}

CNB_Select_Level_Entry::~CNB_Select_Level_Entry()
{
	if( m_pData ) 
	{
		m_pData->deleteThis();
		m_pData = NULL;
	}
}

void CNB_Select_Level_Entry::SetMap( const char *levelname )
{
	if( m_pData )
	{
		m_pData->deleteThis();
		m_pData = NULL;
	}

	m_szLevelName[0] = 0;
	Q_strncpy( m_szLevelName, levelname, 256 );

	char path[256];
	Q_snprintf( path, 256, "maps/%s.res", m_szLevelName );
	m_pData = new KeyValues("Data");
	if( m_pData )
		m_pData->LoadFromFile( filesystem, path );
}

void CNB_Select_Level_Entry::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	LoadControlSettings( "resource/ui/nb_sdk_select_level_entry.res" );
}

void CNB_Select_Level_Entry::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CNB_Select_Level_Entry::OnThink()
{
	BaseClass::OnThink();

	color32 white;
	white.r = 255;
	white.g = 255;
	white.b = 255;
	white.a = 255;

	if( m_pData )
	{
		m_pName->SetText( m_pData->GetString("shortdesc", m_szLevelName) );

		const char *szImage = m_pData->GetString("image");
		if (szImage[0] == '\0')
		{
			m_pImage->SetImage( CBitmapButton::BUTTON_ENABLED, "vgui/swarm/MissionPics/UnknownMissionPic", white );
			m_pImage->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "vgui/swarm/MissionPics/UnknownMissionPic", white );
			m_pImage->SetImage( CBitmapButton::BUTTON_PRESSED, "vgui/swarm/MissionPics/UnknownMissionPic", white );
		}
		else
		{
			char imagename[MAX_PATH];
			Q_snprintf( imagename, sizeof(imagename), "vgui/%s", szImage );
			m_pImage->SetImage( CBitmapButton::BUTTON_ENABLED, imagename, white );
			m_pImage->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, imagename, white );
			m_pImage->SetImage( CBitmapButton::BUTTON_PRESSED, imagename, white );
		}
	}
	else
	{
		m_pName->SetText( m_szLevelName );
		const char *imagename = "vgui/swarm/MissionPics/UnknownMissionPic";
		m_pImage->SetImage( CBitmapButton::BUTTON_ENABLED, imagename, white );
		m_pImage->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, imagename, white );
		m_pImage->SetImage( CBitmapButton::BUTTON_PRESSED, imagename, white );
	}
	m_pImage->SetVisible(true);
}

void CNB_Select_Level_Entry::OnCommand( const char *command )
{
	if ( !Q_stricmp( "LevelClicked", command ) )
	{
		CNB_Select_Level_Panel *pPanel = dynamic_cast<CNB_Select_Level_Panel*>( GetParent()->GetParent()->GetParent() );
		if ( pPanel )
		{
			pPanel->LevelSelected( m_szLevelName );
		}
	}
}