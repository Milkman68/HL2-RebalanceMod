//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_PROFICIENCY_H
#define WEAPON_PROFICIENCY_H

#define USE_NEW_PROFICIENCY_SYSTEM

#if defined( _WIN32 )
#pragma once
#endif

#ifndef USE_NEW_PROFICIENCY_SYSTEM
struct WeaponProficiencyInfo_t
{
	float	spreadscale;
	float	bias;
};

enum WeaponProficiency_t
{
	WEAPON_PROFICIENCY_POOR = 0,
	WEAPON_PROFICIENCY_AVERAGE,
	WEAPON_PROFICIENCY_GOOD,
	WEAPON_PROFICIENCY_VERY_GOOD,
	WEAPON_PROFICIENCY_PERFECT,
};

const char* GetWeaponProficiencyName(WeaponProficiency_t proficiency);

#else

// New proficiency system!
enum ProficiencyValues_t
{
	PROF_MIN_BURST_TIME = 0,
	PROF_MAX_BURST_TIME,
	PROF_MIN_SHOT_COUNT,
	PROF_MAX_SHOT_COUNT,
	PROF_FIRERATE_SCALE,
	PROF_SPREAD_SCALE,
	PROF_SPREAD_BIAS,

	PROF_LAST_VALUE
};

// NPC info:
enum ProficiencyNpcType_t
{
	PROF_NPC_TYPE_ALLY = 1,
	PROF_NPC_TYPE_ENEMY
};

enum ProficiencyNpcTier_t
{
	PROF_NPC_TIER_1 = 1,
	PROF_NPC_TIER_2,
	PROF_NPC_TIER_3
};

#define PROF_MAX_TIER 3
#define PROF_MAX_SKILL 3
#define PROF_NPC_TYPES 2

class ProficiencyTableIndex_t
{
public:
	int		valuetype;
	int		tier;
	int		difficulty;
	int		npctype;

public:
	// Sets all members to -1
	void Initialize();

	// Copies all members from a index table
	void Copy(ProficiencyTableIndex_t* newindex);

	// This if for accessing the value of a specified index.
	// All values in the table must point to a specific index or else we fail.
	bool ValidForGetValue();

	// This if for setting the value of one or more indecies.
	// Only the valuetype must be specified, leaving any other values invalid means we apply the new value to all indecies in that column.
	bool ValidForSetValue();
};

class ProficiencyTable_t
{
public:
	// Accesses the value of a specified index in the table.
	float GetValue(ProficiencyTableIndex_t index);

	// Sets a new value for a specified index in the table. 
	// Leaving a parameter as -1 selects all indecies in that column
	void SetValue(ProficiencyTableIndex_t* index, float newvalue);

	// Prints out a full list of all indecies in the table to the console
	void Debug(void);

private:
	// A 4D array of all possible variable types, tiers, difficulty types, and npc types.
	float	m_flValues[PROF_LAST_VALUE][PROF_MAX_TIER][PROF_MAX_SKILL][PROF_NPC_TYPES];
};

#endif
#endif // WEAPON_PROFICIENCY_H