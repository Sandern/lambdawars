#ifndef WARS_ACHIEVEMENTS_H
#define WARS_ACHIEVEMENTS_H

#include "achievementmgr.h"

enum WarsAchievements_e 
{
	ACHIEVEMENT_WARS_GRADUATED = 0,
	ACHIEVEMENT_WARS_MISSION_RADIO_TOWER,
	ACHIEVEMENT_WARS_MISSION_ABANDONED,
	ACHIEVEMENT_WARS_MISSION_VALLEY,
	ACHIEVEMENT_WARS_ANNIHILATION_VICTORIOUS,
	ACHIEVEMENT_WARS_ANNIHILATION_KINGOFTHEHILL,
	ACHIEVEMENT_WARS_ANNIHILATION_GLADIATOR,
	ACHIEVEMENT_WARS_DESTROYHQ_VANDALISM,
	ACHIEVEMENT_WARS_DESTROYHQ_WRECKINGBALL,
	ACHIEVEMENT_WARS_THECLASH,
	ACHIEVEMENT_WARS_DOGDOG
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