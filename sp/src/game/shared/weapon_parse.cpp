//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"
#include "utldict.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The sound categories found in the weapon classname.txt files
// This needs to match the WeaponSound_t enum in weapon_parse.h
#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
const char *pWeaponSoundCategories[ NUM_SHOOT_SOUND_TYPES ] = 
{
	"empty",
	"single_shot",
	"single_shot_npc",
	"double_shot",
	"double_shot_npc",
	"burst",
	"reload",
	"reload_npc",
	"melee_miss",
	"melee_hit",
	"melee_hit_world",
	"special1",
	"special2",
	"special3",
	"taunt",
	"deploy"
};
#else
extern const char *pWeaponSoundCategories[ NUM_SHOOT_SOUND_TYPES ];
#endif

int GetWeaponSoundFromString( const char *pszString )
{
	for ( int i = EMPTY; i < NUM_SHOOT_SOUND_TYPES; i++ )
	{
		if ( !Q_stricmp(pszString,pWeaponSoundCategories[i]) )
			return (WeaponSound_t)i;
	}
	return -1;
}


// Item flags that we parse out of the file.
typedef struct
{
	const char *m_pFlagName;
	int m_iFlagValue;
} itemFlags_t;
#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
itemFlags_t g_ItemFlags[8] =
{
	{ "ITEM_FLAG_SELECTONEMPTY",	ITEM_FLAG_SELECTONEMPTY },
	{ "ITEM_FLAG_NOAUTORELOAD",		ITEM_FLAG_NOAUTORELOAD },
	{ "ITEM_FLAG_NOAUTOSWITCHEMPTY", ITEM_FLAG_NOAUTOSWITCHEMPTY },
	{ "ITEM_FLAG_LIMITINWORLD",		ITEM_FLAG_LIMITINWORLD },
	{ "ITEM_FLAG_EXHAUSTIBLE",		ITEM_FLAG_EXHAUSTIBLE },
	{ "ITEM_FLAG_DOHITLOCATIONDMG", ITEM_FLAG_DOHITLOCATIONDMG },
	{ "ITEM_FLAG_NOAMMOPICKUPS",	ITEM_FLAG_NOAMMOPICKUPS },
	{ "ITEM_FLAG_NOITEMPICKUP",		ITEM_FLAG_NOITEMPICKUP }
};
#else
extern itemFlags_t g_ItemFlags[7];
#endif


static CUtlDict< FileWeaponInfo_t*, unsigned short > m_WeaponInfoDatabase;

#ifdef _DEBUG
// used to track whether or not two weapons have been mistakenly assigned the wrong slot
bool g_bUsedWeaponSlots[MAX_WEAPON_SLOTS][MAX_WEAPON_POSITIONS] = { 0 };

#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : FileWeaponInfo_t
//-----------------------------------------------------------------------------
static WEAPON_FILE_INFO_HANDLE FindWeaponInfoSlot( const char *name )
{
	// Complain about duplicately defined metaclass names...
	unsigned short lookup = m_WeaponInfoDatabase.Find( name );
	if ( lookup != m_WeaponInfoDatabase.InvalidIndex() )
	{
		return lookup;
	}

	FileWeaponInfo_t *insert = CreateWeaponInfo();

	lookup = m_WeaponInfoDatabase.Insert( name, insert );
	Assert( lookup != m_WeaponInfoDatabase.InvalidIndex() );
	return lookup;
}

// Find a weapon slot, assuming the weapon's data has already been loaded.
WEAPON_FILE_INFO_HANDLE LookupWeaponInfoSlot( const char *name )
{
	return m_WeaponInfoDatabase.Find( name );
}



