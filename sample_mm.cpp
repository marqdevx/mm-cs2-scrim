/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Sample Plugin
 * Written by AlliedModders LLC.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 *
 * This sample plugin is public domain.
 */

#include <stdio.h>
#include "sample_mm.h"
#include "iserver.h"
#include "KeyValues.h"

#include "tier0/memdbgon.h"
#include "convar.h"

#include "iserver.h"

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK4_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, CPlayerSlot, bool, const char *, uint64);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason, const char *, uint64, const char *);
SH_DECL_HOOK4_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, CPlayerSlot, char const *, int, uint64);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, CPlayerSlot );
SH_DECL_HOOK6_void(IServerGameClients, OnClientConnected, SH_NOATTRIB, 0, CPlayerSlot, const char*, uint64, const char *, const char *, bool);
SH_DECL_HOOK6(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, CPlayerSlot, const char*, uint64, const char *, bool, CBufferString *);
SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

SH_DECL_HOOK2_void( IServerGameClients, ClientCommand, SH_NOATTRIB, 0, CPlayerSlot, const CCommand & );
SH_DECL_HOOK3_void(ICvar, DispatchConCommand, SH_NOATTRIB, 0, ConCommandHandle, const CCommandContext &, const CCommand &);

SamplePlugin g_SamplePlugin;
IServerGameDLL *server = NULL;
IServerGameClients *gameclients = NULL;
IVEngineServer *engine = NULL;
IGameEventManager2 *gameevents = NULL;
ICvar *icvar = NULL;


class CCSPlayerController;

#define SCMD_CALLBACK(name) META_RES name(CCSPlayerController *controller, const CCommand *args)

#define SCMD_CONSOLE_PREFIX      "kz_"
#define SCMD_CHAT_SILENT_TRIGGER '/'
#define SCMD_CHAT_TRIGGER        '!'
#define SCMD_MAX_CMDS            512


namespace scmd
{
	typedef SCMD_CALLBACK(Callback_t);
	bool RegisterCmd(const char *name, Callback_t *callback, bool hidden = false);
	bool UnregisterCmd(const char *name);

	META_RES OnClientCommand(CPlayerSlot &slot, const CCommand &args);
	META_RES OnDispatchConCommand(ConCommandHandle cmd, const CCommandContext &ctx, const CCommand &args);
} // namespace scmd


// Should only be called within the active game loop (i e map should be loaded and active)
// otherwise that'll be nullptr!
CGlobalVars *GetGameGlobals()
{
	INetworkGameServer *server = g_pNetworkServerService->GetIGameServer();

	if(!server)
		return nullptr;

	return g_pNetworkServerService->GetIGameServer()->GetGlobals();
}

#if 0
// Currently unavailable, requires hl2sdk work!
ConVar sample_cvar("sample_cvar", "42", 0);
#endif


CON_COMMAND_F(sample_command, "Sample command", FCVAR_NONE)
{
	META_CONPRINTF( "Sample command called by %d. Command: %s\n", context.GetPlayerSlot(), args.GetCommandString() );
		
	char *commandString = const_cast<char *>(args.GetCommandString());
	const char *firstSpace = strchr(commandString, ' ');

	if (firstSpace == nullptr)
	{
		return;
	}

	const char *command = firstSpace + 1;
	engine->ServerCommand(command);
/*
	FORMAT_STRING(command);
	CBroadcastRecipientFilter *filter = new CBroadcastRecipientFilter;
	ClientPrintFilter(filter, HUD_PRINTTALK, command, "", "", "", "");
	delete filter;
	*/
}

CON_COMMAND_F(pracc, "Pracc command", FCVAR_NONE)
{
	META_CONPRINTF( "Pracc command called by %d. Command: %s\n", context.GetPlayerSlot(), args.GetCommandString() );
	engine->ServerCommand("Say practice mode enabled");
	engine->ServerCommand("exec pracc");
}

CON_COMMAND_F(scrim, "Scrim command", FCVAR_NONE)
{
	META_CONPRINTF( "Scrim command called by %d. Command: %s\n", context.GetPlayerSlot(), args.GetCommandString() );
	engine->ServerCommand("Say scrim mode enabled");
	engine->ServerCommand("exec scrim");
}

