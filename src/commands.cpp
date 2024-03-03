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
#include "entity/ccsplayercontroller.h"
#include "entity/ccsplayerpawn.h"
#include "entity/cbasemodelentity.h"
#include "entity/ccsweaponbase.h"
#include "entity/cparticlesystem.h"
#include "entity/lights.h"
#include "playermanager.h"
#include "adminsystem.h"
#include "ctimer.h"

#undef snprintf
#include "vendor/nlohmann/json.hpp"

#include "tier0/memdbgon.h"

using json = nlohmann::json;

extern CGameEntitySystem *g_pEntitySystem;
extern IVEngineServer2* g_pEngineServer2;

extern bool practiceMode;
extern bool no_flash_mode;

WeaponMapEntry_t WeaponMap[] = {
	{{"bizon"},							"weapon_bizon",			"PP-Bizon",			1400, 26, GEAR_SLOT_RIFLE},
	{{"mac10", "mac"},					"weapon_mac10",			"MAC-10",			1050, 27, GEAR_SLOT_RIFLE},
	{{"mp5sd", "mp5"},					"weapon_mp5sd",			"MP5-SD",			1500, 23, GEAR_SLOT_RIFLE},
	{{"mp7"},							"weapon_mp7",			"MP7",				1500, 23, GEAR_SLOT_RIFLE},
	{{"mp9"},							"weapon_mp9",			"MP9",				1250, 34, GEAR_SLOT_RIFLE},
	{{"p90"},							"weapon_p90",			"P90",				2350, 19, GEAR_SLOT_RIFLE},
	{{"ump45", "ump"},					"weapon_ump45",			"UMP-45",			1200, 24, GEAR_SLOT_RIFLE},
	{{"ak47", "ak"},					"weapon_ak47",			"AK-47",			2700, 7, GEAR_SLOT_RIFLE},
	{{"aug"},							"weapon_aug",			"AUG",				3300, 8, GEAR_SLOT_RIFLE},
	{{"famas"},							"weapon_famas",			"FAMAS",			2050, 10, GEAR_SLOT_RIFLE},
	{{"galilar", "galil"},				"weapon_galilar",		"Galil AR",			1800, 13, GEAR_SLOT_RIFLE},
	{{"m4a4"},							"weapon_m4a1",			"M4A4",				3100, 16, GEAR_SLOT_RIFLE},
	{{"m4a1-s", "m4a1"},				"weapon_m4a1_silencer",	"M4A1-S",			2900, 60, GEAR_SLOT_RIFLE},
	{{"sg553"},							"weapon_sg556",			"SG 553",			3000, 39, GEAR_SLOT_RIFLE},
	{{"awp"},							"weapon_awp",			"AWP",				4750, 9, GEAR_SLOT_RIFLE},
	{{"g3sg1"},							"weapon_g3sg1",			"G3SG1",			5000, 11, GEAR_SLOT_RIFLE},
	{{"scar20", "scar"},				"weapon_scar20",		"SCAR-20",			5000, 38, GEAR_SLOT_RIFLE},
	{{"ssg08", "ssg"},					"weapon_ssg08",			"SSG 08",			1700, 40, GEAR_SLOT_RIFLE},
	{{"mag7", "mag"},					"weapon_mag7",			"MAG-7",			1300, 29, GEAR_SLOT_RIFLE},
	{{"nova"},							"weapon_nova",			"Nova",				1050, 35, GEAR_SLOT_RIFLE},
	{{"sawedoff"},						"weapon_sawedoff",		"Sawed-Off",		1100, 29, GEAR_SLOT_RIFLE},
	{{"xm1014", "xm"},					"weapon_xm1014",		"XM1014",			2000, 25, GEAR_SLOT_RIFLE},
	{{"m249"},							"weapon_m249",			"M249",				5200, 14, GEAR_SLOT_RIFLE},
	{{"negev"},							"weapon_negev",			"Negev",			1700, 28, GEAR_SLOT_RIFLE},
	{{"deagle"},						"weapon_deagle",		"Desert Eagle",		700, 1, GEAR_SLOT_PISTOL},
	{{"dualberettas", "elite"},			"weapon_elite",			"Dual Berettas",	300, 2, GEAR_SLOT_PISTOL},
	{{"fiveseven"},						"weapon_fiveseven",		"Five-SeveN",		500, 3, GEAR_SLOT_PISTOL},
	{{"glock18", "glock"},				"weapon_glock",			"Glock-18",			200, 4, GEAR_SLOT_PISTOL},
	{{"p2000"},							"weapon_hkp2000",		"P2000",			200, 32, GEAR_SLOT_PISTOL},
	{{"p250"},							"weapon_p250",			"P250",				300, 36, GEAR_SLOT_PISTOL},
	{{"tec9"},							"weapon_tec9",			"Tec-9",			500, 30, GEAR_SLOT_PISTOL},
	{{"usp-s", "usp"},					"weapon_usp_silencer",	"USP-S",			200, 61, GEAR_SLOT_PISTOL},
	{{"cz75-auto", "cs75a", "cz"},		"weapon_cz75a",			"CZ75-Auto",		500, 63, GEAR_SLOT_PISTOL},
	{{"r8revolver", "revolver", "r8"},	"weapon_revolver",		"R8 Revolver",		600, 64, GEAR_SLOT_PISTOL},
	{{"hegrenade", "he"},				"weapon_hegrenade",		"HE Grenade",		300, 44, GEAR_SLOT_GRENADES, 1},
	{{"molotov"},						"weapon_molotov",		"Molotov",			400, 46, GEAR_SLOT_GRENADES, 1},
	{{"kevlar"},						"item_kevlar",			"Kevlar Vest",		650, 50, GEAR_SLOT_UTILITY},
};

