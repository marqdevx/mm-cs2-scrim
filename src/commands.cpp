/**
 * =============================================================================
 * CS2Fixes
 * Copyright (C) 2023 Source2ZE
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "detours.h"
#include "common.h"
#include "utlstring.h"
#include "recipientfilters.h"
#include "commands.h"
#include "utils/entity.h"
#include "entity/cbaseentity.h"
#include "entity/ccsweaponbase.h"
#include "entity/ccsplayerpawn.h"
#include "entity/ccsplayercontroller.h"
#include "entity/ccsplayerpawnbase.h"

#include "entity/cbasemodelentity.h"
#include "playermanager.h"
#include "adminsystem.h"
#include "ctimer.h"

#include "tier0/memdbgon.h"

extern CEntitySystem *g_pEntitySystem;
extern IVEngineServer2* g_pEngineServer2;
extern int g_targetPawn;
extern int g_targetController;

extern bool practiceMode;
extern bool no_flash_mode;

void ParseChatCommand(const char *pMessage, CCSPlayerController *pController)
{
	if (!pController)
		return;

	CCommand args;
	args.Tokenize(pMessage);

	uint16 index = g_CommandList.Find(hash_32_fnv1a_const(args[0]));

	if (g_CommandList.IsValidIndex(index))
	{
		g_CommandList[index](args, pController);
	}

}

void ClientPrintAll(int hud_dest, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[256];
	V_vsnprintf(buf, sizeof(buf), msg, args);

	va_end(args);

	addresses::UTIL_ClientPrintAll(hud_dest, buf, nullptr, nullptr, nullptr, nullptr);
}

void ClientPrint(CBasePlayerController *player, int hud_dest, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[256];
	V_vsnprintf(buf, sizeof(buf), msg, args);

	va_end(args);

	addresses::ClientPrint(player, hud_dest, buf, nullptr, nullptr, nullptr, nullptr);
}

/* CVARS */
// CONVAR_TODO
bool g_bEnableCoach = true;
CON_COMMAND_F(cs2scrim_coach, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY)
{
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableCoach);
	else
		g_bEnableCoach = V_StringToBool(args[1], false);
}

bool g_bEnablePractice = true;
CON_COMMAND_F(cs2scrim_pracc, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY){
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnablePractice);
	else
		g_bEnablePractice = V_StringToBool(args[1], false);
}

bool g_bEnableScrim = true;
CON_COMMAND_F(cs2scrim_scrim, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY){
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableScrim);
	else
		g_bEnableScrim = V_StringToBool(args[1], false);
}

bool g_bEnableRecord = true;
CON_COMMAND_F(cs2scrim_demo, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY){
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableRecord);
	else
		g_bEnableRecord = V_StringToBool(args[1], false);
}

bool g_bEnableRestore = true;
CON_COMMAND_F(cs2scrim_restore, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY){
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableRestore);
	else
		g_bEnableRestore = V_StringToBool(args[1], false);
}

bool g_bEnableRcon = true;
CON_COMMAND_F(cs2scrim_rcon, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY){
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableRcon);
	else
		g_bEnableRcon = V_StringToBool(args[1], false);
}

bool g_bEnablePause = true;
CON_COMMAND_F(cs2scrim_pause, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY)
{
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnablePause);
	else
		g_bEnablePause = V_StringToBool(args[1], false);
}

bool g_bEnablePraccSpawn = true;
CON_COMMAND_F(cs2scrim_practice_spawn, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY)
{
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnablePraccSpawn);
	else
		g_bEnablePraccSpawn = V_StringToBool(args[1], false);
}

bool g_bEnableBan = true;
CON_COMMAND_F(cs2scrim_admin_ban_enable, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY)
{
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableBan);
	else
		g_bEnableBan = V_StringToBool(args[1], false);
}

bool g_bEnableKick = true;
CON_COMMAND_F(cs2scrim_admin_kick, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY)
{
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableKick);
	else
		g_bEnableKick = V_StringToBool(args[1], false);
}

bool g_bEnableGag = true;
CON_COMMAND_F(cs2scrim_admin_gag, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY)
{
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableGag);
	else
		g_bEnableGag = V_StringToBool(args[1], false);
}

bool g_bEnableSlay = true;
CON_COMMAND_F(cs2scrim_admin_slay, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY)
{
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableSlay);
	else
		g_bEnableSlay = V_StringToBool(args[1], false);
}

bool g_bEnableTeleport = true;
CON_COMMAND_F(cs2scrim_admin_teleport, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY)
{
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableTeleport);
	else
		g_bEnableTeleport = V_StringToBool(args[1], false);
}

bool g_bEnableTeamControl = true;
CON_COMMAND_F(cs2scrim_admin_team, "Whether to enable weapon commands", FCVAR_LINKED_CONCOMMAND | FCVAR_SPONLY)
{
	if (args.ArgC() < 2)
		Msg("%s %i\n", args[0], g_bEnableTeamControl);
	else
		g_bEnableTeamControl = V_StringToBool(args[1], false);
}
/* Player commands */
CON_COMMAND_CHAT(myuid, "test")
{
	if (!player)
		return;

	int iPlayer = player->GetPlayerSlot();

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Your userid is %i, slot: %i, retrieved slot: %i", g_pEngineServer2->GetPlayerUserId(iPlayer).Get(), iPlayer, g_playerManager->GetSlotFromUserId(g_pEngineServer2->GetPlayerUserId(iPlayer).Get()));
}


