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

#include <../cs2fixes.h>
#include "utlstring.h"
#include "playermanager.h"
#include "adminsystem.h"

#include "entity/ccsplayercontroller.h"
#include "utils/entity.h"
#include "ctimer.h"
#include "ctime"

#include "tier0/memdbgon.h"


extern IVEngineServer2 *g_pEngineServer2;
extern CGameEntitySystem *g_pEntitySystem;
extern CGlobalVars *gpGlobals;

void ZEPlayer::OnAuthenticated()
{
	CheckAdmin();
	CheckInfractions();
	//g_pUserPreferencesSystem->PullPreferences(GetPlayerSlot().Get());
}

void ZEPlayer::CheckInfractions()
{
	g_pAdminSystem->ApplyInfractions(this);
}

void ZEPlayer::CheckAdmin()
{
	if (IsFakeClient())
		return;

	auto admin = g_pAdminSystem->FindAdmin(GetSteamId64());
	if (!admin)
	{
		SetAdminFlags(0);
		return;
	}

	SetAdminFlags(admin->GetFlags());

	Message("%lli authenticated as an admin\n", GetSteamId64());
}

bool ZEPlayer::IsAdminFlagSet(uint64 iFlag)
{
	return !iFlag || (m_iAdminFlags & iFlag);
}



static float g_flFloodInterval = 0.75f;
static int g_iMaxFloodTokens = 3;
static float g_flFloodCooldown = 3.0f;

FAKE_FLOAT_CVAR(cs2f_flood_interval, "Amount of time allowed between chat messages acquiring flood tokens", g_flFloodInterval, 0.75f, false)
FAKE_INT_CVAR(cs2f_max_flood_tokens, "Maximum number of flood tokens allowed before chat messages are blocked", g_iMaxFloodTokens, 3, false)
FAKE_FLOAT_CVAR(cs2f_flood_cooldown, "Amount of time to block messages for when a player floods", g_flFloodCooldown, 3.0f, false)

bool ZEPlayer::IsFlooding()
{
	if (m_bGagged) return false;

	float time = gpGlobals->curtime;
	float newTime = time + g_flFloodInterval;

	if (m_flLastTalkTime >= time)
	{
		if (m_iFloodTokens >= g_iMaxFloodTokens)
		{
			m_flLastTalkTime = newTime + g_flFloodCooldown;
			return true;
		}
		else
		{
			m_iFloodTokens++;
		}
	}
	else if(m_iFloodTokens > 0)
	{
		// Remove one flood token when player chats within time limit (slow decay)
		m_iFloodTokens--;
	}

	m_flLastTalkTime = newTime;
	return false;
}

void CPlayerManager::OnBotConnected(CPlayerSlot slot)
{
	m_vecPlayers[slot.Get()] = new ZEPlayer(slot, true);
}

bool CPlayerManager::OnClientConnected(CPlayerSlot slot, uint64 xuid, const char* pszNetworkID)
{
	Assert(m_vecPlayers[slot.Get()] == nullptr);

	Message("%d connected\n", slot.Get());

	ZEPlayer *pPlayer = new ZEPlayer(slot);
	pPlayer->SetUnauthenticatedSteamId(new CSteamID(xuid));

	pPlayer->grenade_throws.Purge();

	std::string ip(pszNetworkID);

	// Remove port
	for (int i = 0; i < ip.length(); i++)
	{
		if (ip[i] == ':')
		{
			ip = ip.substr(0, i);
			break;
		}
	}

	pPlayer->SetIpAddress(ip);

	if (!g_pAdminSystem->ApplyInfractions(pPlayer))
	{
		// Player is banned
		delete pPlayer;
		return false;
	}

	pPlayer->SetConnected();
	m_vecPlayers[slot.Get()] = pPlayer;

	ResetPlayerFlags(slot.Get());

	//g_pMapVoteSystem->ClearPlayerInfo(slot.Get());
	
	return true;
}

void CPlayerManager::OnClientDisconnect(CPlayerSlot slot)
{
	Message("%d disconnected\n", slot.Get());

	//g_pUserPreferencesSystem->PushPreferences(slot.Get());
	//g_pUserPreferencesSystem->ClearPreferences(slot.Get());

	delete m_vecPlayers[slot.Get()];
	m_vecPlayers[slot.Get()] = nullptr;

	ResetPlayerFlags(slot.Get());

	//g_pMapVoteSystem->ClearPlayerInfo(slot.Get());
}

void CPlayerManager::OnClientPutInServer(CPlayerSlot slot)
{
	ZEPlayer* pPlayer = m_vecPlayers[slot.Get()];

	pPlayer->SetInGame(true);
}

void CPlayerManager::OnLateLoad()
{
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CCSPlayerController* pController = CCSPlayerController::FromSlot(i);

		if (!pController || !pController->IsController() || !pController->IsConnected())
			continue;

		OnClientConnected(i, pController->m_steamID(), "0.0.0.0:0");
	}
}

