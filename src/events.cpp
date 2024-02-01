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

#include "common.h"
#include "KeyValues.h"
#include "commands.h"
#include "ctimer.h"
#include "eventlistener.h"
#include "entity/cbaseplayercontroller.h"
#include "entity/ccsplayerpawnbase.h"

#include "tier0/memdbgon.h"

extern IGameEventManager2 *g_gameEventManager;
extern IServerGameClients *g_pSource2GameClients;
extern CEntitySystem *g_pEntitySystem;

CUtlVector<CGameEventListener *> g_vecEventListeners;

extern CUtlVector <CCSPlayerController*> coaches;
extern bool practiceMode;
extern bool no_flash_mode;

void RegisterEventListeners()
{
	if (!g_gameEventManager)
		return;

	FOR_EACH_VEC(g_vecEventListeners, i)
	{
		g_gameEventManager->AddListener(g_vecEventListeners[i], g_vecEventListeners[i]->GetEventName(), true);
	}
}

void UnregisterEventListeners()
{
	if (!g_gameEventManager)
		return;

	FOR_EACH_VEC(g_vecEventListeners, i)
	{
		g_gameEventManager->RemoveListener(g_vecEventListeners[i]);
	}

	g_vecEventListeners.Purge();
}

bool g_bBlockTeamMessages = true;

CON_COMMAND_F(c_toggle_team_messages, "toggle team messages", FCVAR_SPONLY | FCVAR_LINKED_CONCOMMAND)
{
	g_bBlockTeamMessages = !g_bBlockTeamMessages;
}

GAME_EVENT_F(player_team)
{	
	CCSPlayerController* pController = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	
	if (coaches.Count() < 1) return;
	FOR_EACH_VEC(coaches,i){
		if(pController->GetPlayerSlot() == coaches[i]->GetPlayerSlot()) pEvent->SetBool("silent", true); return;
		//ClientPrint(pController, HUD_PRINTTALK, "coach slot %i", coaches[i]->GetPlayerSlot());
	}
}

GAME_EVENT_F(round_start)
{
	if (coaches.Count() < 1) return;
	
	FOR_EACH_VEC(coaches,i){
		CCSPlayerController *pTarget = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(coaches[i]->GetPlayerSlot() + 1));

		if(!pTarget) return;	//avoid crash if coach is not connected

		coaches[i]->m_pInGameMoneyServices->m_iAccount = 0;

		coaches[i]->GetPawn()->CommitSuicide(false, true);
		
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iKills = 0;
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iDeaths = 0;
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iAssists = 0;
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iDamage = 0;
	}
}

GAME_EVENT_F(round_freeze_end)
{
	if (coaches.Count() < 1) return;
	
	FOR_EACH_VEC(coaches,i){
		CCSPlayerController *pTarget = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(coaches[i]->GetPlayerSlot() + 1));
		
		if(!pTarget) return;	//avoid crash if coach is not connected

		CHandle<CCSPlayerController> hController = pTarget->GetHandle();
		
		new CTimer(2.0f, false, false, [hController]()
		{
			CCSPlayerController *pController = hController.Get();

			if(!pController) return;	//avoid crash if coach is not connected

			int currentTeam = pController->m_iTeamNum;
			pController->ChangeTeam(CS_TEAM_SPECTATOR);
			pController->ChangeTeam(currentTeam);
		
			return;
		});
	}
}

GAME_EVENT_F(player_hurt){
	if(!practiceMode) return;

	CCSPlayerController* pController = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("attacker") + 1));
	
	if (!pController)
		return;

	CCSPlayerController* pHurt = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	//ClientPrintAll(HUD_PRINTTALK, "Smoke end %i", pEvent->GetUint64("entityid"));
	int damage = pEvent->GetFloat("dmg_health");
	int actualHealth = pEvent->GetFloat("health");
	ClientPrint(pController, HUD_PRINTTALK, CHAT_PREFIX "Damage done \04%d \01to \04%s\1[\04%d\01]", damage , pHurt->GetPlayerName(), actualHealth);
	
}

GAME_EVENT_F(player_blind){
	if(!practiceMode) return;

	CBasePlayerController *pTarget = (CBasePlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	CCSPlayerController* pController = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	CCSPlayerController* pAttacker = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("attacker") + 1));
	
	CCSPlayerController* pPlayer = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	CCSPlayerPawnBase* cPlayerBase = (CCSPlayerPawnBase*)pPlayer->GetPawn();

	ClientPrint(pAttacker, HUD_PRINTTALK, CHAT_PREFIX "Flashed \04%s\1 for \x04%f\1 s", pTarget->GetPlayerName(), pEvent->GetFloat("blind_duration"));

	if(no_flash_mode) cPlayerBase->m_flFlashMaxAlpha = 2;

	if(pAttacker->GetPlayerSlot() == pController->GetPlayerSlot()) return;
	ClientPrint(pController, HUD_PRINTTALK, CHAT_PREFIX "Flashed by \04%i\1, for \x04%f\1 s", pEvent->GetUint64("attacker"), pEvent->GetFloat("blind_duration"));
}
