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

#include "protobuf/generated/usermessages.pb.h"

#include "adminsystem.h"
#include "KeyValues.h"
#include "interfaces/interfaces.h"
#include "icvar.h"
#include "playermanager.h"
#include "commands.h"
#include "ctimer.h"

extern IVEngineServer2 *g_pEngineServer2;
extern CEntitySystem *g_pEntitySystem;

CAdminSystem* g_pAdminSystem = nullptr;

CUtlMap<uint32, FnChatCommandCallback_t> g_CommandList(0, 0, DefLessFunc(uint32));

bool practiceMode = false;
extern CUtlVector <CCSPlayerController*> coaches;
extern void print_coaches();

#define ADMIN_PREFIX "Admin %s has "

CON_COMMAND_F(c_reload_admins, "Reload admin config", FCVAR_SPONLY | FCVAR_LINKED_CONCOMMAND)
{
	if (!g_pAdminSystem->LoadAdmins())
		return;

	for (int i = 0; i < MAXPLAYERS; i++)
	{
		ZEPlayer* pPlayer = g_playerManager->GetPlayer(i);

		if (!pPlayer || pPlayer->IsFakeClient())
			continue;

		pPlayer->CheckAdmin();
	}

	Message("Admins reloaded\n");
}

CON_COMMAND_F(c_reload_infractions, "Reload infractions file", FCVAR_SPONLY | FCVAR_LINKED_CONCOMMAND)
{
	if (!g_pAdminSystem->LoadInfractions())
		return;

	for (int i = 0; i < MAXPLAYERS; i++)
	{
		ZEPlayer* pPlayer = g_playerManager->GetPlayer(i);

		if (!pPlayer || pPlayer->IsFakeClient())
			continue;

		pPlayer->CheckInfractions();
	}

	Message("Infractions reloaded\n");
}

CON_COMMAND_CHAT(ban, "ban a player")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(iCommandPlayer);

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_BAN))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 3)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Usage: !ban <name> <duration/0 (permanent)>");
		return;
	}

	int iNumClients = 0;
	int pSlot[MAXPLAYERS];

	if (g_playerManager->TargetPlayerString(iCommandPlayer, args[1], iNumClients, pSlot) != ETargetType::PLAYER || iNumClients > 1)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You can only target individual players for banning.");
		return;
	}

	if (!iNumClients)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Target not found.");
		return;
	}

	char* end;
	int iDuration = strtol(args[2], &end, 10);

	if (*end)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Invalid duration.");
		return;
	}

	CBasePlayerController* pTarget = (CBasePlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pSlot[0] + 1));

	if (!pTarget)
		return;

	ZEPlayer* pTargetPlayer = g_playerManager->GetPlayer(pSlot[0]);

	if (pTargetPlayer->IsFakeClient())
		return;

	CInfractionBase *infraction = new CBanInfraction(iDuration, pTargetPlayer->GetSteamId64());

	g_pAdminSystem->AddInfraction(infraction);
	infraction->ApplyInfraction(pTargetPlayer);
	g_pAdminSystem->SaveInfractions();

	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "banned %s for %i minutes.", player->GetPlayerName(), pTarget->GetPlayerName(), iDuration);
}

CON_COMMAND_CHAT(gag, "gag a player")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_BAN))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 3)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Usage: !gag <name> <duration/0 (permanent)>");
		return;
	}

	int iNumClients = 0;
	int pSlot[MAXPLAYERS];

	ETargetType nType = g_playerManager->TargetPlayerString(iCommandPlayer, args[1], iNumClients, pSlot);

	if (!iNumClients)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Target not found.");
		return;
	}

	char *end;
	int iDuration = strtol(args[2], &end, 10);

	if (*end)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Invalid duration.");
		return;
	}

	for (int i = 0; i < iNumClients; i++)
	{
		CBasePlayerController *pTarget = (CBasePlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pSlot[i] + 1));

		if (!pTarget)
			continue;

		ZEPlayer *pTargetPlayer = g_playerManager->GetPlayer(pSlot[i]);

		if (pTargetPlayer->IsFakeClient())
			continue;

		CInfractionBase *infraction = new CGagInfraction(iDuration, pTargetPlayer->GetSteamId64());

		// We're overwriting the infraction, so remove the previous one first
		g_pAdminSystem->FindAndRemoveInfraction(pTargetPlayer, CInfractionBase::Gag);
		g_pAdminSystem->AddInfraction(infraction);
		infraction->ApplyInfraction(pTargetPlayer);

		if (nType < ETargetType::ALL)
			ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "gagged %s for %i minutes.", player->GetPlayerName(), pTarget->GetPlayerName(), iDuration);
	}

	g_pAdminSystem->SaveInfractions();

	switch (nType)
	{
	case ETargetType::ALL:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "gagged everyone for %i minutes.", player->GetPlayerName(), iDuration);
		break;
	case ETargetType::T:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "gagged terrorists for %i minutes.", player->GetPlayerName(), iDuration);
		break;
	case ETargetType::CT:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "gagged counter-terrorists for %i minutes.", player->GetPlayerName(), iDuration);
		break;
	}
}