void CPlayerManager::TryAuthenticate()
{
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if (m_vecPlayers[i] == nullptr || !m_vecPlayers[i]->IsConnected())
			continue;

		if (m_vecPlayers[i]->IsAuthenticated() || m_vecPlayers[i]->IsFakeClient())
			continue;

		if (g_pEngineServer2->IsClientFullyAuthenticated(i))
		{
			m_vecPlayers[i]->SetAuthenticated();
			m_vecPlayers[i]->SetSteamId(m_vecPlayers[i]->GetUnauthenticatedSteamId());
			Message("%lli authenticated %d\n", m_vecPlayers[i]->GetSteamId64(), i);
			m_vecPlayers[i]->OnAuthenticated();
		}
	}
}

void CPlayerManager::CheckInfractions()
{
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if (m_vecPlayers[i] == nullptr || m_vecPlayers[i]->IsFakeClient())
			continue;

		m_vecPlayers[i]->CheckInfractions();
	}

	g_pAdminSystem->SaveInfractions();
}


static bool g_bHideTeammatesOnly = false;

FAKE_BOOL_CVAR(cs2f_hide_teammates_only, "Whether to hide teammates only", g_bHideTeammatesOnly, false, false)

static bool g_bInfiniteAmmo = false;
FAKE_BOOL_CVAR(cs2f_infinite_reserve_ammo, "Whether to enable infinite reserve ammo on weapons", g_bInfiniteAmmo, false, false)

void CPlayerManager::SetupInfiniteAmmo()
{
	new CTimer(5.0f, false, []()
	{
		if (!g_bInfiniteAmmo)
			return 5.0f;

		for (int i = 0; i < gpGlobals->maxClients; i++)
		{
			CCSPlayerController* pController = CCSPlayerController::FromSlot(i);

			if (!pController)
				continue;

			auto pPawn = pController->GetPawn();

			if (!pPawn)
				continue;

			CPlayer_WeaponServices* pWeaponServices = pPawn->m_pWeaponServices;

			// it can sometimes be null when player joined on the very first round? 
			if (!pWeaponServices)
				continue;

			CUtlVector<CHandle<CBasePlayerWeapon>>* weapons = pWeaponServices->m_hMyWeapons();

			FOR_EACH_VEC(*weapons, i)
			{
				CBasePlayerWeapon* weapon = (*weapons)[i].Get();

				if (!weapon)
					continue;

				if (weapon->GetWeaponVData()->m_GearSlot() == GEAR_SLOT_RIFLE || weapon->GetWeaponVData()->m_GearSlot() == GEAR_SLOT_PISTOL)
					weapon->AcceptInput("SetReserveAmmoAmount", "999"); // 999 will be automatically clamped to the weapons m_nPrimaryReserveAmmoMax
			}
		}

		return 5.0f;
	});
}

ETargetType CPlayerManager::TargetPlayerString(int iCommandClient, const char* target, int& iNumClients, int *clients)
{
	ETargetType targetType = ETargetType::NONE;
	if (!V_stricmp(target, "@me"))
		targetType = ETargetType::SELF;
	else if (!V_stricmp(target, "@all"))
		targetType = ETargetType::ALL;
	else if (!V_stricmp(target, "@t"))
		targetType = ETargetType::T;
	else if (!V_stricmp(target, "@ct"))
		targetType = ETargetType::CT;
	else if (!V_stricmp(target, "@spec"))
		targetType = ETargetType::SPECTATOR;
	else if (!V_stricmp(target, "@random"))
		targetType = ETargetType::RANDOM;
	else if (!V_stricmp(target, "@randomt"))
		targetType = ETargetType::RANDOM_T;
	else if (!V_stricmp(target, "@randomct"))
		targetType = ETargetType::RANDOM_CT;
	
	if (targetType == ETargetType::SELF && iCommandClient != -1)
	{
		clients[iNumClients++] = iCommandClient;
	}
	else if (targetType == ETargetType::ALL)
	{
		for (int i = 0; i < gpGlobals->maxClients; i++)
		{
			if (m_vecPlayers[i] == nullptr)
				continue;

			CCSPlayerController* player = CCSPlayerController::FromSlot(i);

			if (!player || !player->IsController() || !player->IsConnected())
				continue;

			clients[iNumClients++] = i;
		}
	}
	else if (targetType >= ETargetType::SPECTATOR)
	{
		for (int i = 0; i < gpGlobals->maxClients; i++)
		{
			if (m_vecPlayers[i] == nullptr)
				continue;

			CCSPlayerController* player = CCSPlayerController::FromSlot(i);

			if (!player || !player->IsController() || !player->IsConnected())
				continue;

			if (player->m_iTeamNum() != (targetType == ETargetType::T ? CS_TEAM_T : targetType == ETargetType::CT ? CS_TEAM_CT : CS_TEAM_SPECTATOR))
				continue;

			clients[iNumClients++] = i;
		}
	}
	else if (targetType >= ETargetType::RANDOM && targetType <= ETargetType::RANDOM_CT)
	{
		int attempts = 0;

		while (iNumClients == 0 && attempts < 10000)
		{
			int slot = rand() % (gpGlobals->maxClients - 1);

			// Prevent infinite loop
			attempts++;

			if (m_vecPlayers[slot] == nullptr)
				continue;

			CCSPlayerController* player = CCSPlayerController::FromSlot(slot);

			if (!player || !player->IsController() || !player->IsConnected())
				continue;

			if (targetType >= ETargetType::RANDOM_T && (player->m_iTeamNum() != (targetType == ETargetType::RANDOM_T ? CS_TEAM_T : CS_TEAM_CT)))
				continue;

			clients[iNumClients++] = slot;
		}
	}
	else if (*target == '#')
	{
		int userid = V_StringToUint16(target + 1, -1);

		if (userid != -1)
		{
			targetType = ETargetType::PLAYER;
			CCSPlayerController* player = CCSPlayerController::FromSlot(GetSlotFromUserId(userid).Get());
			if(player && player->IsController() && player->IsConnected())
				clients[iNumClients++] = GetSlotFromUserId(userid).Get();
		}
	}
	else
	{
		for (int i = 0; i < gpGlobals->maxClients; i++)
		{
			if (m_vecPlayers[i] == nullptr)
				continue;

			CCSPlayerController* player = CCSPlayerController::FromSlot(i);

			if (!player || !player->IsController() || !player->IsConnected())
				continue;

			if (V_stristr(player->GetPlayerName(), target))
			{
				targetType = ETargetType::PLAYER;
				clients[iNumClients++] = i;
				break;
			}
		}
	}

	return targetType;
}