// FIXME, handle differently?
static FileWeaponInfo_t gNullWeaponInfo;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handle - 
// Output : FileWeaponInfo_t
//-----------------------------------------------------------------------------
FileWeaponInfo_t *GetFileWeaponInfoFromHandle( WEAPON_FILE_INFO_HANDLE handle )
{
	if ( handle < 0 || handle >= m_WeaponInfoDatabase.Count() )
	{
		return &gNullWeaponInfo;
	}

	if ( handle == m_WeaponInfoDatabase.InvalidIndex() )
	{
		return &gNullWeaponInfo;
	}

	return m_WeaponInfoDatabase[ handle ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : WEAPON_FILE_INFO_HANDLE
//-----------------------------------------------------------------------------
WEAPON_FILE_INFO_HANDLE GetInvalidWeaponInfoHandle( void )
{
	return (WEAPON_FILE_INFO_HANDLE)m_WeaponInfoDatabase.InvalidIndex();
}

#if 0
void ResetFileWeaponInfoDatabase( void )
{
	int c = m_WeaponInfoDatabase.Count(); 
	for ( int i = 0; i < c; ++i )
	{
		delete m_WeaponInfoDatabase[ i ];
	}
	m_WeaponInfoDatabase.RemoveAll();

#ifdef _DEBUG
	memset(g_bUsedWeaponSlots, 0, sizeof(g_bUsedWeaponSlots));
#endif
}
#endif

void PrecacheFileWeaponInfoDatabase( IFileSystem *filesystem, const unsigned char *pICEKey )
{
	if ( m_WeaponInfoDatabase.Count() )
		return;

	KeyValues *manifest = new KeyValues( "weaponscripts" );
	if ( manifest->LoadFromFile( filesystem, "scripts/weapon_manifest.txt", "GAME" ) )
	{
		for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL ; sub = sub->GetNextKey() )
		{
			if ( !Q_stricmp( sub->GetName(), "file" ) )
			{
				char fileBase[512];
				Q_FileBase( sub->GetString(), fileBase, sizeof(fileBase) );
				WEAPON_FILE_INFO_HANDLE tmp;
#ifdef CLIENT_DLL
				if ( ReadWeaponDataFromFileForSlot( filesystem, fileBase, &tmp, pICEKey ) )
				{
					gWR.LoadWeaponSprites( tmp );
				}
#else
				ReadWeaponDataFromFileForSlot( filesystem, fileBase, &tmp, pICEKey );
#endif
			}
			else
			{
				Error( "Expecting 'file', got %s\n", sub->GetName() );
			}
		}
	}
	manifest->deleteThis();
}

KeyValues* ReadEncryptedKVFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const unsigned char *pICEKey, bool bForceReadEncryptedFile /*= false*/ )
{
	Assert( strchr( szFilenameWithoutExtension, '.' ) == NULL );
	char szFullName[512];

	const char *pSearchPath = "MOD";

	if ( pICEKey == NULL )
	{
		pSearchPath = "GAME";
	}

	// Open the weapon data file, and abort if we can't
	KeyValues *pKV = new KeyValues( "WeaponDatafile" );

	Q_snprintf(szFullName,sizeof(szFullName), "%s.txt", szFilenameWithoutExtension);

	if ( bForceReadEncryptedFile || !pKV->LoadFromFile( filesystem, szFullName, pSearchPath ) ) // try to load the normal .txt file first
	{
#ifndef _XBOX
		if ( pICEKey )
		{
			Q_snprintf(szFullName,sizeof(szFullName), "%s.ctx", szFilenameWithoutExtension); // fall back to the .ctx file

			FileHandle_t f = filesystem->Open( szFullName, "rb", pSearchPath );

			if (!f)
			{
				pKV->deleteThis();
				return NULL;
			}
			// load file into a null-terminated buffer
			int fileSize = filesystem->Size(f);
			char *buffer = (char*)MemAllocScratch(fileSize + 1);
		
			Assert(buffer);
		
			filesystem->Read(buffer, fileSize, f); // read into local buffer
			buffer[fileSize] = 0; // null terminate file as EOF
			filesystem->Close( f );	// close file after reading

			UTIL_DecodeICE( (unsigned char*)buffer, fileSize, pICEKey );

			bool retOK = pKV->LoadFromBuffer( szFullName, buffer, filesystem );

			MemFreeScratch();

			if ( !retOK )
			{
				pKV->deleteThis();
				return NULL;
			}
		}
		else
		{
			pKV->deleteThis();
			return NULL;
		}
#else
		pKV->deleteThis();
		return NULL;
#endif
	}

	return pKV;
}


