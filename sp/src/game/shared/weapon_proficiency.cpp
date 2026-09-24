//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "weapon_proficiency.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#ifndef USE_NEW_PROFICIENCY_SYSTEM
static const char *g_ProficiencyNames[] = 
{
	"Poor",
	"Average",
	"Good",
	"Very Good",
	"Perfect"
};

const char *GetWeaponProficiencyName( WeaponProficiency_t proficiency )
{
	COMPILE_TIME_ASSERT( ARRAYSIZE(g_ProficiencyNames) == WEAPON_PROFICIENCY_PERFECT + 1 ); // Hey, update that there table!

	if ( proficiency < 0 || proficiency > WEAPON_PROFICIENCY_PERFECT )
		return "<<Invalid>>";
	return g_ProficiencyNames[proficiency];
}

#else

static const char* g_ProficiencyTableValueNames[PROF_LAST_VALUE] =
{
	"min_burst_time",
	"max_burst_time",
	"min_shot_count",
	"max_shot_count",
	"firerate_scale",
	"spread_scale",
	"spread_bias"
};

static const char* g_ProficiencyTableTierNames[PROF_MAX_SKILL] =
{
	"tier_1",
	"tier_2",
	"tier_3"
};

static const char* g_ProficiencyTableSkillNames[PROF_MAX_SKILL] =
{
	"easy",
	"normal",
	"hard"
};

static const char* g_ProficiencyTableNpcTypeNames[PROF_NPC_TYPES] =
{
	"ally",
	"enemy"
};


//=============================================================================
// ProficiencyTable_t:
//=============================================================================

ConVar hl2r_debug_weapon_proficiency("hl2r_debug_weapon_proficiency", "0");
float ProficiencyTable_t::GetValue(ProficiencyTableIndex_t index)
{
	float flValue = m_flValues[index.valuetype][index.tier][index.difficulty - 1][index.npctype - 1];
	if (hl2r_debug_weapon_proficiency.GetBool())
	{
		DevMsg("Getting value for Proficiency value: [%s]\n", g_ProficiencyTableValueNames[index.valuetype]);
		DevMsg("	Value tier is: [%s]\n", g_ProficiencyTableTierNames[index.tier - 1]);
		DevMsg("	Value difficulty is: [%s]\n", g_ProficiencyTableSkillNames[index.difficulty - 1]);
		DevMsg("	Value npctype is: [%s]\n\n", g_ProficiencyTableNpcTypeNames[index.npctype - 1]);

		DevMsg("	Value is: [%f]\n\n", flValue);
	}

	return flValue;
}

void ProficiencyTable_t::SetValue(ProficiencyTableIndex_t* index, float newvalue)
{
	if (!index->ValidForSetValue())
		return;

	// I would like to make this more optimized but idk there's only 108 values in total, so it shouldn't be too bad?
	for (int i = 0; i < PROF_MAX_TIER; i++)
		for (int j = 0; j < PROF_MAX_SKILL; j++)
			for (int k = 0; k < PROF_NPC_TYPES; k++)
			{
				m_flValues[index->valuetype]
					[index->tier - 1 < 0 ? i : index->tier - 1]
					[index->difficulty - 1 < 0 ? j : index->difficulty - 1]
					[index->npctype - 1 < 0 ? k : index->npctype - 1]
					= newvalue;
			}
}

void ProficiencyTable_t::Debug( void )
{
	for (int h = 0; h < PROF_LAST_VALUE - 1; h++)
	{
		DevMsg("	ProficiencyTable_t valuetype is: [%s]\n", g_ProficiencyTableValueNames[h] );
		for (int i = 0; i < PROF_MAX_TIER; i++)
		{
			DevMsg("		ProficiencyTable_t tier is: [%s]\n", g_ProficiencyTableTierNames[i] );
			for (int j = 0; j < PROF_MAX_SKILL; j++)
			{
				DevMsg("			ProficiencyTable_t difficulty is: [%s]\n", g_ProficiencyTableSkillNames[j] );
				for (int k = 0; k < PROF_NPC_TYPES; k++)
				{
					DevMsg("				ProficiencyTable_t npctype is: [%s]\n", g_ProficiencyTableNpcTypeNames[k] );
					DevMsg("					ProficiencyTable_t value is: [%f]\n", m_flValues[h][i][j][k]);
				}
				DevMsg("\n");
			}
			DevMsg("\n");
		}
		DevMsg("\n\n");
	}
}

//=============================================================================
// ProficiencyTableIndex_t:
//=============================================================================

void ProficiencyTableIndex_t::Initialize()
{
	valuetype = -1;
	tier = -1;
	difficulty = -1;
	npctype = -1;
}

void ProficiencyTableIndex_t::Copy(ProficiencyTableIndex_t* newindex)
{
	valuetype = newindex->difficulty;
	tier = newindex->tier;
	difficulty = newindex->difficulty;
	npctype = newindex->npctype;
}

bool ProficiencyTableIndex_t::ValidForGetValue()
{
	if (valuetype < 0 || valuetype > PROF_LAST_VALUE - 1)
	{
		Error("ProficiencyTableIndex_t has invalid [valuetype]!");
		return false;
	}

	if (tier < 1 || tier > PROF_MAX_TIER)
	{
		Error("ProficiencyTableIndex_t has invalid [tier] value!");
		return false;
	}

	if (difficulty < 1 || difficulty > PROF_MAX_SKILL)
	{
		Error("ProficiencyTableIndex_t has invalid [difficulty] value!");
		return false;
	}

	if (npctype < 1 || npctype > PROF_NPC_TYPES)
	{
		Error("ProficiencyTableIndex_t has invalid [npctype] value!");
		return false;
	}

	return true;
}

bool ProficiencyTableIndex_t::ValidForSetValue()
{
	if (valuetype < 0 || valuetype > PROF_LAST_VALUE - 1)
	{
		Error("ProficiencyTableIndex_t has invalid [valuetype]!");
		return false;
	}

	return true;
}

#endif