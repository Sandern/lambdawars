//====== Copyright © 20xx, Sander Corporation, All rights reserved. =======//
//
// Purpose: Team color code
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef COMMON_TEAMCOLOR_H_
#define COMMON_TEAMCOLOR_H_

float4 ApplyTeamColor( sampler TeamColorSampler, float4 TeamColor, float2 tc, float4 incolor )
{
	float4 result;
	float a = tex2D( TeamColorSampler, tc ).r;
	result = TextureCombine( incolor, TeamColor, TCOMBINE_MULTIPLY, a );
	result.a = incolor.a;
	return result;
}

#endif // COMMON_TEAMCOLOR_H_