bool g_bEnableWeapons = false;

FAKE_BOOL_CVAR(cs2f_weapons_enable, "Whether to enable weapon commands", g_bEnableWeapons, false, false)

void ParseWeaponCommand(const CCommand& args, CCSPlayerController* player)
{
	if (!g_bEnableWeapons || !player || !player->m_hPawn())
		return;

	CCSPlayerPawn* pPawn = (CCSPlayerPawn*)player->GetPawn();
	WeaponMapEntry_t weaponEntry;
	bool foundWeapon = false;

	for (int i = 0; i < sizeof(WeaponMap) / sizeof(*WeaponMap); i++)
	{
		if (foundWeapon)
			break;

		weaponEntry = WeaponMap[i];
		const char* command = args[0];

		if (!V_strncmp("c_", command, 2))
			command = command + 2;

		for (std::string alias : weaponEntry.aliases)
		{
			if (!V_stricmp(command, alias.c_str()))
			{
				foundWeapon = true;
				break;
			}
		}
	}

	if (!foundWeapon)
		return;

	if (pPawn->m_iHealth() <= 0 || pPawn->m_iTeamNum != CS_TEAM_CT)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You can only buy weapons when human.");
		return;
	}

	CCSPlayer_ItemServices* pItemServices = pPawn->m_pItemServices;
	CPlayer_WeaponServices* pWeaponServices = pPawn->m_pWeaponServices;

	// it can sometimes be null when player joined on the very first round? 
	if (!pItemServices || !pWeaponServices)
		return;

	int money = player->m_pInGameMoneyServices->m_iAccount;

	if (money < weaponEntry.iPrice)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You can't afford %s! It costs $%i, you only have $%i", weaponEntry.szWeaponName, weaponEntry.iPrice, money);
		return;
	}

	if (weaponEntry.maxAmount)
	{
		CUtlVector<WeaponPurchaseCount_t>* weaponPurchases = pPawn->m_pActionTrackingServices->m_weaponPurchasesThisRound().m_weaponPurchases;
		bool found = false;
		FOR_EACH_VEC(*weaponPurchases, i)
		{
			WeaponPurchaseCount_t& purchase = (*weaponPurchases)[i];
			if (purchase.m_nItemDefIndex == weaponEntry.iItemDefIndex)
			{
				if (purchase.m_nCount >= weaponEntry.maxAmount)
				{
					ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You cannot buy any more %s (Max %i)", weaponEntry.szWeaponName, weaponEntry.maxAmount);
					return;
				}
				purchase.m_nCount += 1;
				found = true;
				break;
			}
		}

		if (!found)
		{
			WeaponPurchaseCount_t purchase = {};

			purchase.m_nCount = 1;
			purchase.m_nItemDefIndex = weaponEntry.iItemDefIndex;

			weaponPurchases->AddToTail(purchase);
		}
	}

	CUtlVector<CHandle<CBasePlayerWeapon>>* weapons = pWeaponServices->m_hMyWeapons();

	FOR_EACH_VEC(*weapons, i)
	{
		CHandle<CBasePlayerWeapon>& weaponHandle = (*weapons)[i];

		if (!weaponHandle.IsValid())
			continue;

		CBasePlayerWeapon* weapon = weaponHandle.Get();

		if (!weapon)
			continue;

		if (weapon->GetWeaponVData()->m_GearSlot() == weaponEntry.iGearSlot && (weaponEntry.iGearSlot == GEAR_SLOT_RIFLE || weaponEntry.iGearSlot == GEAR_SLOT_PISTOL))
		{
			// Despite having to pass a weapon into DropPlayerWeapon, it only drops the weapon if it's also the players active weapon
			pWeaponServices->m_hActiveWeapon = weaponHandle;
			pItemServices->DropPlayerWeapon(weapon);

			break;
		}
	}

	player->m_pInGameMoneyServices->m_iAccount = money - weaponEntry.iPrice;
	pItemServices->GiveNamedItem(weaponEntry.szClassName);
	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You have purchased %s for $%i", weaponEntry.szWeaponName, weaponEntry.iPrice);
}