//-----------------------------------------------------------------------------
// Purpose: Read data on weapon from script file
// Output:  true  - if data2 successfully read
//			false - if data load fails
//-----------------------------------------------------------------------------

bool ReadWeaponDataFromFileForSlot( IFileSystem* filesystem, const char *szWeaponName, WEAPON_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey )
{
	if ( !phandle )
	{
		Assert( 0 );
		return false;
	}
	
	*phandle = FindWeaponInfoSlot( szWeaponName );
	FileWeaponInfo_t *pFileInfo = GetFileWeaponInfoFromHandle( *phandle );
	Assert( pFileInfo );

	if ( pFileInfo->bParsedScript )
		return true;

	char sz[128];
	Q_snprintf( sz, sizeof( sz ), "scripts/%s", szWeaponName );

	KeyValues *pKV = ReadEncryptedKVFile( filesystem, sz, pICEKey, false );

	if ( !pKV )
		return false;
	
	Q_strncat(sz, "_override", sizeof(sz), COPY_ALL_CHARACTERS);
	KeyValues *pKV2 = ReadEncryptedKVFile( filesystem, sz, pICEKey, false );
	
	// Check for any override scripts.
	if ( pKV2 )
	{
		for ( KeyValues *pKV2Sub = pKV2->GetFirstSubKey(); pKV2Sub != NULL; pKV2Sub = pKV2Sub->GetNextKey() )
		{
			if ( pKV2Sub->GetString() == NULL )
				continue;
				
			if ( pKV->FindKey( pKV2Sub->GetName() ) )
			{
				pKV->SetString( pKV2Sub->GetName(), pKV2->GetString( pKV2Sub->GetName() ) );
			}
			else
			{
				pKV->AddSubKey(pKV2Sub->MakeCopy());
			}
		}
	}

	pFileInfo->Parse( pKV, szWeaponName );

	pKV->deleteThis();

	return true;
}
#ifdef	USE_NEW_PROFICIENCY_SYSTEM

//-----------------------------------------------------------------------------
// Stuff for Npc spread:
//-----------------------------------------------------------------------------
enum NpcSpreadPresets_t
{
	NPCSPREAD_POOR,
	NPCSPREAD_MEDIOCRE,
	NPCSPREAD_AVERAGE,
	NPCSPREAD_GOOD,
	NPCSPREAD_VERY_GOOD,
	NPCSPREAD_PERFECT
};

struct NpcSpreadInfo_t
{
	float	spreadscale;
	float	bias;
};

static NpcSpreadInfo_t g_NpcSpreadInfo[NPCSPREAD_PERFECT + 1] =
{
	{ 3.0f,		0.75f	}, //	[poor]
	{ 2.0f,		0.75f	}, //	[mediocre]
	{ 1.75f,	0.8f	}, //	[average]
	{ 1.5f,		0.8f	}, //	[good]
	{ 1.25f,	0.8f	}, //	[very_good]
	{ 1.0f,		1.0f	}, //	[perfect]
};

static const char* g_NpcSpreadNames[] =
{
	"poor",
	"mediocre",
	"average",
	"good",
	"very_good",
	"perfect"
};

NpcSpreadInfo_t GetNpcSpreadInfo(const char* name)
{
	COMPILE_TIME_ASSERT(ARRAYSIZE(g_NpcSpreadNames) == NPCSPREAD_PERFECT + 1); // Hey, update that there table!

	for (int i = 0; i < NPCSPREAD_PERFECT+1; i++)
	{
		if (!Q_stricmp(name, g_NpcSpreadNames[i]))
			return g_NpcSpreadInfo[i];
	}

	Warning("Attempted to get npc spread type (%s) but it does not exist!\n", name);
	return { 0.0, 0.0 };
}

