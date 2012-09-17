//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hl2wars_scoreboard.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "hl2wars_gamerules.h"
#include "hl2wars_backgroundpanel.h"

#include <KeyValues.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVgui.h>
#include <vgui_controls/SectionedListPanel.h>

#include "src_python.h"

using namespace vgui;

#define TEAM_MAXCOUNT			5

// id's of sections used in the scoreboard
enum EScoreboardSections
{
	SCORESECTION_FREEFORALL = 1, 
	SCORESECTION_SPECTATOR,
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHL2WarsScoreboard::CHL2WarsScoreboard(IViewPort *pViewPort) : CClientScoreBoardDialog(pViewPort)
{
	// load the new scheme early!!
	SetScheme("SourceScheme");

	SetPaintBackgroundType( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHL2WarsScoreboard::~CHL2WarsScoreboard()
{
}

//-----------------------------------------------------------------------------
// Purpose: Instead of centering, place it a bit more up
//-----------------------------------------------------------------------------
void CHL2WarsScoreboard::Update()
{
	BaseClass::Update();

	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds(wx, wy, ww, wt);
	SetPos((ww - GetWide()) / 2, (wt - GetTall()) / 3);
}

//-----------------------------------------------------------------------------
// Purpose: Paint background with rounded corners
//-----------------------------------------------------------------------------
void CHL2WarsScoreboard::PaintBackground()
{
	m_pPlayerList->SetBgColor( Color(0, 0, 0, 0) );
	m_pPlayerList->SetBorder(NULL);

	int wide, tall;
	GetSize( wide, tall );

	DrawRoundedBackground( m_bgColor, wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: Paint border with rounded corners
//-----------------------------------------------------------------------------
void CHL2WarsScoreboard::PaintBorder()
{
	int wide, tall;
	GetSize( wide, tall );

	//surface()->DrawSetColor(m_borderColor);
	//surface()->DrawSetTextColor(m_borderColor);
	//surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );
	//DrawRoundedBorder( m_borderColor, wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: Apply scheme settings
//-----------------------------------------------------------------------------
void CHL2WarsScoreboard::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_bgColor = GetSchemeColor("SectionedListPanel.BgColor", GetBgColor(), pScheme);
	m_borderColor = pScheme->GetColor( "FgColor", Color( 0, 0, 0, 0 ) );

	SetBgColor( m_borderColor );
	//SetBgColor( Color(0, 0, 0, 0) );
	SetBorder( pScheme->GetBorder( "BaseBorder" ) );

	m_hPlayerFont = pScheme->GetFont("MenuTitle2");
}


//-----------------------------------------------------------------------------
// Purpose: sets up base sections
//-----------------------------------------------------------------------------
void CHL2WarsScoreboard::InitScoreboardSections()
{
	m_pPlayerList->SetBgColor( Color(0, 0, 0, 0) );
	m_pPlayerList->SetBorder(NULL);

	// fill out the structure of the scoreboard
	AddHeader();

	m_iSectionId = GetSectionFromTeamNumber( TYPE_TEAM );
	AddSection( TYPE_TEAM, TEAM_UNASSIGNED );
	m_iSectionId = GetSectionFromTeamNumber( TYPE_SPECTATORS );
	AddSection( TYPE_SPECTATORS, TEAM_SPECTATOR );

	m_pPlayerList->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: resets the scoreboard team info
//-----------------------------------------------------------------------------
void CHL2WarsScoreboard::UpdateTeamInfo()
{
	if ( GameResources() == NULL )
		return;

	int iNumPlayersInGame = 0;

	for ( int j = 1; j <= gpGlobals->maxClients; j++ )
	{	
		if ( GameResources()->IsConnected( j ) )
		{
			iNumPlayersInGame++;
		}
	}

	// update the team sections in the scoreboard
	for ( int i = TEAM_UNASSIGNED; i < TEAM_MAXCOUNT; i++ )
	{
		wchar_t *teamName = NULL;
		int sectionID = 0;
		C_Team *team = GetGlobalTeam(i);

		sectionID = GetSectionFromTeamNumber( i );
		if ( team && HL2WarsGameRules()->IsTeamplay() == true )
		{
			// update team name
			wchar_t name[64];
			wchar_t string1[1024];
			wchar_t wNumPlayers[6];

			_snwprintf(wNumPlayers, 6, L"%i", team->Get_Number_Players());

			if (!teamName && team)
			{
				g_pVGuiLocalize->ConvertANSIToUnicode(team->Get_Name(), name, sizeof(name));
				teamName = name;
			}

			if (team->Get_Number_Players() == 1)
			{
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#HL2Wars_ScoreBoard_Player"), 2, teamName, wNumPlayers );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#HL2Wars_ScoreBoard_Players"), 2, teamName, wNumPlayers );
			}

			// update stats
			wchar_t val[6];
			swprintf(val, L"%d", team->Get_Score());
			m_pPlayerList->ModifyColumn(sectionID, "frags", val);
			if (team->Get_Ping() < 1)
			{
				m_pPlayerList->ModifyColumn(sectionID, "ping", L"");
			}
			else
			{
				swprintf(val, L"%d", team->Get_Ping());
				m_pPlayerList->ModifyColumn(sectionID, "ping", val);
			}
		
			m_pPlayerList->ModifyColumn(sectionID, "name", string1);
		}
		else
		{
			// update team name
			//wchar_t name[64];
			wchar_t string1[1024];
			wchar_t wNumPlayers[6];

			_snwprintf(wNumPlayers, 6, L"%i", iNumPlayersInGame );
			//_snwprintf( name, sizeof(name), L"%s", g_pVGuiLocalize->Find("#HL2Wars_ScoreBoard_Player") );
			
			//teamName = name;

			if ( iNumPlayersInGame == 1)
			{
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#HL2Wars_ScoreBoard_Player"), 2, wNumPlayers );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#HL2Wars_ScoreBoard_Players"), 2, wNumPlayers );
			}

			m_pPlayerList->ModifyColumn(sectionID, "name", string1);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CHL2WarsScoreboard::AddHeader()
{
	// add the top header
	m_iSectionId = 0;
	m_pPlayerList->AddSection(m_iSectionId, "");
	m_pPlayerList->SetFontSection(0, m_hPlayerFont);
	m_pPlayerList->SetSectionAlwaysVisible(m_iSectionId);
	m_pPlayerList->AddColumnToSection(m_iSectionId, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iAvatarWidth );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "name", "#PlayerName", 0, scheme()->GetProportionalScaledValueEx( GetScheme(),NAME_WIDTH) );
	//m_pPlayerList->AddColumnToSection(m_iSectionId, "frags", "#PlayerScore", 0, scheme()->GetProportionalScaledValueEx( GetScheme(),SCORE_WIDTH) );
	//m_pPlayerList->AddColumnToSection(m_iSectionId, "deaths", "#PlayerDeath", 0, scheme()->GetProportionalScaledValueEx( GetScheme(),DEATH_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "ping", "#PlayerPing", 0, scheme()->GetProportionalScaledValueEx( GetScheme(),PING_WIDTH) );
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
void CHL2WarsScoreboard::AddSection(int teamType, int teamNumber)
{
	if ( teamType == TYPE_TEAM )
	{
		IGameResources *gr = GameResources();

		if ( !gr )
			return;

		// setup the team name
		wchar_t *teamName = g_pVGuiLocalize->Find( gr->GetTeamName(teamNumber) );
		wchar_t name[64];
		wchar_t string1[1024];
		
		if (!teamName)
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetTeamName(teamNumber), name, sizeof(name));
			teamName = name;
		}

		g_pVGuiLocalize->ConstructString( string1, sizeof( string1 ), g_pVGuiLocalize->Find("#Player"), 2, teamName );
		
		m_pPlayerList->AddSection(m_iSectionId, "", StaticPlayerSortFunc);

		// Avatars are always displayed at 32x32 regardless of resolution
		if ( ShowAvatars() )
		{
			m_pPlayerList->AddColumnToSection( m_iSectionId, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iAvatarWidth );
		}

		m_pPlayerList->AddColumnToSection(m_iSectionId, "name", string1, 0, scheme()->GetProportionalScaledValueEx( GetScheme(),NAME_WIDTH) );
		//m_pPlayerList->AddColumnToSection(m_iSectionId, "frags", "", 0, scheme()->GetProportionalScaledValueEx( GetScheme(),SCORE_WIDTH) );
		//m_pPlayerList->AddColumnToSection(m_iSectionId, "deaths", "", 0, scheme()->GetProportionalScaledValueEx( GetScheme(),DEATH_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "ping", "", 0, scheme()->GetProportionalScaledValueEx( GetScheme(),PING_WIDTH) );
	}
	else if ( teamType == TYPE_SPECTATORS )
	{
		m_pPlayerList->AddSection(m_iSectionId, "");

		// Avatars are always displayed at 32x32 regardless of resolution
		if ( ShowAvatars() )
		{
			m_pPlayerList->AddColumnToSection( m_iSectionId, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iAvatarWidth );
		}
		m_pPlayerList->AddColumnToSection(m_iSectionId, "name", "#Spectators", 0, scheme()->GetProportionalScaledValueEx( GetScheme(),NAME_WIDTH) );
		//m_pPlayerList->AddColumnToSection(m_iSectionId, "frags", "", 0, scheme()->GetProportionalScaledValueEx( GetScheme(),SCORE_WIDTH) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHL2WarsScoreboard::GetSectionFromTeamNumber( int teamNumber )
{
	switch ( teamNumber )
	{
	case TEAM_SPECTATOR:
		return SCORESECTION_SPECTATOR;
	default:
		return SCORESECTION_FREEFORALL;
	}
	return SCORESECTION_FREEFORALL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsScoreboard::UpdatePlayerInfo()
{
	m_iSectionId = 0; // 0'th row is a header
	int selectedRow = -1;

	// walk all the players and make sure they're in the scoreboard
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		IGameResources *gr = GameResources();

		if ( gr && gr->IsConnected( i ) )
		{
			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
			GetPlayerScoreInfo( i, playerData );
			UpdatePlayerAvatar( i, playerData );

			const char *oldName = playerData->GetString("name","");
			char newName[MAX_PLAYER_NAME_LENGTH];

			UTIL_MakeSafeName( oldName, newName, MAX_PLAYER_NAME_LENGTH );

			playerData->SetString("name", newName);

			int itemID = FindItemIDForPlayerIndex( i );
  			int sectionID = gr->GetTeam( i );
			
			if ( gr->IsLocalPlayer( i ) )
			{
				selectedRow = itemID;
			}
			if (itemID == -1)
			{
				// add a new row
				itemID = m_pPlayerList->AddItem( sectionID, playerData );
			}
			else
			{
				// modify the current row
				m_pPlayerList->ModifyItem( itemID, sectionID, playerData );
			}

			m_pPlayerList->SetItemFont( itemID, m_hPlayerFont );

			// set the row color based on the players team
			int ownernumber = g_PR->GetOwnerNumber(i);

			try	{	
				m_pPlayerList->SetItemFgColor( itemID, 
						boost::python::extract<Color>( SrcPySystem()->Get("_GetColorForOwnerNumber", "playermgr")(ownernumber) ) );
			} catch(...) {	
				m_pPlayerList->SetItemFgColor( itemID, gr->GetTeamColor( gr->GetTeam( i ) ) );
			}	

			playerData->deleteThis();
		}
		else
		{
			// remove the player
			int itemID = FindItemIDForPlayerIndex( i );
			if (itemID != -1)
			{
				m_pPlayerList->RemoveItem(itemID);
			}
		}
	}

	if ( selectedRow != -1 )
	{
		m_pPlayerList->SetSelectedItem(selectedRow);
	}
}