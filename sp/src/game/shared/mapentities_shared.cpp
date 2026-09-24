//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Controls the loading, parsing and creation of the entities from the BSP.
//
//=============================================================================//


#include "cbase.h"
#include "mapentities_shared.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)

static const char *s_BraceChars = "{}()\'";
static bool s_BraceCharacters[256];
static bool s_BuildReverseMap = true;

bool MapEntity_ExtractValue( const char *pEntData, const char *keyName, char Value[MAPKEY_MAXLENGTH] )
{
	char token[MAPKEY_MAXLENGTH];
	const char *inputData = pEntData;

	while ( inputData )
	{
		inputData = MapEntity_ParseToken( inputData, token );	// get keyname
		if ( token[0] == '}' )									// end of entity?
			break;												// must not have seen the classname

		// is this the right key?
		if ( !strcmp(token, keyName) )
		{
			inputData = MapEntity_ParseToken( inputData, token );	// get value and return it
			Q_strncpy( Value, token, MAPKEY_MAXLENGTH );
			return true;
		}

		inputData = MapEntity_ParseToken( inputData, token );	// skip over value
	}

	return false;
}

int MapEntity_GetNumKeysInEntity( const char *pEntData )
{
	char token[MAPKEY_MAXLENGTH];
	const char *inputData = pEntData;
	int iNumKeys = 0;

	while ( inputData )
	{
		inputData = MapEntity_ParseToken( inputData, token );	// get keyname
		if ( token[0] == '}' )									// end of entity?
			break;												// must not have seen the classname

		iNumKeys++;

		inputData = MapEntity_ParseToken( inputData, token );	// skip over value
	}

	return iNumKeys;
}


// skips to the beginning of the next entity in the data block
// returns NULL if no more entities
const char *MapEntity_SkipToNextEntity( const char *pMapData, char *pWorkBuffer )
{
	if ( !pMapData )
		return NULL;

	// search through the map string for the next matching '{'
	int openBraceCount = 1;
	while ( pMapData != NULL )
	{
		pMapData = MapEntity_ParseToken( pMapData, pWorkBuffer );

		if ( FStrEq(pWorkBuffer, "{") )
		{
			openBraceCount++;
		}
		else if ( FStrEq(pWorkBuffer, "}") )
		{
			if ( --openBraceCount == 0 )
			{
				// we've found the closing brace, so return the next character
				return pMapData;
			}
		}
	}

	// eof hit
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: parses a token out of a char data block
//			the token gets fully read no matter what the length, but only MAPKEY_MAXLENGTH 
//			characters are written into newToken
// Input  : char *data - the data to parse
//			char *newToken - the buffer into which the new token is written
//			char *braceChars - a string of characters that constitute braces.  this pointer needs to be
//			distince for each set of braceChars, since the usage is cached.
// Output : const char * - returns a pointer to the position in the data following the newToken
//-----------------------------------------------------------------------------
const char *MapEntity_ParseToken( const char *data, char *newToken )
{
	int             c;
	int             len;
		
	len = 0;
	newToken[0] = 0;
	
	if (!data)
		return NULL;

	// build the new table if we have to
	if ( s_BuildReverseMap )
	{
		s_BuildReverseMap = false; 

		Q_memset( s_BraceCharacters, 0, sizeof(s_BraceCharacters) );

		for ( const char *c = s_BraceChars; *c; c++ )
		{
			s_BraceCharacters[*c] = true;
		}
	}
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while ( len < MAPKEY_MAXLENGTH )
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				newToken[len] = 0;
				return data;
			}
			newToken[len] = c;
			len++;
		}

		if ( len >= MAPKEY_MAXLENGTH )
		{
			len--;
			newToken[len] = 0;
		}
	}

// parse single characters
	if ( s_BraceCharacters[c]/*c=='{' || c=='}'|| c==')'|| c=='(' || c=='\''*/ )
	{
		newToken[len] = c;
		len++;
		newToken[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		newToken[len] = c;
		data++;
		len++;
		c = *data;
		if ( s_BraceCharacters[c] /*c=='{' || c=='}'|| c==')'|| c=='(' || c=='\''*/ )
			break;

		if ( len >= MAPKEY_MAXLENGTH )
		{
			len--;
			newToken[len] = 0;
		}

	} while (c>32);
	
	newToken[len] = 0;
	return data;
}

#endif // !STATIC_LINKED || CLIENT_DLL

/* ================= CEntityMapData definition ================ */

