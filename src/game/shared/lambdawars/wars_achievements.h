#ifndef WARS_ACHIEVEMENTS_H
#define WARS_ACHIEVEMENTS_H

#include "achievementmgr.h"

enum WarsAchievements_e 
{
	ACHIEVEMENT_WARS_TEST = 0,
	ACHIEVEMENT_WARS_MISSION_RADIO_TOWER,
	ACHIEVEMENT_WARS_MISSION_ABANDONED,
	ACHIEVEMENT_WARS_MISSION_VALLEY
};

#ifdef CLIENT_DLL

class CWarsAchievement;

class CWarsAchievementMgr : public CAchievementMgr
{
public:
	typedef CAchievementMgr BaseClass;

	CWarsAchievementMgr();

	virtual bool Init();
	virtual void LevelInitPreEntity();
	virtual void FireGameEvent( IGameEvent *event );
	virtual void Shutdown();
};

// base class for all Lambda Wars achievements

class CWarsAchievement : public CBaseAchievement
{	
public:
	typedef CBaseAchievement BaseClass;

	CWarsAchievement();
};

extern CWarsAchievementMgr g_WarsAchievementMgr;	// global achievement manager for Lambda Wars

#endif

#endif // WARS_ACHIEVEMENTS_H