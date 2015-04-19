#include "cbase.h"
#include "wars_achievements.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
CWarsAchievementMgr g_WarsAchievementMgr;	// global achievement manager for Lambda Wars
CWarsAchievementMgr* WarsAchievementManager() { return &g_WarsAchievementMgr; }

CWarsAchievementMgr::CWarsAchievementMgr()
{

}


bool CWarsAchievementMgr::Init()
{
	return BaseClass::Init();
}

void CWarsAchievementMgr::Shutdown()
{
	BaseClass::Shutdown();
}

void CWarsAchievementMgr::LevelInitPreEntity()
{
	BaseClass::LevelInitPreEntity();
}

void CWarsAchievementMgr::FireGameEvent( IGameEvent *event )
{
	//const char *name = event->GetName();
	//if ( !V_strcmp( name, "some_event" ) )
	//{
	//}
	//else
	{
		BaseClass::FireGameEvent( event );
	}
}

// =====================

CWarsAchievement::CWarsAchievement()
{
}

// =====================

class CAchievement_Test : public CWarsAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT_ORDER( CAchievement_Test, ACHIEVEMENT_WARS_TEST, "WARS_TEST", 5, 1 );

class CAchievement_MissionRadioTower : public CWarsAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT_ORDER( CAchievement_MissionRadioTower, ACHIEVEMENT_WARS_MISSION_RADIO_TOWER, "WARS_MISSION_RADIO_TOWER", 5, 1 );

class CAchievement_MissionAbandoned : public CWarsAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT_ORDER( CAchievement_MissionAbandoned, ACHIEVEMENT_WARS_MISSION_ABANDONED, "WARS_MISSION_ABANDONED", 5, 1 );

class CAchievement_MissionValley : public CWarsAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT_ORDER( CAchievement_MissionValley, ACHIEVEMENT_WARS_MISSION_VALLEY, "WARS_MISSION_VALLEY", 5, 1 );

#endif