CON_COMMAND_CHAT(ungag, "ungags a player")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_UNBAN))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Usage: !ungag <name>");
		return;
	}

	int iNumClients = 0;
	int pSlot[MAXPLAYERS];

	ETargetType nType = g_playerManager->TargetPlayerString(iCommandPlayer, args[1], iNumClients, pSlot);

	if (!iNumClients)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Target not found.");
		return;
	}

	for (int i = 0; i < iNumClients; i++)
	{
		CBasePlayerController *pTarget = (CBasePlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pSlot[i] + 1));

		if (!pTarget)
			continue;

		ZEPlayer *pTargetPlayer = g_playerManager->GetPlayer(pSlot[i]);

		if (pTargetPlayer->IsFakeClient())
			continue;

		if (!g_pAdminSystem->FindAndRemoveInfraction(pTargetPlayer, CInfractionBase::Gag) && nType < ETargetType::ALL)
		{
			ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "%s is not gagged.", pTarget->GetPlayerName());
			continue;
		}

		if (nType < ETargetType::ALL)
			ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "ungagged %s.", player->GetPlayerName(), pTarget->GetPlayerName());
	}

	g_pAdminSystem->SaveInfractions();

	switch (nType)
	{
	case ETargetType::ALL:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "ungagged everyone.", player->GetPlayerName());
		break;
	case ETargetType::T:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "ungagged terrorists.", player->GetPlayerName());
		break;
	case ETargetType::CT:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "ungagged counter-terrorists.", player->GetPlayerName());
		break;
	}
}

CON_COMMAND_CHAT(kick, "kick a player")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer* pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_KICK))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Usage: !kick <name>");
		return;
	}

	int iNumClients = 0;
	int pSlot[MAXPLAYERS];

	g_playerManager->TargetPlayerString(iCommandPlayer, args[1], iNumClients, pSlot);

	if (!iNumClients)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Target not found.");
		return;
	}

	for (int i = 0; i < iNumClients; i++)
	{
		CBasePlayerController* pTarget = (CBasePlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pSlot[i] + 1));

		if (!pTarget)
			continue;

		ZEPlayer* pTargetPlayer = g_playerManager->GetPlayer(pSlot[i]);
		
		g_pEngineServer2->DisconnectClient(pTargetPlayer->GetPlayerSlot(), NETWORK_DISCONNECT_KICKED);

		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "kicked %s.", player->GetPlayerName(), pTarget->GetPlayerName());
	}
}

CON_COMMAND_CHAT(slay, "slay a player")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Usage: !slay <name>");
		return;
	}

	int iNumClients = 0;
	int pSlots[MAXPLAYERS];

	ETargetType nType = g_playerManager->TargetPlayerString(iCommandPlayer, args[1], iNumClients, pSlots);

	if (!iNumClients)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Target not found.");
		return;
	}

	for (int i = 0; i < iNumClients; i++)
	{
		CBasePlayerController *pTarget = (CBasePlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pSlots[i] + 1));

		if (!pTarget)
			continue;

		pTarget->GetPawn()->CommitSuicide(false, true);

		if (nType < ETargetType::ALL)
			ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "slayed %s.", player->GetPlayerName(), pTarget->GetPlayerName());
	}

	switch (nType)
	{
	case ETargetType::ALL:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "slayed everyone.", player->GetPlayerName());
		break;
	case ETargetType::T:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "slayed terrorists.", player->GetPlayerName());
		break;
	case ETargetType::CT:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "slayed counter-terrorists.", player->GetPlayerName());
		break;
	}
}