void SamplePlugin::Hook_DispatchConCommand(ConCommandHandle cmdHandle, const CCommandContext& ctx, const CCommand& args)
{
	auto iCommandPlayerSlot = ctx.GetPlayerSlot();

	bool bSay = !V_strcmp(args.Arg(0), "say");
	bool bTeamSay = !V_strcmp(args.Arg(0), "say_team");

	if (iCommandPlayerSlot != -1 && (bSay || bTeamSay))
	{
		
		bool bAdminChat = bTeamSay && *args[1] == '@';
		bool bSilent = *args[1] == '/' || bAdminChat;
		bool bCommand = *args[1] == '!' || *args[1] == '/' || *args[1] == '.' ;
		
		

		char *pszMessage = (char *)(args.ArgS() + 2);
		
		META_CONPRINTF("Player %i, command: %s", iCommandPlayerSlot, pszMessage );
	}

	RETURN_META(MRES_IGNORED);

	return;
}

PLUGIN_EXPOSE(SamplePlugin, g_SamplePlugin);
bool SamplePlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2ServerConfig, ISource2ServerConfig, SOURCE2SERVERCONFIG_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameEntities, ISource2GameEntities, SOURCE2GAMEENTITIES_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameClients, IServerGameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);
	
	//GET_V_IFACE_ANY(GetEngineFactory, g_gameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
	//GET_V_IFACE_ANY(GetEngineFactory, g_pGameTypes, IGameTypes, GAMETYPES_INTERFACE_VERSION);
	//GET_V_IFACE_ANY(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	

	// Currently doesn't work from within mm side, use GetGameGlobals() in the mean time instead
	// gpGlobals = ismm->GetCGlobals();

	// Required to get the IMetamodListener events
	g_SMAPI->AddListener( this, this );

	META_CONPRINTF( "Starting plugin.\n" );

	SH_ADD_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &SamplePlugin::Hook_GameFrame), true);
	SH_ADD_HOOK(IServerGameClients, ClientActive, g_pSource2GameClients, SH_MEMBER(this, &SamplePlugin::Hook_ClientActive), true);
	SH_ADD_HOOK(IServerGameClients, ClientDisconnect, g_pSource2GameClients, SH_MEMBER(this, &SamplePlugin::Hook_ClientDisconnect), true);
	SH_ADD_HOOK(IServerGameClients, ClientPutInServer, g_pSource2GameClients, SH_MEMBER(this, &SamplePlugin::Hook_ClientPutInServer), true);
	SH_ADD_HOOK(IServerGameClients, OnClientConnected, g_pSource2GameClients, SH_MEMBER(this, &SamplePlugin::Hook_OnClientConnected), false);
	SH_ADD_HOOK(IServerGameClients, ClientConnect, g_pSource2GameClients, SH_MEMBER(this, &SamplePlugin::Hook_ClientConnect), false);
	SH_ADD_HOOK(IServerGameClients, ClientCommand, g_pSource2GameClients, SH_MEMBER(this, &SamplePlugin::Hook_ClientCommand), false);

	SH_ADD_HOOK(ICvar, DispatchConCommand, g_pCVar, SH_MEMBER(this, &SamplePlugin::Hook_DispatchConCommand), false);
	

	META_CONPRINTF( "All hooks started!\n" );

	g_pCVar = icvar;
	ConVar_Register( FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL );

	return true;
}




bool SamplePlugin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &SamplePlugin::Hook_GameFrame), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &SamplePlugin::Hook_ClientActive), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, gameclients, SH_MEMBER(this, &SamplePlugin::Hook_ClientDisconnect), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientPutInServer, gameclients, SH_MEMBER(this, &SamplePlugin::Hook_ClientPutInServer), true);
	SH_REMOVE_HOOK(IServerGameClients, OnClientConnected, gameclients, SH_MEMBER(this, &SamplePlugin::Hook_OnClientConnected), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientConnect, gameclients, SH_MEMBER(this, &SamplePlugin::Hook_ClientConnect), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientCommand, gameclients, SH_MEMBER(this, &SamplePlugin::Hook_ClientCommand), false);

	SH_REMOVE_HOOK(ICvar, DispatchConCommand, g_pCVar, SH_MEMBER(this, &SamplePlugin::Hook_DispatchConCommand), false);

	return true;
}