CON_COMMAND_CHAT(noflash, "noflash"){

	no_flash_mode = !no_flash_mode;

	if (!player)
		return;
	
	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Flash mode set to \04%i", no_flash_mode);
}

CON_COMMAND_CHAT(test, "test")
{
	if (!player)
		return;


	//CBasePlayerPawn *pPawn = player->m_hPawn();
	//pPawn->m_flFlashMaxAlpha() = 0.5f;
//	player->GetPawn()->m_flFlashMaxAlpha = 0.5f;

	CCSPlayerController* pPlayer = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(player->GetPlayerSlot()+1));
	
	CCSPlayerPawnBase* cPlayerBase = (CCSPlayerPawnBase*)pPlayer->GetPawn();
	
	
	cPlayerBase->m_flFlashMaxAlpha = 2;
	ClientPrint(pPlayer, HUD_PRINTTALK, "test %i flash: %lu", cPlayerBase->m_ArmorValue(), cPlayerBase->m_flFlashMaxAlpha());
	//pPawn->m_ArmorValue() = (int32_t)20;

	int iPlayer = player->GetPlayerSlot();

}

bool match_paused = false;
bool ct_ready = true;
bool t_ready = true;

CON_COMMAND_CHAT(pause, "Request pause")
{
	if(!g_bEnablePause)
		return;

	if (!player)
		return;

	int iPlayer = player->GetPlayerSlot();

	CBasePlayerController* pPlayer = (CBasePlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(player->GetPlayerSlot() + 1));

	g_pEngineServer2->ServerCommand("mp_pause_match");

	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"%s requested a pause", player->GetPlayerName());

	match_paused = true;
	ct_ready = false;
	t_ready = false;	
}

CON_COMMAND_CHAT(unpause, "Request unpause")
{
	if(!g_bEnablePause)
		return;

	if (!player)
		return;

	if(!match_paused)
		return;
	
	CBasePlayerController* pPlayer = (CBasePlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(player->GetPlayerSlot() + 1));
	
	int teamSide = pPlayer->m_iTeamNum();
	if( teamSide == CS_TEAM_T && !t_ready){
		t_ready = true;
	}else if( teamSide == CS_TEAM_CT && !ct_ready){
		ct_ready = true;
	}

	if(ct_ready && !t_ready){
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"CT ready, type .unpause");
		return;
	}else if(!ct_ready && t_ready){
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"T ready, type .unpause");
		return;
	}

	match_paused = false;
	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Match \2unpaused");
	g_pEngineServer2->ServerCommand("mp_unpause_match");
}

CON_COMMAND_CHAT(spawn, "teleport to desired spawn")
{
	if (!g_bEnablePraccSpawn)
		return;

	if (!player)
		return;
	
	if (!practiceMode){
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Only available on practice mode");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Usage: !spawn <spawn number>");
		return;
	}

	char teamName[256];
	int target_team_number = CS_TEAM_NONE;

	if(args.ArgC() > 2){
		char team_id_input[256];
		V_snprintf(team_id_input, sizeof(team_id_input), "%s", args[2]);
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"%s ", args[2]);
		if((std::string)team_id_input == "t"){
			target_team_number = CS_TEAM_T;
		}else if((std::string)team_id_input == "ct"){
			target_team_number = CS_TEAM_CT;
		}else{
			ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Usage: !spawn <spawn number> <ct/t> ");
		}
	}else{
		target_team_number = player->m_iTeamNum;
	}

	if(target_team_number == CS_TEAM_SPECTATOR){
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You cannot teleport in spectator!");
		return;
	}
	if(target_team_number == CS_TEAM_SPECTATOR){
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You cannot teleport in spectator!");
		return;
	}
	if(target_team_number == CS_TEAM_T){
		V_snprintf(teamName, sizeof(teamName), "info_player_terrorist");
	}else{
		V_snprintf(teamName, sizeof(teamName), "info_player_counterterrorist");
	}

	//Count spawnpoints (info_player_counterterrorist & info_player_terrorist)
	SpawnPoint* spawn = nullptr;
	CUtlVector<SpawnPoint*> spawns;

	int minimum_priority = 1;
	while (nullptr != (spawn = (SpawnPoint*)UTIL_FindEntityByClassname(spawn, teamName)))
	{
		if (spawn->m_bEnabled() && spawn->m_iPriority() < minimum_priority)
		{
			minimum_priority = spawn->m_iPriority();
			// ClientPrint(player, HUD_PRINTTALK, "Spawn %i: %f / %f / %f", spawns.Count(), spawn->GetAbsOrigin().x, spawn->GetAbsOrigin().y, spawn->GetAbsOrigin().z);
			//spawns.AddToTail(spawn);
		}
	}

	while (nullptr != (spawn = (SpawnPoint*)UTIL_FindEntityByClassname(spawn, teamName)))
	{
		if (spawn->m_bEnabled() && spawn->m_iPriority() == minimum_priority)
		{
			// ClientPrint(player, HUD_PRINTTALK, "Spawn %i: %f / %f / %f", spawns.Count(), spawn->GetAbsOrigin().x, spawn->GetAbsOrigin().y, spawn->GetAbsOrigin().z);
			spawns.AddToTail(spawn);
		}
	}

	//Pick and get position of random spawnpoint
	//Spawns selection from 1 to spawns.Count()
	int targetSpawn = atoi(args[1]) - 1;
	if (targetSpawn < 0) targetSpawn = 0;
	
	int spawnIndex = targetSpawn % spawns.Count();
	Vector spawnpos = spawns[spawnIndex]->GetAbsOrigin();
	int spawn_priority = spawns[spawnIndex]->m_iPriority();

	//Here's where the mess starts
	CBasePlayerPawn *pPawn = player->GetPawn();
	if (!pPawn)
	{
		return;
	}
	if (pPawn->m_iHealth() <= 0)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You cannot teleport when dead!");
		return;
	}

	int totalSpawns = spawns.Count();

	pPawn->SetAbsOrigin(spawnpos);

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You have been teleported to spawn. %i/%i priority:%i", spawnIndex +1, totalSpawns, spawn_priority);			
}