ZEPlayer *CPlayerManager::GetPlayer(CPlayerSlot slot)
{
	if (slot.Get() < 0 || slot.Get() >= gpGlobals->maxClients)
		return nullptr;

	return m_vecPlayers[slot.Get()];
};

// In userids, the lower byte is always the player slot
CPlayerSlot CPlayerManager::GetSlotFromUserId(uint16 userid)
{
	return CPlayerSlot(userid & 0xFF);
}

ZEPlayer *CPlayerManager::GetPlayerFromUserId(uint16 userid)
{
	uint8 index = userid & 0xFF;

	if (index >= gpGlobals->maxClients)
		return nullptr;

	return m_vecPlayers[index];
}

ZEPlayer* CPlayerManager::GetPlayerFromSteamId(uint64 steamid)
{
	for (ZEPlayer* player : m_vecPlayers)
	{
		if (player && player->IsAuthenticated() && player->GetSteamId64() == steamid)
			return player;
	}

	return nullptr;
}

void CPlayerManager::SetPlayerStopSound(int slot, bool set)
{
	if (set)
		m_nUsingStopSound |= ((uint64)1 << slot);
	else
		m_nUsingStopSound &= ~((uint64)1 << slot);

	// Set the user prefs if the player is ingame
	ZEPlayer* pPlayer = m_vecPlayers[slot];
	if (!pPlayer) return;

	uint64 iSlotMask = (uint64)1 << slot;
	int iStopPreferenceStatus = (m_nUsingStopSound & iSlotMask)?1:0;
	int iSilencePreferenceStatus = (m_nUsingSilenceSound & iSlotMask)?2:0;
	//g_pUserPreferencesSystem->SetPreferenceInt(slot, SOUND_STATUS_PREF_KEY_NAME, iStopPreferenceStatus + iSilencePreferenceStatus);
}

void CPlayerManager::SetPlayerSilenceSound(int slot, bool set)
{
	if (set)
		m_nUsingSilenceSound |= ((uint64)1 << slot);
	else
		m_nUsingSilenceSound &= ~((uint64)1 << slot);

	// Set the user prefs if the player is ingame
	ZEPlayer* pPlayer = m_vecPlayers[slot];
	if (!pPlayer) return;

	uint64 iSlotMask = (uint64)1 << slot;
	int iStopPreferenceStatus = (m_nUsingStopSound & iSlotMask)?1:0;
	int iSilencePreferenceStatus = (m_nUsingSilenceSound & iSlotMask)?2:0;
	//g_pUserPreferencesSystem->SetPreferenceInt(slot, SOUND_STATUS_PREF_KEY_NAME, iStopPreferenceStatus + iSilencePreferenceStatus);
}

void CPlayerManager::SetPlayerStopDecals(int slot, bool set)
{
	if (set)
		m_nUsingStopDecals |= ((uint64)1 << slot);
	else
		m_nUsingStopDecals &= ~((uint64)1 << slot);

	// Set the user prefs if the player is ingame
	ZEPlayer* pPlayer = m_vecPlayers[slot];
	if (!pPlayer) return;

	uint64 iSlotMask = (uint64)1 << slot;
	int iDecalPreferenceStatus = (m_nUsingStopDecals & iSlotMask)?1:0;
	//g_pUserPreferencesSystem->SetPreferenceInt(slot, DECAL_PREF_KEY_NAME, iDecalPreferenceStatus);
}

void CPlayerManager::ResetPlayerFlags(int slot)
{
	SetPlayerStopSound(slot, false);
	SetPlayerSilenceSound(slot, true);
	SetPlayerStopDecals(slot, true);
}
