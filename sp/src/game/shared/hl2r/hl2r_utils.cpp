#include <windows.h>
#include "cbase.h"
#include "filesystem.h"
#include "hl2r_utils.h"
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Removes all files in a given directory
// 
// Input  : pDir - Our target directory.
//			pSearch - Only remove files that contain a given string. Useful for only removing files
//					  of a specified filetype.
//-----------------------------------------------------------------------------
bool RemoveFilesInDirectory( const char *pDir, const char *pSearch )
{
	bool bSuccess = false;

	char szSearchPath[MAX_PATH];
	V_sprintf_safe( szSearchPath, "%s\\*.*", pDir);

	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( szSearchPath, &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		if ( !Q_stricmp( dirName, "root" ) || !Q_stricmp( dirName, ".." ) || !Q_stricmp( dirName, "." ) )
			continue;

		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "%s\\%s", pDir, dirName );

		if ( g_pFullFileSystem->IsDirectory(szFilePath) )
		{
			if ( RemoveFilesInDirectory(szFilePath, pSearch) )
			{
				RemoveDirectory(szFilePath);
				bSuccess = true;
			}
		}
		else
		{
			if ( pSearch != NULL )
			{
				if ( V_stristr( szFilePath, pSearch ) )
				{
					g_pFullFileSystem->RemoveFile(szFilePath);
					bSuccess = true;
				}
			}
			else
			{
				g_pFullFileSystem->RemoveFile(szFilePath);
				bSuccess = true;
			}
		}
	}

	RemoveDirectory(pDir);

	g_pFullFileSystem->FindClose( fh );
	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Moves a directory from one location to another
// 
// Input  : pDirFolder - The directory that contains the directory we want to move.
//			pDir - The directory we want to move.
//			pTargetDir - The directory we want to move the input directory to.
//			pIgnoreList	- A list of filesnames to not move.
//-----------------------------------------------------------------------------

