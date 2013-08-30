#ifndef CASCADE_T_H
#define CASCADE_T_H

#include "deferred/deferred_shared_common.h"

struct cascade_t
{
	int iResolution;

	float flProjectionSize;
	float flOriginOffset;
	float flFarZ;

	float flSlopeScaleMin;
	float flSlopeScaleMax;
	float flNormalScaleMax;

	float flUpdateDelay;
	bool bOutputRadiosityData;
	int iRadiosityCascadeTarget;

#if CSM_USE_COMPOSITED_TARGET
	int iViewport_x;
	int iViewport_y;
#endif
};

const cascade_t &GetCascadeInfo( int index );


#endif