void WeaponCommandCallback(const CCommandContext& context, const CCommand& args)
{
	CCSPlayerController* pController = nullptr;
	if (context.GetPlayerSlot().Get() != -1)
		pController = (CCSPlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(context.GetPlayerSlot().Get() + 1));

	// Only allow connected players to run chat commands
	if (pController && !pController->IsConnected())
		return;

	ParseWeaponCommand(args, pController);
}

void RegisterWeaponCommands()
{
	for (int i = 0; i < sizeof(WeaponMap) / sizeof(*WeaponMap); i++)
	{
		WeaponMapEntry_t weaponEntry = WeaponMap[i];

		for (std::string alias : weaponEntry.aliases)
		{
			new CChatCommand(alias.c_str(), ParseWeaponCommand, "- Buys this weapon", ADMFLAG_NONE, CMDFLAG_NOHELP);
			ConCommandRefAbstract ref;

			char cmdName[64];
			V_snprintf(cmdName, sizeof(cmdName), "%s%s", COMMAND_PREFIX, alias.c_str());

			ConCommand command(&ref, cmdName, WeaponCommandCallback, "Buys this weapon", FCVAR_CLIENT_CAN_EXECUTE | FCVAR_LINKED_CONCOMMAND);
		}
	}
}

void ParseChatCommand(const char *pMessage, CCSPlayerController *pController)
{
	if (!pController || !pController->IsConnected())
		return;

	CCommand args;
	args.Tokenize(pMessage);

	uint16 index = g_CommandList.Find(hash_32_fnv1a_const(args[0]));

	if (g_CommandList.IsValidIndex(index))
	{
		(*g_CommandList[index])(args, pController);
	}
}

bool CChatCommand::CheckCommandAccess(CBasePlayerController *pPlayer, uint64 flags)
{
	if (!pPlayer)
		return false;

	int slot = pPlayer->GetPlayerSlot();

	ZEPlayer *pZEPlayer = g_playerManager->GetPlayer(slot);

	if (!pZEPlayer->IsAdminFlagSet(flags))
	{
		ClientPrint(pPlayer, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return false;
	}

	return true;
}

void ClientPrintAll(int hud_dest, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[256];
	V_vsnprintf(buf, sizeof(buf), msg, args);

	va_end(args);

	addresses::UTIL_ClientPrintAll(hud_dest, buf, nullptr, nullptr, nullptr, nullptr);
	ConMsg("%s\n", buf);
}

void ClientPrint(CBasePlayerController *player, int hud_dest, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[256];
	V_vsnprintf(buf, sizeof(buf), msg, args);

	va_end(args);

	if (player)
		addresses::ClientPrint(player, hud_dest, buf, nullptr, nullptr, nullptr, nullptr);
	else
		ConMsg("%s\n", buf);
}

bool g_bEnableHide = false;
static int g_iDefaultHideDistance = 250;
static int g_iMaxHideDistance = 2000;

FAKE_BOOL_CVAR(cs2f_hide_enable, "Whether to enable hide", g_bEnableHide, false, false)
FAKE_INT_CVAR(cs2f_hide_distance_default, "The default distance for hide", g_iDefaultHideDistance, 250, false)
FAKE_INT_CVAR(cs2f_hide_distance_max, "The max distance for hide", g_iMaxHideDistance, 2000, false)


CON_COMMAND_CHAT(help, "- Display list of commands in console")
{
	if (!player)
	{
		ClientPrint(player, HUD_PRINTCONSOLE, "The list of all commands is:");

		FOR_EACH_VEC(g_CommandList, i)
		{
			CChatCommand *cmd = g_CommandList[i];

			if (!cmd->IsCommandFlagSet(CMDFLAG_NOHELP))
				ClientPrint(player, HUD_PRINTCONSOLE, "c_%s %s", cmd->GetName(), cmd->GetDescription());
		}

		return;
	}

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "The list of all available commands will be shown in console.");
	ClientPrint(player, HUD_PRINTCONSOLE, "The list of all commands you can use is:");

	int iSlot = player->GetPlayerSlot();

	ZEPlayer *pZEPlayer = g_playerManager->GetPlayer(iSlot);

	FOR_EACH_VEC(g_CommandList, i)
	{
		CChatCommand *cmd = g_CommandList[i];
		uint64 flags = cmd->GetAdminFlags();

		if (pZEPlayer->IsAdminFlagSet(flags) && !cmd->IsCommandFlagSet(CMDFLAG_NOHELP))
				ClientPrint(player, HUD_PRINTCONSOLE, "!%s %s", cmd->GetName(), cmd->GetDescription());
	}

	ClientPrint(player, HUD_PRINTCONSOLE, "! can be replaced with / for a silent chat command, or c_ for console usage");
}