bool MoveDirectory( const char *pDirFolder, const char *pDir, const char *pTargetDir, CUtlVector<const char *> *pIgnoreList, bool bOverrideMode )
{
	bool bSuccess = false;

	// The directory the file transfer is happening in.
	char szSearchPath[MAX_PATH];

	if ( pDirFolder == NULL )
	{
		V_sprintf_safe( szSearchPath, "%s\\%s\\*.*", engine->GetGameDirectory(), pDir );
	}
	else
	{
		V_sprintf_safe( szSearchPath, "%s\\%s\\%s\\*.*", engine->GetGameDirectory(), pDirFolder, pDir );
	}

	//DevMsg("Directory folder is: [%s]		Directory is: [%s]		TargetDirectory is: [%s]\n\n\n", pDirFolder, pDir, pTargetDir);

	// Iterate through all files in this directory
	FileFindHandle_t fh;
	for ( const char *pFileName = g_pFullFileSystem->FindFirst( szSearchPath, &fh ); pFileName; pFileName = g_pFullFileSystem->FindNext( fh ))
	{
		// Handle invalid directories
		if ( !Q_stricmp( pFileName, "root" ) || !Q_stricmp( pFileName, ".." ) || !Q_stricmp( pFileName, "." ) )
			continue;

		if (pIgnoreList)
		{
			bool bIgnoreFile = false;
			for (int i = 0; i < pIgnoreList->Count(); i++)
			{
				if (!Q_stricmp(pFileName, pIgnoreList->Element(i)))
				{
					bIgnoreFile = true;
					break;
				}
			}

			if (bIgnoreFile)
				continue;
		}

		char szFilePath[MAX_PATH];
		if ( pDirFolder == NULL )
		{
			V_sprintf_safe( szFilePath, "%s\\%s", pDir, pFileName );
		}
		else
		{
			V_sprintf_safe( szFilePath, "%s\\%s\\%s", pDirFolder, pDir, pFileName );
		}

		// If we hit a directory, transfer the files in it too.
		if ( g_pFullFileSystem->IsDirectory(szFilePath) )
		{
			char szRelativeFilePath[MAX_PATH];
			V_sprintf_safe( szRelativeFilePath, "%s\\%s", pDir, pFileName );

		//	DevMsg("\n\n");
			if ( MoveDirectory( pDirFolder, szRelativeFilePath, pTargetDir, pIgnoreList, bOverrideMode ) )
				bSuccess = true;
		}
		else
		{
			char szNewFilePath[MAX_PATH];
			if ( pTargetDir == NULL )
			{
				V_sprintf_safe( szNewFilePath, "%s\\%s", pDir, pFileName );
			}
			else
			{
				V_sprintf_safe( szNewFilePath, "%s\\%s\\%s", pTargetDir, pDir, pFileName );
			}

	//		DevMsg("	Filepath is: [%s]		RenameFile Command is: [%s]", szFilePath, szNewFilePath);
	//		DevMsg("\n");


			// File conflict
			if ( g_pFullFileSystem->FileExists(szNewFilePath) )
			{
				if ( bOverrideMode == OVERRIDE_TARGET_DIR )
				{
					// Remove the conflicting file and move the original file in.
					g_pFullFileSystem->RemoveFile(szNewFilePath);
					g_pFullFileSystem->RenameFile(szFilePath, szNewFilePath);
					bSuccess = true;
				}
				else
				{
					// Remove the original file.
					g_pFullFileSystem->RemoveFile(szNewFilePath);
				}
			}
			else
			{
				g_pFullFileSystem->RenameFile(szFilePath, szNewFilePath);
				bSuccess = true;
			}
		}
	}

//	DevMsg("\n\n");
	g_pFullFileSystem->FindClose( fh );
	return bSuccess;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool MoveFilesInDirectory( const char *pDir, const char *pTargetDir, CUtlVector<const char *> *pIgnoreList, bool bOverrideMode )
{
	bool bSuccess = false;

	// Don't allow full-file transfers to or from our base folder.
	if ( pDir == NULL || pTargetDir == NULL )
		return bSuccess;

	// The directory the file transfer is happening in.
	char szSearchPath[MAX_PATH];
	V_sprintf_safe( szSearchPath, "%s\\%s\\*.*", engine->GetGameDirectory(), pDir );

	// Iterate through all files in this directory
	FileFindHandle_t fh;
	for ( const char *pFileName = g_pFullFileSystem->FindFirst( szSearchPath, &fh ); pFileName; pFileName = g_pFullFileSystem->FindNext( fh ))
	{
		if ( !Q_stricmp( pFileName, "root" ) || !Q_stricmp( pFileName, ".." ) || !Q_stricmp( pFileName, "." ) )
			continue;

		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "%s\\%s", pDir, pFileName );

		// Make sure we hit a file and not a directory.
		if ( !g_pFullFileSystem->IsDirectory(szFilePath) )
		{
			if ( pIgnoreList )
			{
				bool bIgnoreFile = false;
				for (int i = 0; i < pIgnoreList->Count(); i++ )
				{
					if ( !Q_stricmp(pFileName, pIgnoreList->Element(i) ) )
					{
						bIgnoreFile = true;
						break;
					}
				}

				if ( bIgnoreFile )
					continue;
			}

			// The path of this file if it were in the target directory.
			char szNewFilePath[MAX_PATH];
			V_sprintf_safe( szNewFilePath, "%s\\%s", pTargetDir, pFileName );

			// File conflict
			if ( g_pFullFileSystem->FileExists(szNewFilePath) )
			{
				if ( bOverrideMode == OVERRIDE_TARGET_DIR )
				{
					// Remove the conflicting file and move the original file in.
					g_pFullFileSystem->RemoveFile(szNewFilePath);
					g_pFullFileSystem->RenameFile(szFilePath, szNewFilePath);
					bSuccess = true;
				}
				else
				{
					// Remove the original file.
					g_pFullFileSystem->RemoveFile(szNewFilePath);
				}
			}
			else
			{
				g_pFullFileSystem->RenameFile(szFilePath, szNewFilePath);
				bSuccess = true;
			}
		}
	}

	g_pFullFileSystem->FindClose( fh );
	return bSuccess;
}
//-----------------------------------------------------------------------------
// Purpose: Returns a string containing the filepath to the steamapps directory of this mod.
//-----------------------------------------------------------------------------
const char *GetSteamAppsDir(void)
{
	static char steamappsdir[MAX_PATH];

	V_strcpy(steamappsdir, engine->GetGameDirectory() );

	int iSliceChar = MAX_PATH;
	int iSlashCount = 0;

	// Iterate backwards from the end of our GetGameDirectory() string to the start and look for any slashes.
	// As soon as we've found 2 (moved up 2 directories) stop and return that as the steamapps directory.
	while ( iSliceChar > 0 )
	{
		if ( steamappsdir[iSliceChar] && steamappsdir[iSliceChar] == '\\' )
			iSlashCount++;

		if (iSlashCount == 2)
			break;

		iSliceChar--;
	}

	if (iSliceChar == 0)
		return NULL;

	V_StrSlice(steamappsdir, 0, iSliceChar, steamappsdir, MAX_PATH );
	return steamappsdir;
}
#endif
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
char* ReadFileIntoBuffer( const char *pFilePath, int &len )
{
	FileHandle_t fh = g_pFullFileSystem->Open(pFilePath, "rb");
	if ( fh == FILESYSTEM_INVALID_HANDLE )
		return NULL;

	// read file into memory
	int size = g_pFullFileSystem->Size(fh);
	char* buf = new char[size + 1];
	g_pFullFileSystem->Read(buf, size, fh);
	buf[size] = 0;
	g_pFullFileSystem->Close(fh);

	len += size+1;
	return buf;
}