CON_COMMAND_CHAT(goto, "teleport to a player")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Usage: !goto <name>");
		return;
	}

	int iNumClients = 0;
	int pSlots[MAXPLAYERS];

	if (g_playerManager->TargetPlayerString(iCommandPlayer, args[1], iNumClients, pSlots) != ETargetType::PLAYER || iNumClients > 1)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Target too ambiguous.");
		return;
	}

	if (!iNumClients)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Target not found.");
		return;
	}

	for (int i = 0; i < iNumClients; i++)
	{
		CBasePlayerController *pTarget = (CBasePlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pSlots[i] + 1));

		if (!pTarget)
			continue;

		Vector newOrigin = pTarget->GetPawn()->GetAbsOrigin();

		player->GetPawn()->Teleport(&newOrigin, nullptr, nullptr);

		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "Teleported to %s.", pTarget->GetPlayerName());
	}
}

CON_COMMAND_CHAT(bring, "bring a player")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Usage: !bring <name>");
		return;
	}

	int iNumClients = 0;
	int pSlots[MAXPLAYERS];

	ETargetType nType = g_playerManager->TargetPlayerString(iCommandPlayer, args[1], iNumClients, pSlots);

	if (!iNumClients)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Target not found.");
		return;
	}

	for (int i = 0; i < iNumClients; i++)
	{
		CBasePlayerController *pTarget = (CBasePlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pSlots[i] + 1));

		if (!pTarget)
			continue;

		Vector newOrigin = player->GetPawn()->GetAbsOrigin();

		pTarget->GetPawn()->Teleport(&newOrigin, nullptr, nullptr);

		if (nType < ETargetType::ALL)
			ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "brought %s.", player->GetPlayerName(), pTarget->GetPlayerName());
	}

	switch (nType)
	{
	case ETargetType::ALL:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "brought everyone.", player->GetPlayerName());
		break;
	case ETargetType::T:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "brought terrorists.", player->GetPlayerName());
		break;
	case ETargetType::CT:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "brought counter-terrorists.", player->GetPlayerName());
		break;
	}
}

CON_COMMAND_CHAT(setteam, "set a player's team")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 3)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Usage: !setteam <name> <team (0-3)>");
		return;
	}

	int iNumClients = 0;
	int pSlots[MAXPLAYERS];

	ETargetType nType = g_playerManager->TargetPlayerString(iCommandPlayer, args[1], iNumClients, pSlots);

	if (!iNumClients)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Target not found.");
		return;
	}

	int iTeam = V_StringToInt32(args[2], -1);

	if (iTeam < CS_TEAM_NONE || iTeam > CS_TEAM_CT)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Invalid team specified, range is 0-3.");
		return;
	}

	for (int i = 0; i < iNumClients; i++)
	{
		CBasePlayerController *pTarget = (CBasePlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pSlots[i] + 1));

		if (!pTarget)
			continue;

		pTarget->m_iTeamNum = iTeam;
		pTarget->GetPawn()->m_iTeamNum = iTeam;

		if (nType < ETargetType::ALL)
			ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "moved %s to team %i.", player->GetPlayerName(), pTarget->GetPlayerName(), iTeam);
	}

	switch (nType)
	{
	case ETargetType::ALL:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "moved everyone to team %i.", iTeam);
		break;
	case ETargetType::T:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "moved terrorists to team %i.", iTeam);
		break;
	case ETargetType::CT:
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "moved counter-terrorists to team %i.", iTeam);
		break;
	}
}

CON_COMMAND_CHAT(noclip, "toggle noclip on yourself")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(iCommandPlayer);
	
	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	CBasePlayerPawn *pPawn = player->m_hPawn();

	if (!pPawn)
		return;

	if (pPawn->m_iHealth() <= 0)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You cannot noclip while dead!");
		return;
	}

	if (pPawn->m_MoveType() == MOVETYPE_NOCLIP)
	{
		pPawn->m_MoveType = MOVETYPE_WALK;
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "exited noclip.", player->GetPlayerName());
	}
	else
	{
		pPawn->m_MoveType = MOVETYPE_NOCLIP;
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX ADMIN_PREFIX "entered noclip.", player->GetPlayerName());
	}
}
/*
CON_COMMAND_CHAT(map, "change map")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(iCommandPlayer);

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_CHANGEMAP))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Usage: !map <mapname>");
		return;
	}

	if (!g_pEngineServer2->IsMapValid(args[1]))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Invalid map specified.");
		return;
	}

	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Changing map to %s...", args[1]);

	char buf[MAX_PATH];
	V_snprintf(buf, sizeof(buf), "changelevel %s", args[1]);

	new CTimer(5.0f, false, false, [buf]()
	{
		g_pEngineServer2->ServerCommand(buf);
	});
}
*/
CON_COMMAND_CHAT(hsay, "say something as a hud hint")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(iCommandPlayer);

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_CHAT))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Usage: !hsay <message>");
		return;
	}

	ClientPrintAll(HUD_PRINTCENTER, "%s", args.ArgS());
}