bool CEntityMapData::ExtractValue( const char *keyName, char *value )
{
	return MapEntity_ExtractValue( m_pEntData, keyName, value );
}

bool CEntityMapData::GetFirstKey( char *keyName, char *value )
{
	m_pCurrentKey = m_pEntData; // reset the status pointer
	return GetNextKey( keyName, value );
}

const char *CEntityMapData::CurrentBufferPosition( void )
{
	return m_pCurrentKey;
}

bool CEntityMapData::GetNextKey( char *keyName, char *value )
{
	char token[MAPKEY_MAXLENGTH];

	// parse key
	char *pPrevKey = m_pCurrentKey;
	m_pCurrentKey = (char*)MapEntity_ParseToken( m_pCurrentKey, token );
	if ( token[0] == '}' )
	{
		// step back
		m_pCurrentKey = pPrevKey;
		return false;
	}

	if ( !m_pCurrentKey )
	{
		Warning( "CEntityMapData::GetNextKey: EOF without closing brace\n" );
		Assert(0);
		return false;
	}
	
	Q_strncpy( keyName, token, MAPKEY_MAXLENGTH );

	// fix up keynames with trailing spaces
	int n = strlen(keyName);
	while (n && keyName[n-1] == ' ')
	{
		keyName[n-1] = 0;
		n--;
	}

	// parse value	
	m_pCurrentKey = (char*)MapEntity_ParseToken( m_pCurrentKey, token );
	if ( !m_pCurrentKey )
	{
		Warning( "CEntityMapData::GetNextKey: EOF without closing brace\n" );
		Assert(0);
		return false;
	}
	if ( token[0] == '}' )
	{
		Warning( "CEntityMapData::GetNextKey: closing brace without data\n" );
		Assert(0);
		return false;
	}

	// value successfully found
	Q_strncpy( value, token, MAPKEY_MAXLENGTH );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: find the keyName in the endata and change its value to specified one
//-----------------------------------------------------------------------------
bool CEntityMapData::SetValue( const char *keyName, char *NewValue, int nKeyInstance )
{
	// If this is -1, the size of the string is unknown and cannot be safely modified!
	Assert( m_nEntDataSize != -1 );
	if ( m_nEntDataSize == -1 )
		return false;

	char token[MAPKEY_MAXLENGTH];
	char *inputData = m_pEntData;
	char *prevData;

	char newvaluebuf[ 1024 ];
	int nCurrKeyInstance = 0;

	while ( inputData )
	{
		inputData = (char*)MapEntity_ParseToken( inputData, token );	// get keyname
		if ( token[0] == '}' )									// end of entity?
			break;												// must not have seen the classname

		// is this the right key?
		if ( !strcmp(token, keyName) )
		{
			++nCurrKeyInstance;
			if ( nCurrKeyInstance > nKeyInstance )
			{
				// Find the start & end of the token we're going to replace
				int entLen = strlen(m_pEntData);
				char *postData = new char[entLen];
				prevData = inputData;
				inputData = (char*)MapEntity_ParseToken( inputData, token );	// get keyname
				Q_strncpy( postData, inputData, entLen );

				// Insert quotes if caller didn't
				if ( NewValue[0] != '\"' )
				{
					Q_snprintf( newvaluebuf, sizeof( newvaluebuf ), "\"%s\"", NewValue );
				}
				else
				{
					Q_strncpy( newvaluebuf, NewValue, sizeof( newvaluebuf ) );
				}

				int iNewValueLen = Q_strlen(newvaluebuf);
				int iPadding = iNewValueLen - Q_strlen( token ) - 2;	// -2 for the quotes (token doesn't have them)

				// prevData has a space at the start, seperating the value from the key.
				// Add 1 to prevData when pasting in the new Value, to account for the space.
				Q_strncpy( prevData+1, newvaluebuf, iNewValueLen+1 );	// +1 for the null terminator
				Q_strcat( prevData, postData, m_nEntDataSize - ((prevData-m_pEntData)+1) );

				m_pCurrentKey += iPadding;
				delete [] postData;
				return true;
			}
		}

		inputData = (char*)MapEntity_ParseToken( inputData, token );	// skip over value
	}

	return false;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static char m_szTargetKey[256];
bool ParseConditional( char *pToken, char *pOperator, char *pRight)
{

	pToken = strtok(NULL, " ");

	// Go to the next token to get the operator
	Q_strcpy(pOperator, pToken);

	pToken = strtok(NULL, " ");

	// Go to the next token to get the right side
	Q_strcpy(pRight, pToken);

	return true;
}

bool ParseConditional( char *pToken, char *pLeft, char *pOperator, char *pRight)
{
	pToken = strtok(NULL, " ");

	// Go to the next token to get the left side
	Q_strcpy(pLeft, pToken);

	pToken = strtok(NULL, " ");

	// Go to the next token to get the operator
	Q_strcpy(pOperator, pToken);

	pToken = strtok(NULL, " ");

	// Go to the next token to get the right side
	Q_strcpy(pRight, pToken);

	return true;
}

bool EvaluateConditional( const char *pLeft, const char *pOperator, const char *pRight)
{
	// Partially equal
	if ( !Q_stricmp(pOperator, "=") )
		return V_stristr( pLeft, pRight ) != 0;

	// Fully equal
	if ( !Q_stricmp(pOperator, "==") )
		return !(Q_stricmp( pLeft, pRight ));

	// Not partially equal
	if ( !Q_stricmp(pOperator, "!=") )
		return V_stristr( pLeft, pRight ) == 0;

	// Not fully equal
	if ( !Q_stricmp(pOperator, "!==") )
		return Q_stricmp( pLeft, pRight ) != 0;

	// Lesser than
	if ( !Q_stricmp(pOperator, "<") )
		return atof(pLeft) < atof(pRight);

	// Lesser than or equal to
	if ( !Q_stricmp(pOperator, "<=") )
		return atof(pLeft) <= atof(pRight);

	// Greater than
	if ( !Q_stricmp(pOperator, ">") )
		return atof(pLeft) > atof(pRight);

	// Greater than or equal to
	if ( !Q_stricmp(pOperator, ">=") )
		return atof(pLeft) >= atof(pRight);

	return -1;
}

bool GetKeyConditional(char *pszToken)
{
	if (!Q_stricmp(pszToken, "mapname"))
		return true;

	if (!Q_stricmp(pszToken, "key_get"))
		return true;

	if (V_stristr(pszToken, "rnd_chance"))
		return true;

	return false;
}

bool EvaluateKeyConditional(char *pszToken, CEntityMapData *pEntData)
{
	if (!Q_stricmp(pszToken, "mapname"))
	{
		char szOperator[4]; char szRight[256];
		ParseConditional(pszToken, szOperator, szRight);
#ifdef CLIENT_DLL
		return EvaluateConditional(engine->GetLevelName(), szOperator, szRight);
#else
		return EvaluateConditional(STRING(gpGlobals->mapname), szOperator, szRight);
#endif
	}

	if (!Q_stricmp(pszToken, "key_get"))
	{
		char szLeft[256]; char szOperator[4]; char szRight[256];
		ParseConditional(pszToken, szLeft, szOperator, szRight);

		char szActualKeyValue[MAPKEY_MAXLENGTH];
		if (pEntData->ExtractValue(szLeft, szActualKeyValue))
		{
			return EvaluateConditional(szActualKeyValue, szOperator, szRight);
		}
	}

	if (V_stristr(pszToken, "rnd_chance"))
	{
		if (random->RandomInt(0, 100) <= atof(pszToken))
			return true;
	}

	return false;
}

enum eConnectorType
{
	CONNECTOR_AND = 0,
	CONNECTOR_OR,
};

struct ConditionsBlock_t
{
	CUtlVector<bool> conditions;	// Conditions within this block
	CUtlVector<eConnectorType> connectors;

	bool AllConditionsTrue()
	{
		bool bRetAnd = true;
		bool bRetOr = false;

		for (int i = 0; i < conditions.Count(); i++)
		{
			int iConnectorIndex = i - 1;
			if ( iConnectorIndex == -1 )
			{
				if ( !conditions[i] )
					bRetAnd = false;
			}
			else if ( connectors[iConnectorIndex] == CONNECTOR_AND )
			{
				// If a single AND is false then all are false.
				if ( !conditions[i] )
					bRetAnd = false;
			}
			else if ( connectors[iConnectorIndex] == CONNECTOR_OR )
			{	
				// If a single OR is true then all are true.
				if ( conditions[i] )
					bRetOr = true;
			}
		}

		return bRetAnd || bRetOr;
	}
};

#define MAX_DEPTH 10
ConVar hl2r_entedit_debug( "hl2r_entedit_debug", "0" );

bool ParseKeyHeader(KeyValues *pVarKey, CEntityMapData *pEntData)
{
	// Iterate through all tokens in our key header
	ConditionsBlock_t headerConditions[MAX_DEPTH];
	int iDepth = 0;
	int iMaxDepth = 0;

	char szHeader[256];
	Q_strcpy( szHeader, pVarKey->GetName() );
	for ( char *pszToken = strtok( szHeader, " " ); pszToken != NULL; pszToken = strtok( NULL, " " ))
	{
		if (!Q_stricmp(pszToken, "("))
		{
			iDepth++;
			iMaxDepth++;

			continue;
		}

		if (!Q_stricmp(pszToken, ")"))
		{
			iDepth--;
			headerConditions[iDepth].conditions.AddToTail( headerConditions[iDepth+1].AllConditionsTrue() );
			continue;
		}

		if (!Q_stricmp(pszToken, "&&"))
		{
			headerConditions[iDepth].connectors.AddToTail(CONNECTOR_AND);
			continue;
		}

		if (!Q_stricmp(pszToken, "||"))
		{
			headerConditions[iDepth].connectors.AddToTail(CONNECTOR_OR);
			continue;
		}

		if (!Q_stricmp(pszToken, "key_replace"))
		{
			// Go to the next token to get our target value
			pszToken = strtok(NULL, " ");
			Q_strcpy(m_szTargetKey, pszToken);

			continue;
		}

		if ( GetKeyConditional(pszToken) )
		{
			headerConditions[iDepth].conditions.AddToTail(EvaluateKeyConditional(pszToken, pEntData));
			continue;
		}

		Error("Warning! Entedit header token [%s] not a valid keword!\n", pszToken);
	}

	if ( iDepth != 0 )
	{
		Error("Warning! Entedit header [%s] has open parenthesis!\n", pVarKey->GetName());
	}
	
	if ( hl2r_entedit_debug.GetBool() )
	{
		DevMsg("Key is: %s\n", pVarKey->GetName());
		for (int i = iMaxDepth; i >= 0; i-- )
		{
			DevMsg("Key block [%i] is:	", i);
			for (int j = 0; j < headerConditions[i].conditions.Count(); j++ )
			{
				if ( headerConditions[i].conditions[j] )
				{
					DevMsg("TRUE ");
				}
				else
				{
					DevMsg("FALSE ");
				}

				if ( j+1 != headerConditions[i].conditions.Count() )
				{
					if ( headerConditions[i].connectors[j] == CONNECTOR_AND )
					{
						DevMsg("&& ");
					}
					else if ( headerConditions[i].connectors[j] == CONNECTOR_OR )
					{
						DevMsg("|| ");
					}
				}
			}
			DevMsg("\n");
		}
		DevMsg("\n");
	}

	if ( headerConditions[0].conditions.Count() && !headerConditions[0].AllConditionsTrue() )
		return false;

	return true;
}

void ParseKey(KeyValues *pVarKey, CEntityMapData *pEntData)
{
	char szActualKeyName[MAPKEY_MAXLENGTH];
	if ( !pEntData->ExtractValue(m_szTargetKey, szActualKeyName) )
		return;

	char szTargetKeyName[MAPKEY_MAXLENGTH];
	Q_strcpy(szTargetKeyName, pVarKey->GetName());

	if ( Q_stricmp(szActualKeyName, szTargetKeyName) )
		return;

	char szTargetKeyValue[MAPKEY_MAXLENGTH];
	Q_strcpy(szTargetKeyValue, pVarKey->GetString());

	pEntData->SetValue( m_szTargetKey, szTargetKeyValue );
}

void Parse(KeyValues *pData, CEntityMapData *pEntData)
{
	for ( KeyValues *sub = pData->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
	{
		// If we contain a string, we're not a header.
		if ( !Q_stricmp( sub->GetString(), "") )
		{
			if ( ParseKeyHeader(sub, pEntData) )
				Parse(sub, pEntData);
		}
		else
		{
			ParseKey(sub, pEntData);
		}
	}
}

bool MapEntity_ModifyEntityData( CEntityMapData *pEntData ) 
{
#if 0
	KeyValues *m_pEntEditFile = new KeyValues( "entedit.txt" );
	m_pEntEditFile->LoadFromFile( filesystem, "scripts/entedit.txt", "GAME" );
	if ( m_pEntEditFile == NULL )
		return true;

	Parse(m_pEntEditFile, pEntData);

	m_pEntEditFile->deleteThis();
#endif
	return true;
}