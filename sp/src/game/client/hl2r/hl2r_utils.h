#ifndef hl2r_utils
#define hl2r_utils
#ifdef _WIN32
#pragma once
#endif


bool	RemoveFilesInDirectory( const char *pDir, const char *pSearch );


enum
{
	OVERRIDE_TARGET_DIR = 0,
	OVERRIDE_START_DIR = 1
};
bool	MoveDirectory(	const char *pDirFolder,
						const char *pDir, 
						const char *pTargetDir, 
						CUtlVector<const char *> *pIgnoreList = NULL, 
						bool bOverrideMode = OVERRIDE_TARGET_DIR );
bool	MoveFilesInDirectory( const char *pDir, 
						const char *pTargetDir, 
						CUtlVector<const char *> *pIgnoreList = NULL, 
						bool bOverrideMode = OVERRIDE_TARGET_DIR );



const char	*GetSteamAppsDir(void);

#endif