//-----------------------------------------------------------------------------
// Example of a standard shotregulator block:
//-----------------------------------------------------------------------------
/*
		"firerate_scale"		"1.0			1.0				1.0"		// How much to scale the weapon's firerate
		"shot_count"			"3	 4			4	5			5	6"		// Min-max amount of shots in a burst
		"burst_time"			"0.5 0.6		0.3	0.4			0.2	0.3"	// Min-max time between bursts
		"spread_scale"			"average		good			very_good"	// How much to scale the weapon's spread.

		"enemies"
		{
			"tier_1" // Metrocops
			{
				"spread_scale"	"poor			average			good"
			}
			"tier_2" // Combine Soldiers
			{
			}
			"tier_3" // Combine Elites
			{
				"spread_scale"	"good			very_good		perfect"
			}
		}
		"companions"
		{
			"tier_1" // Citizens
			{
				"spread_scale"	"poor			average			good"
			}
			"tier_2" // Essential Allies
			{
				"spread_scale"	"good			very_good		perfect"
			}
		}
*/

bool ParseHeader(ProficiencyTableIndex_t* index, KeyValues* pHeader);
void ParseKey(ProficiencyTable_t* table, ProficiencyTableIndex_t* index, KeyValues* pKey);
void ParseKeySection(ProficiencyTable_t* table, ProficiencyTableIndex_t* index, char* pToken, const char* pKeyName, int iSection);

void ParseShotregulatorBlock(ProficiencyTable_t *table, ProficiencyTableIndex_t *index, KeyValues *pData )
{
	// Iterate through all keys in this header
	for (KeyValues* sub = pData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		// If this key doesn't contain a string, it's a header.
		if (!Q_stricmp(sub->GetString(), ""))
		{
			ProficiencyTableIndex_t *newindex = new ProficiencyTableIndex_t;
			newindex->Copy(index);

			if ( !ParseHeader(index, sub) )
				Error("Shotregulator header name [%s] is an invalid value!", sub->GetName() );

			ParseShotregulatorBlock(table, index, sub);
			delete newindex;
		}
		else
		{
			ParseKey(table, index, sub);
		}
	}
}

bool ParseHeader(ProficiencyTableIndex_t *index, KeyValues *pHeader)
{
	char szHeaderName[64];
	V_strcpy_safe(szHeaderName, pHeader->GetName());

	if (!Q_stricmp(szHeaderName, "enemies"))
	{
		index->npctype = PROF_NPC_TYPE_ENEMY;
		return true;
	}

	if (!Q_stricmp(szHeaderName, "companions"))
	{
		index->npctype = PROF_NPC_TYPE_ALLY;
		return true;
	}

	if (V_stristr(szHeaderName, "tier_"))
	{
		char szTierNum[64];
		V_StrSubst(szHeaderName, "tier_", "", szTierNum, 64);
		int iTier = V_atoi(szTierNum);

		if (iTier < 1 || iTier > PROF_MAX_TIER)
		{
			Error("Shotregulator tier_ is set to an invalid value!");
			return false;
		}

		index->tier = iTier;
		return true;
	}

	return false;
}

void ParseKey(ProficiencyTable_t *table, ProficiencyTableIndex_t *index, KeyValues *pKey)
{
	char szKeyName[64];
	V_strcpy_safe(szKeyName, pKey->GetName());

	char szKeyString[128];
	V_strcpy_safe(szKeyString, pKey->GetString());

	int section = SKILL_EASY;
	for (char* pszToken = strtok(szKeyString, " \t"); pszToken != NULL; pszToken = strtok(NULL, " \t"))
	{
		if (section > SKILL_HARD)
			Error("Shotregulator key has too many sections!");

		ParseKeySection(table, index, pszToken, szKeyName, section);
		section++;
	}
}