bool CAdminSystem::LoadAdmins()
{
	m_vecAdmins.Purge();
	KeyValues* pKV = new KeyValues("admins");
	KeyValues::AutoDelete autoDelete(pKV);

	const char *pszPath = "addons/cs2scrim/configs/admins.cfg";

	if (!pKV->LoadFromFile(g_pFullFileSystem, pszPath))
	{
		Warning("Failed to load %s\n", pszPath);
		return false;
	}
	for (KeyValues* pKey = pKV->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey())
	{
		const char *pszName = pKey->GetName();
		const char *pszSteamID = pKey->GetString("steam", nullptr);
		const char *pszFlags = pKey->GetString("flags", nullptr);

		if (!pszSteamID)
		{
			Warning("Admin entry %s is missing 'steam' key\n", pszName);
			return false;
		}

		if (!pszFlags)
		{
			Warning("Admin entry %s is missing 'flags' key\n", pszName);
			return false;
		}

		ConMsg("Loaded admin %s\n", pszName);
		ConMsg(" - Steam ID %5s\n", pszSteamID);
		ConMsg(" - Flags %5s\n", pszFlags);

		uint64 iFlags = ParseFlags(pszFlags);

		// Let's just use steamID64 for now
		m_vecAdmins.AddToTail(CAdmin(pszName, atoll(pszSteamID), iFlags));
	}

	return true;
}

bool CAdminSystem::LoadInfractions()
{
	m_vecInfractions.PurgeAndDeleteElements();
	KeyValues* pKV = new KeyValues("infractions");
	KeyValues::AutoDelete autoDelete(pKV);

	const char *pszPath = "addons/cs2scrim/data/infractions.txt";

	if (!pKV->LoadFromFile(g_pFullFileSystem, pszPath))
	{
		Warning("Failed to load %s\n", pszPath);
		return false;
	}

	for (KeyValues* pKey = pKV->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey())
	{
		uint64 iSteamId = pKey->GetUint64("steamid", -1);
		int iEndTime = pKey->GetInt("endtime", -1);
		int iType = pKey->GetInt("type", -1);

		if (iSteamId == -1)
		{
			Warning("Infraction entry is missing 'steam' key\n");
			return false;
		}

		if (iEndTime == -1)
		{
			Warning("Infraction entry is missing 'endtime' key\n");
			return false;
		}

		if (iType == -1)
		{
			Warning("Infraction entry is missing 'type' key\n");
			return false;
		}

		switch (iType)
		{
		case CInfractionBase::Ban:
			AddInfraction(new CBanInfraction(iEndTime, iSteamId));
			break;
		case CInfractionBase::Mute:
			AddInfraction(new CMuteInfraction(iEndTime, iSteamId));
			break;
		case CInfractionBase::Gag:
			AddInfraction(new CGagInfraction(iEndTime, iSteamId));
			break;
		default:
			Warning("Invalid infraction type %d\n", iType);
		}
	}

	return true;
}

void CAdminSystem::SaveInfractions()
{
	KeyValues* pKV = new KeyValues("infractions");
	KeyValues* pSubKey;
	KeyValues::AutoDelete autoDelete(pKV);

	FOR_EACH_VEC(m_vecInfractions, i)
	{
		int timestamp = m_vecInfractions[i]->GetTimestamp();
		if (timestamp != 0 && timestamp < std::time(0))
			continue;

		char buf[5];
		V_snprintf(buf, sizeof(buf), "%d", i);
		pSubKey = new KeyValues(buf);
		pSubKey->AddUint64("steamid", m_vecInfractions[i]->GetSteamId64());
		pSubKey->AddInt("endtime", m_vecInfractions[i]->GetTimestamp());
		pSubKey->AddInt("type", m_vecInfractions[i]->GetType());

		pKV->AddSubKey(pSubKey);
	}

	const char *pszPath = "addons/cs2scrim/data/infractions.txt";

	if (!pKV->SaveToFile(g_pFullFileSystem, pszPath))
		Warning("Failed to save infractions to %s", pszPath);
}