void SamplePlugin::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

void SamplePlugin::Hook_ClientActive( CPlayerSlot slot, bool bLoadGame, const char *pszName, uint64 xuid )
{
	META_CONPRINTF( "Hook_ClientActive(%d, %d, \"%s\", %d)\n", slot, bLoadGame, pszName, xuid );
}

void SamplePlugin::Hook_ClientCommand( CPlayerSlot slot, const CCommand &args )
{
	META_CONPRINTF( "Hook_ClientCommand(%d, \"%s\")\n", slot, args.GetCommandString() );

	/*
	const char *commandName = g_pCVar->GetCommand(cmd)->GetName();
	if (!V_stricmp(commandName, "say") || !V_stricmp(commandName, "say_team"))
	{
		META_CONPRINTF( "Say command executed (%d, \"%s\")\n", slot, args.GetCommandString() );
	}
	*/
}

void SamplePlugin::Hook_ClientSettingsChanged( CPlayerSlot slot )
{
	META_CONPRINTF( "Hook_ClientSettingsChanged(%d)\n", slot );
}

void SamplePlugin::Hook_OnClientConnected( CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, const char *pszAddress, bool bFakePlayer )
{
	META_CONPRINTF( "Hook_OnClientConnected(%d, \"%s\", %d, \"%s\", \"%s\", %d)\n", slot, pszName, xuid, pszNetworkID, pszAddress, bFakePlayer );
}

bool SamplePlugin::Hook_ClientConnect( CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, bool unk1, CBufferString *pRejectReason )
{
	META_CONPRINTF( "Hook_ClientConnect(%d, \"%s\", %d, \"%s\", %d, \"%s\")\n", slot, pszName, xuid, pszNetworkID, unk1, pRejectReason->ToGrowable()->Get() );

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void SamplePlugin::Hook_ClientPutInServer( CPlayerSlot slot, char const *pszName, int type, uint64 xuid )
{
	META_CONPRINTF( "Hook_ClientPutInServer(%d, \"%s\", %d, %d)\n", slot, pszName, type, xuid );
}

void SamplePlugin::Hook_ClientDisconnect( CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID )
{
	META_CONPRINTF( "Hook_ClientDisconnect(%d, %d, \"%s\", %d, \"%s\")\n", slot, reason, pszName, xuid, pszNetworkID );
}

void SamplePlugin::Hook_GameFrame( bool simulating, bool bFirstTick, bool bLastTick )
{
	/**
	 * simulating:
	 * ***********
	 * true  | game is ticking
	 * false | game is not ticking
	 */
}

void SamplePlugin::OnLevelInit( char const *pMapName,
									 char const *pMapEntities,
									 char const *pOldLevel,
									 char const *pLandmarkName,
									 bool loadGame,
									 bool background )
{
	META_CONPRINTF("OnLevelInit(%s)\n", pMapName);
}

void SamplePlugin::OnLevelShutdown()
{
	META_CONPRINTF("OnLevelShutdown()\n");
}

bool SamplePlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool SamplePlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *SamplePlugin::GetLicense()
{
	return "Public Domain";
}

const char *SamplePlugin::GetVersion()
{
	return "1.0.0.0";
}

const char *SamplePlugin::GetDate()
{
	return __DATE__;
}

const char *SamplePlugin::GetLogTag()
{
	return "SAMPLE";
}

const char *SamplePlugin::GetAuthor()
{
	return "AlliedModders LLC";
}

const char *SamplePlugin::GetDescription()
{
	return "Sample basic plugin";
}

const char *SamplePlugin::GetName()
{
	return "Sample Plugin";
}

const char *SamplePlugin::GetURL()
{
	return "http://www.sourcemm.net/";
}
