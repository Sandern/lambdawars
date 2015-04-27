#ifndef WARS_STEAMSTATS_H
#define WARS_STEAMSTATS_H

#include "steam/steam_api.h"
#include "platform.h"

typedef struct WarsUserStats_t
{
	WarsUserStats_t() : pending_steam_stats(false), valid(false) {}

	bool valid;
	bool pending_steam_stats;

	// Stats
	int32 games_total;
	int32 annihilation_wins;
	int32 annihilation_games;
	int32 destroyhq_wins;
	int32 destroyhq_games;
} WarsUserStat_t;

class CWars_Steamstats
{
public:
	CWars_Steamstats();

	WarsUserStats_t &GetUserStats( CSteamID &steamID );

	// Fetch the client's steamstats
	bool FetchStats( CSteamID playerSteamID );

	// Send the client's stats off to steam
	void StoreStats(); 

#if !defined(NO_STEAM)
	STEAM_CALLBACK( CWars_Steamstats, Steam_OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived );
	STEAM_CALLBACK( CWars_Steamstats, Steam_OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored );
#endif	

private:
	CUtlMap< uint64, WarsUserStats_t > m_UserStats;
	
};

CWars_Steamstats &WarsSteamStats();

#endif // #ifndef WARS_STEAMSTATS_H