void CAdminSystem::AddInfraction(CInfractionBase* infraction)
{
	m_vecInfractions.AddToTail(infraction);
}


// This function can run at least twice when a player connects: Immediately upon client connection, and also upon getting authenticated by steam.
// It's also run when we're periodically checking for infraction expiry in the case of mutes/gags.
// This returns false only when called from ClientConnect and the player is banned in order to reject them.
bool CAdminSystem::ApplyInfractions(ZEPlayer *player)
{
	FOR_EACH_VEC(m_vecInfractions, i)
	{
		// Because this can run without the player being authenticated, and the fact that we're applying a ban/mute here,
		// we can immediately just use the steamid we got from the connecting player.
		uint64 iSteamID = player->IsAuthenticated() ? 
			player->GetSteamId64() : g_pEngineServer2->GetClientSteamID(player->GetPlayerSlot())->ConvertToUint64();

		// We're only interested in infractions concerning this player
		if (m_vecInfractions[i]->GetSteamId64() != iSteamID)
			continue;

		// Undo the infraction just briefly while checking if it ran out
		m_vecInfractions[i]->UndoInfraction(player);

		int timestamp = m_vecInfractions[i]->GetTimestamp();
		if (timestamp != 0 && timestamp <= std::time(0))
		{
			m_vecInfractions.Remove(i);
			continue;
		}

		// We are called from ClientConnect and the player is banned, immediately reject them
		if (!player->IsConnected() && m_vecInfractions[i]->GetType() == CInfractionBase::EInfractionType::Ban)
			return false;
		
		m_vecInfractions[i]->ApplyInfraction(player);
	}

	return true;
}

bool CAdminSystem::FindAndRemoveInfraction(ZEPlayer *player, CInfractionBase::EInfractionType type)
{
	FOR_EACH_VEC(m_vecInfractions, i)
	{
		if (m_vecInfractions[i]->GetSteamId64() == player->GetSteamId64() && m_vecInfractions[i]->GetType() == type)
		{
			m_vecInfractions[i]->UndoInfraction(player);
			m_vecInfractions.Remove(i);
			
			return true;
		}
	}

	return false;
}

CAdmin *CAdminSystem::FindAdmin(uint64 iSteamID)
{
	FOR_EACH_VEC(m_vecAdmins, i)
	{
		if (m_vecAdmins[i].GetSteamID() == iSteamID)
			return &m_vecAdmins[i];
	}

	return nullptr;
}

uint64 CAdminSystem::ParseFlags(const char* pszFlags)
{
	uint64 flags = 0;
	size_t length = V_strlen(pszFlags);

	for (size_t i = 0; i < length; i++)
	{
		char c = tolower(pszFlags[i]);
		if (c < 'a' || c > 'z')
			continue;

		if (c == 'z')
			return -1; // all flags

		flags |= ((uint64)1 << (c - 'a'));
	}

	return flags;
}

void CBanInfraction::ApplyInfraction(ZEPlayer *player)
{
	g_pEngineServer2->DisconnectClient(player->GetPlayerSlot(), NETWORK_DISCONNECT_KICKBANADDED); // "Kicked and banned"
}

void CMuteInfraction::ApplyInfraction(ZEPlayer* player)
{
	player->SetMuted(true);
}

void CMuteInfraction::UndoInfraction(ZEPlayer *player)
{
	player->SetMuted(false);
}

void CGagInfraction::ApplyInfraction(ZEPlayer *player)
{
	player->SetGagged(true);
}

void CGagInfraction::UndoInfraction(ZEPlayer *player)
{
	player->SetGagged(false);
}

//CS2Scrim custom commands
CON_COMMAND_CHAT(scrim, "Scrim mode")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}
	
	print_coaches();

	practiceMode = false;
	
	char buf[256];
	//V_snprintf(buf, sizeof(buf), "exec %s", args[1]);

	g_pEngineServer2->ServerCommand("exec scrim");

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Scrim mode loaded, mr24");
	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Available commands: \4.pause\1, \4.unpause\1");
	//ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Admin commands: \4.pracc\1, \4.scrim\1, \4.record\1, \4.stoprecord\1, \4.map\1");
}