CUtlVector <CCSPlayerController*> coaches;

void print_coaches(){
	if (coaches.Count() < 1) return;
	
	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"\5%i \1active \5coaches", coaches.Count());
	FOR_EACH_VEC(coaches,i){
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Coach %i: \5%s", i+1, coaches[i]->GetPlayerName());
	}
}

CON_COMMAND_CHAT(coach, "Request slot coach")
{
	if(!g_bEnableCoach)
		return;

	if (!player)
		return;
	
	int iPlayer = player->GetPlayerSlot();

	player->m_pInGameMoneyServices->m_iAccount = 0;

	//Check it is not existing already
	FOR_EACH_VEC(coaches,i){
		if(coaches[i]->GetPlayerSlot() == iPlayer){	
			ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You are already a coach type \4.uncoach \1to be a player");
			return;
		}
	}

	coaches.AddToTail(player);

	int target_team_number = CS_TEAM_SPECTATOR;

	if(args.ArgC() > 1){
		char team_id_input[256];
		V_snprintf(team_id_input, sizeof(team_id_input), "%s", args[1]);

		if((std::string)team_id_input == "t"){
			target_team_number = CS_TEAM_T;
		}else if((std::string)team_id_input == "ct"){
			target_team_number = CS_TEAM_CT;
		}else{
			ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Usage: .coach <ct/t> ");
		}
	}else{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Coach enabled, type \4.uncoach \1to cancel");
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You are on spectator mode, choose \4.ct \1or \4.t");
	}

	print_coaches();
	
	CHandle<CCSPlayerController> hController = player->GetHandle();

	// Gotta do this on the next frame...
	new CTimer(0.0f, false, false, [hController, target_team_number]()
	{
		CCSPlayerController *pController = hController.Get();

		if (!pController)
			return;
		
		pController->ChangeTeam(target_team_number);
		pController->m_szClan = "Coaching:";
		return;
	});
}

//Todo, unify different aliases
CON_COMMAND_CHAT(uncoach, "Undo slot coach")
{
	if(!g_bEnableCoach)
		return;

	if (!player)
		return;
	
	int iPlayer = player->GetPlayerSlot();
	CBasePlayerController *pTarget = (CBasePlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(iPlayer + 1));

	if (!pTarget)
		return;

	//Check it is not existing already
	FOR_EACH_VEC(coaches,i){
		if(coaches[i]->GetPlayerSlot() == iPlayer){
			coaches.Remove(i);
			player->m_pInGameMoneyServices->m_iAccount = 0;
			player->m_szClan = "";
			ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You are no longer set as \4coach\1");
			print_coaches();
			return;
		}
	}
	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You haven't set as \4coach\1 yet");
}

CON_COMMAND_CHAT(ct, "Switch to CT side")
{
	if (!player)
		return;

	if (!practiceMode && player->m_iTeamNum != CS_TEAM_SPECTATOR)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Switch teams only available on .pracc mode");
		return;
	}

	player->ChangeTeam(CS_TEAM_CT);
}

CON_COMMAND_CHAT(t, "Switch to T side")
{
	if (!player)
		return;

	if (!practiceMode && player->m_iTeamNum != CS_TEAM_SPECTATOR)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Switch teams only available on .pracc mode");
		return;
	}

	player->ChangeTeam(CS_TEAM_T);
}

CON_COMMAND_CHAT(spec, "Switch to Spectator")
{
	if (!player)
		return;

	if (!practiceMode)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Switch teams only available on .pracc mode");
		return;
	}

	player->ChangeTeam(CS_TEAM_SPECTATOR);
}

CON_COMMAND_CHAT(side, "Switch to team selector")
{
	if (!player)
		return;

	if (!practiceMode)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Switch teams only available on .pracc mode");
		return;
	}

	player->ChangeTeam(CS_TEAM_NONE);
}