/* CVARS */
// CONVAR_TODO
bool g_bEnableCoach = true;
bool g_bEnableRecord = true;
bool g_bEnableBan = true;
bool g_bEnableGag = true;
bool g_bEnableKick = true;
bool g_bEnablePause = true;
bool g_bEnablePraccSpawn = true;
bool g_bEnablePractice = true;
bool g_bEnableRcon = true;
bool g_bEnableRestore = true;
bool g_bEnableScrim = true;
bool g_bEnableSlay = true;
bool g_bEnableTeamControl = true;
bool g_bEnableTeleport = true;

FAKE_BOOL_CVAR(cs2scrim_coach, "Whether to enable coach", g_bEnableCoach, true, false)
FAKE_BOOL_CVAR(cs2scrim_admin_ban, "Whether to enable coach", g_bEnableBan, true, false)
FAKE_BOOL_CVAR(cs2scrim_admin_gag, "Whether to enable coach", g_bEnableGag, true, false)
FAKE_BOOL_CVAR(cs2scrim_admin_kick, "Whether to enable coach", g_bEnableKick, true, false)
FAKE_BOOL_CVAR(cs2scrim_pause, "Whether to enable coach", g_bEnablePause, true, false)
FAKE_BOOL_CVAR(cs2scrim_practice_spawn, "Whether to enable coach", g_bEnablePraccSpawn, true, false)
FAKE_BOOL_CVAR(cs2scrim_pracc, "Whether to enable coach", g_bEnablePractice, true, false)
FAKE_BOOL_CVAR(cs2scrim_rcon, "Whether to enable coach", g_bEnableRcon, true, false)
FAKE_BOOL_CVAR(cs2scrim_demo, "Whether to enable coach", g_bEnableRecord, true, false)
FAKE_BOOL_CVAR(cs2scrim_restore, "Whether to enable coach", g_bEnableRestore, true, false)
FAKE_BOOL_CVAR(cs2scrim_scrim, "Whether to enable coach", g_bEnableScrim, true, false)
FAKE_BOOL_CVAR(cs2scrim_admin_slay, "Whether to enable coach", g_bEnableSlay, true, false)
FAKE_BOOL_CVAR(cs2scrim_admin_team, "Whether to enable coach", g_bEnableTeamControl, true, false)
FAKE_BOOL_CVAR(cs2scrim_admin_teleport, "Whether to enable coach", g_bEnableTeleport, true, false)

/* Player commands */
CON_COMMAND_CHAT(myuid, "test")
{
	if (!player)
		return;

	int iPlayer = player->GetPlayerSlot();

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Your userid is %i, slot: %i, retrieved slot: %i", g_pEngineServer2->GetPlayerUserId(iPlayer).Get(), iPlayer, g_playerManager->GetSlotFromUserId(g_pEngineServer2->GetPlayerUserId(iPlayer).Get()));
}


CON_COMMAND_CHAT(noflash, "noflash"){

	if (!player || !g_bEnablePractice)
		return;
	
	no_flash_mode = !no_flash_mode;

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Flash mode set to \04%i", no_flash_mode);
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

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You have been teleported to spawn. %i/%i", spawnIndex +1, totalSpawns);			
}

CUtlVector <CCSPlayerController*> coaches;

void print_coaches(){
	if (!g_bEnableCoach)
		return;
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
	new CTimer(0.0f, false, [hController, target_team_number]()
	{
		CCSPlayerController *pController = hController.Get();

		if (!pController)
			return -1.0f;
		
		pController->ChangeTeam(target_team_number);
		pController->m_szClan = "Coaching:";
		return -1.0f;
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
	if (!player || !g_bEnablePractice)
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
	if (!player || !g_bEnablePractice)
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
	if (!player || !g_bEnablePractice)
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
	if (!player || !g_bEnablePractice)
		return;

	if (!practiceMode)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Switch teams only available on .pracc mode");
		return;
	}

	player->ChangeTeam(CS_TEAM_NONE);
}

CON_COMMAND_CHAT(last, "Teleport to the last thrown grenade")
{
	if (!player || !g_bEnablePractice)
		return;

	if (!practiceMode)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Last grenade teleport only available on .pracc mode");
		return;
	}

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());
	
	player->GetPawn()->Teleport(&pPlayer->lastThrow_position, &pPlayer->lastThrow_rotation, nullptr);
}