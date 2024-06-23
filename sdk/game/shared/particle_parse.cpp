//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "filesystem.h"
#include <KeyValues.h>
#include "particle_parse.h"
#include "particles/particles.h"

#ifdef GAME_DLL
#include "te_effect_dispatch.h"
#include "networkstringtable_gamedll.h"
#else
#include "c_te_effect_dispatch.h"
#include "networkstringtable_clientdll.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PARTICLES_MANIFEST_FILE				"particles/particles_manifest.txt"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int GetAttachTypeFromString( const char *pszString )
{
	if ( !pszString || !pszString[0] )
		return -1;

	// If you add new attach types, you need to add them to this list
	static const char *pAttachmentNames[MAX_PATTACH_TYPES] =
	{
		"start_at_origin",		// PATTACH_ABSORIGIN = 0,
		"follow_origin",		// PATTACH_ABSORIGIN_FOLLOW,
		"start_at_customorigin",// PATTACH_CUSTOMORIGIN,
		"start_at_attachment",	// PATTACH_POINT,
		"follow_attachment",	// PATTACH_POINT_FOLLOW,
		"follow_rootbone",		// PATTACH_ROOTBONE_FOLLOW
	};

	for ( int i = 0; i < MAX_PATTACH_TYPES; i++ )
	{
		if ( FStrEq( pAttachmentNames[i], pszString ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : list - 
//-----------------------------------------------------------------------------
void GetParticleManifest( CUtlVector<CUtlString>& list )
{
	// Open the manifest file, and read the particles specified inside it
	KeyValues *manifest = new KeyValues( PARTICLES_MANIFEST_FILE );
	if ( manifest->LoadFromFile( filesystem, PARTICLES_MANIFEST_FILE, "GAME" ) )
	{
		for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
		{
			if ( !Q_stricmp( sub->GetName(), "file" ) )
			{
				list.AddToTail( sub->GetString() );
				continue;
			}

			Warning( "CParticleMgr::Init:  Manifest '%s' with bogus file type '%s', expecting 'file'\n", PARTICLES_MANIFEST_FILE, sub->GetName() );
		}
	}
	else
	{
		Warning( "PARTICLE SYSTEM: Unable to load manifest file '%s'\n", PARTICLES_MANIFEST_FILE );
	}

	manifest->deleteThis();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ParseParticleEffects( bool bLoadSheets, bool bPrecache )
{
	MEM_ALLOC_CREDIT();

	g_pParticleSystemMgr->ShouldLoadSheets( bLoadSheets );

	CUtlVector<CUtlString> files;
	GetParticleManifest( files );

	int nCount = files.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		g_pParticleSystemMgr->ReadParticleConfigFile( files[i], bPrecache, false );
	}

	g_pParticleSystemMgr->DecommitTempMemory();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ReloadParticleEffectsInList( IFileList *pFilesToReload )
{
	MEM_ALLOC_CREDIT();

	CUtlVector<CUtlString> files;
	GetParticleManifest( files );

	// CAB 2/17/11 Reload all the particles regardless (Fixes filename change exploits).
	bool bReloadAll = true;

	//int nCount = files.Count();
	//for ( int i = 0; i < nCount; ++i )
	//{
	//	// Skip the precache marker
	//	const char *pFile = files[i];
 //		if ( pFile[0] == '!' )
 //		{
 //			pFile++;
 //		}

	//	char szDX80Filename[MAX_PATH];
	//	V_strncpy( szDX80Filename, pFile, sizeof( szDX80Filename ) );
	//	V_StripExtension( pFile, szDX80Filename, sizeof( szDX80Filename ) );
	//	V_strncat( szDX80Filename, "_dx80.", sizeof( szDX80Filename ) );
	//	V_strncat( szDX80Filename, V_GetFileExtension( pFile ), sizeof( szDX80Filename ) );

	//	if ( pFilesToReload->IsFileInList( pFile ) || pFilesToReload->IsFileInList( szDX80Filename ) )
	//	{
	//		Msg( "Reloading all particle files due to pure settings.\n" );
	//		bReloadAll = true;
	//		break;
	//	}
	//}

	// Then check to see if we need to reload the map's particles
	const char *pszMapName = NULL;
#ifdef CLIENT_DLL
	pszMapName = engine->GetLevelName();	
#else
	pszMapName = STRING( gpGlobals->mapname );
#endif
	if ( pszMapName && pszMapName[0] )
	{
		char mapname[MAX_MAP_NAME];
		Q_FileBase( pszMapName, mapname, sizeof( mapname ) );
		Q_strlower( mapname );
		ParseParticleEffectsMap( mapname, true, pFilesToReload );
	}

	if ( bReloadAll )
	{
		ParseParticleEffects( true, true );
	}
	
	g_pParticleSystemMgr->DecommitTempMemory();
}

//-----------------------------------------------------------------------------
// Purpose: loads per-map manifest!
//-----------------------------------------------------------------------------
void ParseParticleEffectsMap( const char *pMapName, bool bLoadSheets, IFileList *pFilesToReload )
{
	MEM_ALLOC_CREDIT();

	CUtlVector<CUtlString> files;
	char szMapManifestFilename[MAX_PATH];

	szMapManifestFilename[0] = NULL;

	if ( pMapName && *pMapName )
	{
		V_snprintf( szMapManifestFilename, sizeof( szMapManifestFilename ), "maps/%s_particles.txt", pMapName );
	}

	// Open the manifest file, and read the particles specified inside it
	KeyValues *manifest = new KeyValues( szMapManifestFilename );
	if ( manifest->LoadFromFile( filesystem, szMapManifestFilename, "GAME" ) )
	{
		DevMsg( "Successfully loaded particle effects manifest '%s' for map '%s'\n", szMapManifestFilename, pMapName );
		for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
		{
			if ( !Q_stricmp( sub->GetName(), "file" ) )
			{
				// Ensure the particles are in the particles directory
				char szPath[ 512 ];
				Q_strncpy( szPath, sub->GetString(), sizeof( szPath ) );
				Q_StripFilename( szPath );
				char *pszPath = (szPath[0] == '!') ? &szPath[1] : &szPath[0];
				if ( pszPath && pszPath[0] && !Q_stricmp( pszPath, "particles" ) )
				{
					files.AddToTail( sub->GetString() );
					continue;
				}
				else
				{
					Warning( "CParticleMgr::LevelInit:  Manifest '%s' contains a particle file '%s' that's not under the particles directory. Custom particles must be placed in the particles directory.\n", szMapManifestFilename, sub->GetString() );
				}
			}
			else
			{
				Warning( "CParticleMgr::LevelInit:  Manifest '%s' with bogus file type '%s', expecting 'file'\n", szMapManifestFilename, sub->GetName() );
			}
		}
	}
	else
	{
		// Don't print a warning, and don't proceed any further if the file doesn't exist!
		return;
	}

	int nCount = files.Count();
	if ( !nCount )
	{
		return;
	}

	g_pParticleSystemMgr->ShouldLoadSheets( bLoadSheets );

	for ( int i = 0; i < nCount; ++i )
	{
		// If we've been given a list of particles to reload, only reload those.
		if ( !pFilesToReload || (pFilesToReload && pFilesToReload->IsFileInList( files[i] )) )
		{
			g_pParticleSystemMgr->ReadParticleConfigFile( files[i], true, true );
		}
	}

	g_pParticleSystemMgr->DecommitTempMemory();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PrecacheStandardParticleSystems( )
{
#ifdef GAME_DLL
	// Now add each particle system name to the network string pool, so we can send string_t's 
	// down to the client instead of full particle system names.
	for ( int i = 0; i < g_pParticleSystemMgr->GetParticleSystemCount(); i++ )
	{
		const char *pParticleSystemName = g_pParticleSystemMgr->GetParticleSystemNameFromIndex(i);
		CParticleSystemDefinition *pParticleSystem = g_pParticleSystemMgr->FindParticleSystem( pParticleSystemName );
		if ( pParticleSystem->ShouldAlwaysPrecache() )
		{
			PrecacheParticleSystem( pParticleSystemName );
		}
	}
#endif
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, bool bResetAllParticlesOnEntity )
{
	int iAttachment = -1;
	if ( pEntity && pEntity->GetBaseAnimating() )
	{
		// Find the attachment point index
		iAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pszAttachmentName );
		if ( iAttachment <= 0 )
		{
			Warning("Model '%s' doesn't have attachment '%s' to attach particle system '%s' to.\n", STRING(pEntity->GetBaseAnimating()->GetModelName()), pszAttachmentName, pszParticleName );
			return;
		}
	}

	DispatchParticleEffect( pszParticleName, iAttachType, pEntity, iAttachment, bResetAllParticlesOnEntity );
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, int iAttachmentPoint, bool bResetAllParticlesOnEntity )
{
	CEffectData	data;

	data.m_nHitBox = GetParticleSystemIndex( pszParticleName );
	if ( pEntity )
	{
#ifdef CLIENT_DLL
		data.m_hEntity = pEntity;
#else
		data.m_nEntIndex = pEntity->entindex();
#endif
		data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
		data.m_vOrigin = pEntity->GetAbsOrigin();
	}
	data.m_nDamageType = iAttachType;
	data.m_nAttachmentIndex = iAttachmentPoint;

	if ( bResetAllParticlesOnEntity )
	{
		data.m_fFlags |= PARTICLE_DISPATCH_RESET_PARTICLES;
	}

#ifdef GAME_DLL
	if ( ( data.m_fFlags & PARTICLE_DISPATCH_FROM_ENTITY ) != 0 &&
		 ( iAttachType == PATTACH_ABSORIGIN_FOLLOW || iAttachType == PATTACH_POINT_FOLLOW || iAttachType == PATTACH_ROOTBONE_FOLLOW ) )
	{
		CBroadcastRecipientFilter filter;
		DispatchEffect( "ParticleEffect", data, filter );
	}
	else
#endif
	{
		DispatchEffect( "ParticleEffect", data );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, Vector vecColor1, Vector vecColor2, bool bUseColors, bool bResetAllParticlesOnEntity )
{
	int iAttachment = -1;
	if ( pEntity && pEntity->GetBaseAnimating() )
	{
		// Find the attachment point index
		iAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pszAttachmentName );
		if ( iAttachment <= 0 )
		{
			Warning("Model '%s' doesn't have attachment '%s' to attach particle system '%s' to.\n", STRING(pEntity->GetBaseAnimating()->GetModelName()), pszAttachmentName, pszParticleName );
			return;
		}
	}

	CEffectData	data;

	data.m_nHitBox = GetParticleSystemIndex( pszParticleName );
	if ( pEntity )
	{
#ifdef CLIENT_DLL
		data.m_hEntity = pEntity;
#else
		data.m_nEntIndex = pEntity->entindex();
#endif
		data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
		data.m_vOrigin = pEntity->GetAbsOrigin();
	}
	data.m_nDamageType = iAttachType;
	data.m_nAttachmentIndex = iAttachment;

	if ( bResetAllParticlesOnEntity )
	{
		data.m_fFlags |= PARTICLE_DISPATCH_RESET_PARTICLES;
	}

	if ( bUseColors )
	{
		data.m_bCustomColors = true;
		data.m_CustomColors.m_vecColor1 = vecColor1;
		data.m_CustomColors.m_vecColor2 = vecColor2;
	}

#ifdef GAME_DLL
	if ( ( data.m_fFlags & PARTICLE_DISPATCH_FROM_ENTITY ) != 0 &&
		 ( iAttachType == PATTACH_ABSORIGIN_FOLLOW || iAttachType == PATTACH_POINT_FOLLOW || iAttachType == PATTACH_ROOTBONE_FOLLOW ) )
	{
		CReliableBroadcastRecipientFilter filter;
		DispatchEffect( "ParticleEffect", data, filter );
	}
	else
#endif
	{
		DispatchEffect( "ParticleEffect", data );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchParticleEffect( int iEffectIndex, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity )
{
	CEffectData	data;

	data.m_nHitBox = iEffectIndex;
	data.m_vOrigin = vecOrigin;
	data.m_vStart = vecStart;
	data.m_vAngles = vecAngles;

	if ( pEntity )
	{
#ifdef CLIENT_DLL
		data.m_hEntity = pEntity;
#else
		data.m_nEntIndex = pEntity->entindex();
#endif
		data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
		data.m_nDamageType = PATTACH_CUSTOMORIGIN;
	}
	else
	{
#ifdef CLIENT_DLL
		data.m_hEntity = NULL;
#else
		data.m_nEntIndex = 0;
#endif
	}

	DispatchEffect( "ParticleEffect", data );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, Vector vecColor1, Vector vecColor2, bool bUseColors, CBaseEntity *pEntity, int iAttachType )
{
	int iEffectIndex = GetParticleSystemIndex( pszParticleName );

	CEffectData	data;

	data.m_nHitBox = iEffectIndex;
	data.m_vOrigin = vecOrigin;
	data.m_vAngles = vecAngles;

	if ( pEntity )
	{
#ifdef CLIENT_DLL
		data.m_hEntity = pEntity;
#else
		data.m_nEntIndex = pEntity->entindex();
#endif
		data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
		data.m_nDamageType = PATTACH_CUSTOMORIGIN;
	}
	else
	{
#ifdef CLIENT_DLL
		data.m_hEntity = NULL;
#else
		data.m_nEntIndex = 0;
#endif
	}

	if ( bUseColors )
	{
		data.m_bCustomColors = true;
		data.m_CustomColors.m_vecColor1 = vecColor1;
		data.m_CustomColors.m_vecColor2 = vecColor2;
	}

	DispatchEffect( "ParticleEffect", data );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity )
{
	int iIndex = GetParticleSystemIndex( pszParticleName );
	DispatchParticleEffect( iIndex, vecOrigin, vecOrigin, vecAngles, pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Yet another overload, lets us supply vecStart
//-----------------------------------------------------------------------------
void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity )
{
	int iIndex = GetParticleSystemIndex( pszParticleName );
	DispatchParticleEffect( iIndex, vecOrigin, vecStart, vecAngles, pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void StopParticleEffects( CBaseEntity *pEntity )
{
	CEffectData	data;

	if ( pEntity )
	{
#ifdef CLIENT_DLL
		data.m_hEntity = pEntity;
#else
		data.m_nEntIndex = pEntity->entindex();
#endif
	}

#ifdef GAME_DLL
	CReliableBroadcastRecipientFilter filter;
	DispatchEffect( "ParticleEffectStop", data, filter );
#else
	DispatchEffect( "ParticleEffectStop", data );
#endif
}

#ifndef CLIENT_DLL

	extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

	ConVar particle_test_file( "particle_test_file", "", FCVAR_CHEAT, "Name of the particle system to dynamically spawn" );
	ConVar particle_test_attach_mode( "particle_test_attach_mode", "follow_attachment", FCVAR_CHEAT, "Possible Values: 'start_at_attachment', 'follow_attachment', 'start_at_origin', 'follow_origin'" );
	ConVar particle_test_attach_attachment( "particle_test_attach_attachment", "0", FCVAR_CHEAT, "Attachment index for attachment mode" );

	void Particle_Test_Start( CBasePlayer* pPlayer, const char *name, bool bStart )
	{
		if ( !pPlayer )
			return;

		int iAttachType = GetAttachTypeFromString( particle_test_attach_mode.GetString() );

		if ( iAttachType < 0 )
		{
			Warning( "Invalid attach type specified for particle_test in cvar 'particle_test_attach_mode.\n" );
			return;
		}

		int iAttachmentIndex = particle_test_attach_attachment.GetInt();

		const char *pszParticleFile = particle_test_file.GetString();

		CBaseEntity *pEntity = NULL;
		while ( (pEntity = GetNextCommandEntity( pPlayer, name, pEntity )) != NULL )
		{
			/* 
			Fire the test particle system on this entity
			*/

			DispatchParticleEffect( 
				pszParticleFile,
				(ParticleAttachment_t)iAttachType,
				pEntity,
				iAttachmentIndex,
				true );				// stops existing particle systems
		}
	}

	void CC_Particle_Test_Start( const CCommand& args )
	{
		Particle_Test_Start( UTIL_GetCommandClient(), args[1], true );
	}
	static ConCommand particle_test_start("particle_test_start", CC_Particle_Test_Start, "Dispatches the test particle system with the parameters specified in particle_test_file,\n particle_test_attach_mode and particle_test_attach_param on the entity the player is looking at.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);


	void Particle_Test_Stop( CBasePlayer* pPlayer, const char *name, bool bStart )
	{
		if ( !pPlayer )
			return;

		CBaseEntity *pEntity = NULL;
		while ( (pEntity = GetNextCommandEntity( pPlayer, name, pEntity )) != NULL )
		{
			//Stop all particle systems on the selected entity
			DispatchParticleEffect( "", PATTACH_ABSORIGIN, pEntity, 0, true );
		}
	}

	void CC_Particle_Test_Stop( const CCommand& args )
	{
		Particle_Test_Stop( UTIL_GetCommandClient(), args[1], false );
	}
	static ConCommand particle_test_stop("particle_test_stop", CC_Particle_Test_Stop, "Stops all particle systems on the selected entities.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

#endif	//!CLIENT_DLL

#if defined( CLIENT_DLL ) && defined( STAGING_ONLY )
	
	void CC_DispatchParticle( const CCommand& args )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pLocalPlayer )
			return;

		if ( args.ArgC() < 2 )
		{
			DevMsg( "Use: dispatch_particle {particle_name} {surface_offset_distance}\n" );
			return;
		}

		float flSurfaceOffsetDistance = 0.f;
		if ( args.ArgC() == 3 )
		{
			flSurfaceOffsetDistance = atof( args[2] );
		}

		Vector vForward;
		pLocalPlayer->GetVectors( &vForward, NULL, NULL );
		trace_t tr;
		UTIL_TraceLine( pLocalPlayer->EyePosition(), pLocalPlayer->EyePosition() + vForward * 3000, MASK_SOLID_BRUSHONLY, NULL, &tr );
	
		Vector vTargetDeathPos = tr.endpos;
		DispatchParticleEffect( args[1], vTargetDeathPos + flSurfaceOffsetDistance * tr.plane.normal, vec3_angle );
	}

	static ConCommand dispatch_particle( "dispatch_particle", CC_DispatchParticle, "Dispatch specified particle effect 50 units away from the lookat surface normal.\n\tArguments: {particle_name} {surface_offset_distance}", FCVAR_CHEAT );

#endif // CLIENT_DLL && STAGING_ONLY