void ParseKeySection( ProficiencyTable_t *table, ProficiencyTableIndex_t *index, char *pToken, const char *pKeyName, int iSection )
{
	index->difficulty = iSection;

	if ( !Q_stricmp(pKeyName, "firerate_scale" ) )
	{
		index->valuetype = PROF_FIRERATE_SCALE;
		table->SetValue(index, V_atof(pToken));
		return;
	}

	if ( !Q_stricmp(pKeyName, "shot_count" ) )
	{
		// Convert the shotregulator valuetype to a proficiency valuetype
		index->valuetype = PROF_MIN_SHOT_COUNT;
		table->SetValue(index, V_atof(pToken));

		pToken = strtok(NULL, " \t");

		index->valuetype = PROF_MAX_SHOT_COUNT;
		table->SetValue(index, V_atof(pToken));
		return;
	}

	if ( !Q_stricmp(pKeyName, "burst_time" ) )
	{
		// Convert the shotregulator valuetype to a proficiency valuetype
		index->valuetype = PROF_MIN_BURST_TIME;
		table->SetValue(index, V_atof(pToken));

		pToken = strtok(NULL, " \t");

		index->valuetype = PROF_MAX_BURST_TIME;
		table->SetValue(index, V_atof(pToken));
		return;
	}

	if ( !Q_stricmp(pKeyName, "spread_scale" ) )
	{
		// Convert the shotregulator valuetype to a proficiency valuetype
		index->valuetype = PROF_SPREAD_SCALE;
		float spreadscale = GetNpcSpreadInfo(pToken).spreadscale;

		table->SetValue(index, spreadscale);

		index->valuetype = PROF_SPREAD_BIAS;

		float spreadbias = GetNpcSpreadInfo(pToken).bias;
		table->SetValue(index, spreadbias);
		return;
	}

	Error("Shotregulator key [%s] is an unknown keyvalue!", pKeyName);
}
#endif
//-----------------------------------------------------------------------------
// FileWeaponInfo_t implementation.
//-----------------------------------------------------------------------------

FileWeaponInfo_t::FileWeaponInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;
	szClassName[0] = 0;
	szPrintName[0] = 0;

	szViewModel[0] = 0;
	szWorldModel[0] = 0;
	szAnimationPrefix[0] = 0;
	iSlot = 0;
	iPosition = 0;
	iMaxClip1 = 0;
	iMaxClip2 = 0;
	iDefaultClip1 = 0;
	iDefaultClip2 = 0;
	iWeight = 0;
	iRumbleEffect = -1;
	bAutoSwitchTo = false;
	bAutoSwitchFrom = false;
	iFlags = 0;
	szAmmo1[0] = 0;
	szAmmo1_carry[0] = 0;
	szAmmo1_damage[0] = 0;
	szAmmo2[0] = 0;
	memset( aShootSounds, 0, sizeof( aShootSounds ) );
	memset( aAnimNames, 0, sizeof( aAnimNames ) );
	iAmmoType = 0;
	iAmmo2Type = 0;
	m_bMeleeWeapon = false;
	iSpriteCount = 0;
	iconActive = 0;
	iconInactive = 0;
	iconAmmo = 0;
	iconAmmo2 = 0;
	iconCrosshair = 0;
	iconAutoaim = 0;
	iconZoomedCrosshair = 0;
	iconZoomedAutoaim = 0;
	bShowUsageHint = false;
	m_bAllowFlipping = true;
	m_bBuiltRightHanded = true;

	m_ProficiencyValues = NULL;
}

#ifdef CLIENT_DLL
extern ConVar hud_fastswitch;
#endif

extern ConVar hl2r_debug_weapon_proficiency;

