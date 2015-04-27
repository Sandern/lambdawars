#include "cbase.h"
#include "c_wars_steamstats.h"
#include "wars_achievements.h"
#include "fmtstr.h"
#include "string_t.h"
#include <vgui/ILocalize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CWars_Steamstats s_Wars_Steamstats;
CWars_Steamstats &WarsSteamStats() { return s_Wars_Steamstats; }

namespace 
{
#define FETCH_STEAM_STATS( apiname, membername ) \
	{ const char * szApiName = apiname; \
	if( !steamapicontext->SteamUserStats()->GetUserStat( playerSteamID, szApiName, &membername ) ) \
	{ \
		bOK = false; \
		Msg( "STEAMSTATS: Failed to retrieve stat %s.\n", szApiName ); \
} }

#define SEND_STEAM_STATS( apiname, membername ) \
	{ const char * szApiName = apiname; \
	steamapicontext->SteamUserStats()->SetStat( szApiName, membername ); }
};

CWars_Steamstats::CWars_Steamstats() :
	m_UserStats( 0, 0, DefLessFunc( uint64 ) )
#if !defined(NO_STEAM)
	, m_CallbackUserStatsReceived( this, &CWars_Steamstats::Steam_OnUserStatsReceived )
	, m_CallbackUserStatsStored( this, &CWars_Steamstats::Steam_OnUserStatsStored )
#endif
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WarsUserStats_t &CWars_Steamstats::GetUserStats( CSteamID &steamID )
{
	int idx = m_UserStats.Find( steamID.ConvertToUint64() );
	if( !m_UserStats.IsValidIndex( idx ) )
	{
		idx = m_UserStats.Insert( steamID.ConvertToUint64() );
	}
	return m_UserStats[idx];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWars_Steamstats::FetchStats( CSteamID playerSteamID )
{
	bool bOK = true;

	WarsUserStats_t &stats = GetUserStats( playerSteamID );

	// Fetch the player's overall stats
	FETCH_STEAM_STATS( "games_total", stats.games_total );
	FETCH_STEAM_STATS( "annihilation_wins", stats.annihilation_wins );
	FETCH_STEAM_STATS( "annihilation_games", stats.annihilation_games );
	FETCH_STEAM_STATS( "destroyhq_wins", stats.destroyhq_wins );
	FETCH_STEAM_STATS( "destroyhq_games", stats.destroyhq_games );
	
	return bOK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWars_Steamstats::StoreStats()
{
	if( !steamapicontext )
		return;

	CSteamID playerSteamID = steamapicontext->SteamUser()->GetSteamID();

	WarsUserStats_t &stats = GetUserStats( playerSteamID );

	// Send player's overall stats
	SEND_STEAM_STATS( "games_total", stats.games_total );
	SEND_STEAM_STATS( "annihilation_wins", stats.annihilation_wins );
	SEND_STEAM_STATS( "annihilation_games", stats.annihilation_games );
	SEND_STEAM_STATS( "destroyhq_wins", stats.destroyhq_wins );
	SEND_STEAM_STATS( "destroyhq_games", stats.destroyhq_games );

	g_WarsAchievementMgr.UploadUserData( STEAM_PLAYER_SLOT );
}

//-----------------------------------------------------------------------------
// Purpose: called when stat download is complete
//-----------------------------------------------------------------------------
void CWars_Steamstats::Steam_OnUserStatsReceived( UserStatsReceived_t *pUserStatsReceived )
{
	Assert( steamapicontext->SteamUserStats() );
	if ( !steamapicontext->SteamUserStats() )
		return;

	CSteamID steamID = pUserStatsReceived->m_steamIDUser;
	WarsUserStats_t &stats = GetUserStats( steamID );

	if ( pUserStatsReceived->m_eResult != k_EResultOK )
	{
		//Msg( "CWars_Steamstats: failed to download stats from Steam, EResult %d\n", pUserStatsReceived->m_eResult );
		//Msg( " m_nGameID = %I64u\n", pUserStatsReceived->m_nGameID );
		//Msg( " SteamID = %I64u\n", pUserStatsReceived->m_steamIDUser.ConvertToUint64() );
		stats.pending_steam_stats = false;
		return;
	}

	Msg( "Received user stats for %ld\n", steamID.ConvertToUint64() );

	stats.pending_steam_stats = false;

	// Fetch the rest of the steam stats here
	FetchStats( steamID );

	stats.valid = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWars_Steamstats::Steam_OnUserStatsStored( UserStatsStored_t *pUserStatsStored )
{
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();

	if ( k_EResultOK != pUserStatsStored->m_eResult )
	{
		DevMsg( "CWars_Steamstats: failed to upload stats to Steam, EResult %d\n", pUserStatsStored->m_eResult );

		// Set stats to whatever is stored on steam
		FetchStats( steamID );
	} 

	Msg("Stats were stored for user %ld\n", steamID.ConvertToUint64());
}