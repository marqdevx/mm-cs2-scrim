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

#include "tier0/memdbgon.h"

extern IGameEventManager2 *g_gameEventManager;
extern IServerGameClients *g_pSource2GameClients;
extern CEntitySystem *g_pEntitySystem;

CUtlVector<CGameEventListener *> g_vecEventListeners;

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
	// Remove chat message for team changes
	if (g_bBlockTeamMessages)
		pEvent->SetBool("silent", true);
}

extern CUtlVector <CCSPlayerController*> coaches;

GAME_EVENT_F(round_start)
{
	if (coaches.Count() < 1) return;
	
	FOR_EACH_VEC(coaches,i){
		CCSPlayerController *pTarget = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(coaches[i]->GetPlayerSlot() + 1));

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
		
		CHandle<CCSPlayerController> hController = pTarget->GetHandle();
		
		new CTimer(2.0f, false, false, [hController]()
		{
			CCSPlayerController *pController = hController.Get();

			int currentTeam = pController->m_iTeamNum;
			pController->ChangeTeam(CS_TEAM_SPECTATOR);
			pController->ChangeTeam(currentTeam);
		
			return;
		});
	}
}
