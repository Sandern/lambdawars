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
DECLARE_ACHIEVEMENT_ORDER( CWarsAchievement, ACHIEVEMENT_WARS_TEST, "WARS_TEST", 5, 1 );

#endif