void FileWeaponInfo_t::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	// Classname
	Q_strncpy( szClassName, szWeaponName, MAX_WEAPON_STRING );
	// Printable name
	Q_strncpy( szPrintName, pKeyValuesData->GetString( "printname", WEAPON_PRINTNAME_MISSING ), MAX_WEAPON_STRING );
	// View model & world model
	Q_strncpy( szViewModel, pKeyValuesData->GetString( "viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szWorldModel, pKeyValuesData->GetString( "playermodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szAnimationPrefix, pKeyValuesData->GetString( "anim_prefix" ), MAX_WEAPON_PREFIX );
	iSlot = pKeyValuesData->GetInt( "bucket", 0 );
	iPosition = pKeyValuesData->GetInt( "bucket_position", 0 );
	
	// Use the console (X360) buckets if hud_fastswitch is set to 2.
#ifdef CLIENT_DLL
	if ( hud_fastswitch.GetInt() == 2 )
#else
	if ( IsX360() )
#endif
	{
		iSlot = pKeyValuesData->GetInt( "bucket_360", iSlot );
		iPosition = pKeyValuesData->GetInt( "bucket_position_360", iPosition );
	}
	iMaxClip1 = pKeyValuesData->GetInt( "clip_size", WEAPON_NOCLIP );					// Max primary clips gun can hold (assume they don't use clips by default)
	iMaxClip2 = pKeyValuesData->GetInt( "clip2_size", WEAPON_NOCLIP );					// Max secondary clips gun can hold (assume they don't use clips by default)
	iDefaultClip1 = pKeyValuesData->GetInt( "default_clip", iMaxClip1 );		// amount of primary ammo placed in the primary clip when it's picked up
	iDefaultClip2 = pKeyValuesData->GetInt( "default_clip2", iMaxClip2 );		// amount of secondary ammo placed in the secondary clip when it's picked up
	iWeight = pKeyValuesData->GetInt( "weight", 0 );

	iRumbleEffect = pKeyValuesData->GetInt( "rumble", -1 );
	
	// LAME old way to specify item flags.
	// Weapon scripts should use the flag names.
	iFlags = pKeyValuesData->GetInt( "item_flags", ITEM_FLAG_LIMITINWORLD );

	for ( int i=0; i < ARRAYSIZE( g_ItemFlags ); i++ )
	{
		int iVal = pKeyValuesData->GetInt( g_ItemFlags[i].m_pFlagName, -1 );
		if ( iVal == 0 )
		{
			iFlags &= ~g_ItemFlags[i].m_iFlagValue;
		}
		else if ( iVal == 1 )
		{
			iFlags |= g_ItemFlags[i].m_iFlagValue;
		}
	}


	bShowUsageHint = ( pKeyValuesData->GetInt( "showusagehint", 0 ) != 0 ) ? true : false;
	bAutoSwitchTo = ( pKeyValuesData->GetInt( "autoswitchto", 1 ) != 0 ) ? true : false;
	bAutoSwitchFrom = ( pKeyValuesData->GetInt( "autoswitchfrom", 1 ) != 0 ) ? true : false;
	m_bBuiltRightHanded = ( pKeyValuesData->GetInt( "BuiltRightHanded", 1 ) != 0 ) ? true : false;
	m_bAllowFlipping = ( pKeyValuesData->GetInt( "AllowFlipping", 1 ) != 0 ) ? true : false;
	m_bMeleeWeapon = ( pKeyValuesData->GetInt( "MeleeWeapon", 0 ) != 0 ) ? true : false;

#if defined(_DEBUG) && defined(HL2_CLIENT_DLL)
	// make sure two weapons aren't in the same slot & position
	if ( iSlot >= MAX_WEAPON_SLOTS ||
		iPosition >= MAX_WEAPON_POSITIONS )
	{
		Warning( "Invalid weapon slot or position [slot %d/%d max], pos[%d/%d max]\n",
			iSlot, MAX_WEAPON_SLOTS - 1, iPosition, MAX_WEAPON_POSITIONS - 1 );
	}
	else
	{
		if (g_bUsedWeaponSlots[iSlot][iPosition])
		{
			Warning( "Duplicately assigned weapon slots in selection hud:  %s (%d, %d)\n", szPrintName, iSlot, iPosition );
		}
		g_bUsedWeaponSlots[iSlot][iPosition] = true;
	}
#endif

	// Primary ammo used
	const char *pAmmo = pKeyValuesData->GetString( "primary_ammo", "None" );
	if ( strcmp("None", pAmmo) == 0 )
	{
		Q_strncpy( szAmmo1, "", sizeof( szAmmo1 ) );
		Q_strncpy( szAmmo1_carry, "", sizeof( szAmmo1_carry ) );
		Q_strncpy( szAmmo1_damage, "", sizeof( szAmmo1_damage ) );
	}
	else
	{
		Q_strncpy( szAmmo1, pAmmo, sizeof( szAmmo1 )  );
		Q_strncpy( szAmmo1_carry, pAmmo, sizeof( szAmmo1_carry )  );
		Q_strncpy( szAmmo1_damage, pAmmo, sizeof( szAmmo1_damage )  );
	}

	const char *pAmmoDamage = pKeyValuesData->GetString( "primary_ammo_dmg", "None" );
	if ( strcmp("None", pAmmoDamage) )
		Q_strncpy( szAmmo1_damage, pAmmoDamage, sizeof( szAmmo1_damage )  );

	const char *pAmmoCarry = pKeyValuesData->GetString( "primary_ammo_carry", "None" );
	if ( strcmp("None", pAmmoCarry) )
		Q_strncpy( szAmmo1_carry, pAmmoCarry, sizeof( szAmmo1_carry )  );

	iAmmoType = GetAmmoDef()->Index( szAmmo1 );
	iAmmoType_carry = GetAmmoDef()->Index( szAmmo1_carry );
	iAmmoType_damage = GetAmmoDef()->Index( szAmmo1_damage );
	
	// Secondary ammo used
	pAmmo = pKeyValuesData->GetString( "secondary_ammo", "None" );
	if ( strcmp("None", pAmmo) == 0)
		Q_strncpy( szAmmo2, "", sizeof( szAmmo2 ) );
	else
		Q_strncpy( szAmmo2, pAmmo, sizeof( szAmmo2 )  );
	iAmmo2Type = GetAmmoDef()->Index( szAmmo2 );

	// Now read the weapon sounds
	memset( aShootSounds, 0, sizeof( aShootSounds ) );
	KeyValues *pSoundData = pKeyValuesData->FindKey( "SoundData" );
	if ( pSoundData )
	{
		for ( int i = EMPTY; i < NUM_SHOOT_SOUND_TYPES; i++ )
		{
			const char *soundname = pSoundData->GetString( pWeaponSoundCategories[i] );
			if ( soundname && soundname[0] )
			{
				Q_strncpy( aShootSounds[i], soundname, MAX_WEAPON_STRING );
			}
		}
	}
	
	memset( aAnimNames, 0, sizeof( aAnimNames ) );
	KeyValues *pAnimSpeedSection = pKeyValuesData->FindKey( "AnimSpeed" );
	if ( pAnimSpeedSection  )
	{
		KeyValues *pTemp = pAnimSpeedSection->GetFirstSubKey();
		for ( int i = 0; i < MAX_WEAPON_ANIM_STRINGS; i++ )
		{
			if (!pTemp)
				continue;
			
			if ( pTemp->GetName() && pTemp->GetFloat() )
			{
				Q_strncpy( aAnimNames[i], pTemp->GetName(), MAX_WEAPON_STRING );
				fAnimSpeed[i] = pTemp->GetFloat();
			}
			
			pTemp = pTemp->GetNextKey();
		}
	}
	
#ifdef	USE_NEW_PROFICIENCY_SYSTEM
	KeyValues *pShotRegulatorBlock = pKeyValuesData->FindKey( "ShotRegulator" );
	if (pShotRegulatorBlock)
	{
		// Create a new table for this weapon.
		m_ProficiencyValues = new ProficiencyTable_t;

		// Set all indecies in the valuetype array to -1
		for (int i = 0; i < PROF_LAST_VALUE; i++ )
		{
			ProficiencyTableIndex_t *initIndex = new ProficiencyTableIndex_t;
			initIndex->Initialize();

			initIndex->valuetype = i;

			m_ProficiencyValues->SetValue(initIndex, -1);

			delete initIndex;
		}

		// Parse the shotregulator block:
		ProficiencyTableIndex_t *initIndex = new ProficiencyTableIndex_t;
		initIndex->Initialize();

		ParseShotregulatorBlock( m_ProficiencyValues, initIndex, pShotRegulatorBlock);


	//	if (hl2r_debug_weapon_proficiency.GetBool())
	//	{
	//		DevMsg("Shotregulator information for weapon [%s]\n", szPrintName);
	//		m_ProficiencyValues->Debug();
	//	}

		delete initIndex;
	}
#endif
}			
		