CON_COMMAND_CHAT(pracc, "Practice mode")
{
	if (!player)
		return;
	
	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	practiceMode = true;

	char buf[256];
	//V_snprintf(buf, sizeof(buf), "exec %s", args[1]);

	g_pEngineServer2->ServerCommand("exec pracc");

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Practice mode loaded");
	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Available commands: \4.pause\1, \4.unpause\1");
	//ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Admin commands: \4.pracc\1, \4.scrim\1, \4.record\1, \4.stoprecord\1, \4.map\1");
}

CON_COMMAND_CHAT(forceunpause, "Force unpause")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Forced unpause");

	char buf[256];
	//V_snprintf(buf, sizeof(buf), "exec %s", args[1]);

	g_pEngineServer2->ServerCommand("mp_unpause_match");
}

CON_COMMAND_CHAT(map, "change map")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(iCommandPlayer);

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_CHANGEMAP))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Usage: !map <mapname>");
		return;
	}
/*
	if (!g_pEngineServer2->IsMapValid(args[1]))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Invalid map specified.");
		return;
	}
*/
	char buf[MAX_PATH];
	V_snprintf(buf, sizeof(buf), "changelevel de_%s", args[1]);

	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Changing map to %s...", args[1]);

	new CTimer(5.0f, false, false, [buf]()
	{
		g_pEngineServer2->ServerCommand(buf);
	});
	
	
}

CON_COMMAND_CHAT(restore, "Restore round")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Usage: !restore <round number>");
		return;
	}

	char buf[256];
	
	//Save the round number
    char nRound[2];

    //Needed to build a string for the default restore, format "backup_round<round>.txt" the <round> with "XX" format like "backup_round02.txt"
    char backupFile[256] = "backup_round";
	char aux[256] = "";
        
	int nRounds = atoi(args[1]);
	
	V_snprintf(buf, MAX_PATH, "%s", backupFile);

	V_snprintf(aux, MAX_PATH, "%s%02i.txt", buf, nRounds);

	V_snprintf(buf, MAX_PATH, "mp_backup_restore_load_file %s", aux);

	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"\4Restored round \3%i", nRounds);

	g_pEngineServer2->ServerCommand(buf);
}

char demoName[128];
CON_COMMAND_CHAT(record, "Record demo")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	char buf[256];
	char actualTime[32];
	char actualMap[32];

	std::tm tm{};
	std::time_t result = std::time(nullptr);
	std::localtime(&result);
	std::strftime(actualTime, sizeof(actualTime), "%d%B_%H-%M", std::localtime(&result));

	V_snprintf(actualMap,MAX_PATH, "unknownMap");
	V_snprintf(demoName, MAX_PATH, "%s_%s", actualTime, actualMap);

	V_snprintf(buf, MAX_PATH, "tv_record gotv/%s", demoName);
	ClientPrint(player, HUD_PRINTTALK, buf);
	g_pEngineServer2->ServerCommand(buf);
	//V_snprintf(buf, MAX_PATH, "record gotv/%s", demoName);
/*
	FormatTime(actualTime, sizeof(actualTime), "%d%B_%H-%M", GetTimestamp());	//https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
	GetCurrentMap(actualMap, sizeof(actualMap));

	StrCat(demoName, 256, actualTime);
	StrCat(demoName, 256, "_");
	StrCat(demoName, 256, actualMap[3]);

	PrintToChat(client, "Demo will be saved at gotv/%s.dem", demoName);

	ServerCommand("tv_record gotv/%s", demoName);*/

}

CON_COMMAND_CHAT(stoprecord, "Stop demo recording")
{
	if (!player)
		return;

	int iCommandPlayer = player->GetPlayerSlot();

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(player->GetPlayerSlot());

	if (!pPlayer->IsAdminFlagSet(ADMFLAG_SLAY))
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "You don't have access to this command.");
		return;
	}

	char buf[256];
	char command[256];
	
	V_snprintf(buf, MAX_PATH, "Demo saved at gotv/%s", demoName);
	ClientPrint(player, HUD_PRINTTALK, buf);

	V_snprintf(command, MAX_PATH, "tv_stoprecord");
	g_pEngineServer2->ServerCommand(command);
	
}