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
#include "entity/cgamerules.h"

#include "tier0/memdbgon.h"

extern IGameEventManager2 *g_gameEventManager;
extern IServerGameClients *g_pSource2GameClients;
extern CGameEntitySystem *g_pEntitySystem;
extern CGlobalVars *gpGlobals;
extern CCSGameRules *g_pGameRules;

CUtlVector<CGameEventListener *> g_vecEventListeners;

extern CUtlVector <CCSPlayerController*> coaches;
extern bool practiceMode;
extern bool no_flash_mode;
bool half_last_round = false;
bool swapped_teams = false;

extern bool g_bEnablePractice;
extern bool g_bEnableCoach;

int g_DamageDone[MAXPLAYERS+1][MAXPLAYERS+1];
int g_DamageDoneHits[MAXPLAYERS+1][MAXPLAYERS+1];

bool g_bEnableDamagePrint = true;
FAKE_BOOL_CVAR(cs2scrim_damage_print, "Whether to enable chat Damage Print", g_bEnableDamagePrint, true, false)

void PrintDamageInfo(int client) {
	CCSPlayerController* pController = CCSPlayerController::FromSlot(client);

	if(!pController)
		return;

    int team = pController->m_iTeamNum();
    if (team != CS_TEAM_T && team != CS_TEAM_CT)
        return;

    int otherTeam = (team == CS_TEAM_T) ? CS_TEAM_CT : CS_TEAM_T;
	
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
			CCSPlayerController* pTarget = CCSPlayerController::FromSlot(i);

			if(!pTarget) 
				continue;

			if (pTarget->m_iTeamNum() == otherTeam) {
				int targetHealth = pTarget->GetPawn()->m_iHealth();
				if (targetHealth < 0) targetHealth = 0;

				ClientPrint(pController, HUD_PRINTTALK, CHAT_PREFIX "(%i dmg / %i hits) to  (%i dmg / %i hits) from %s (%i)", g_DamageDone[client][i], g_DamageDoneHits[client][i], g_DamageDone[i][client],  g_DamageDoneHits[i][client], pTarget->GetPlayerName(), targetHealth);
			}
    }
}

void RegisterEventListeners()
{
	static bool bRegistered = false;

	if (bRegistered || !g_gameEventManager)
		return;

	FOR_EACH_VEC(g_vecEventListeners, i)
	{
		g_gameEventManager->AddListener(g_vecEventListeners[i], g_vecEventListeners[i]->GetEventName(), true);
	}

	bRegistered = true;
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



GAME_EVENT_F(player_team)
{	
	CCSPlayerController* pController = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	
	if (coaches.Count() < 1) return;
	FOR_EACH_VEC(coaches,i){
		if(pController->GetPlayerSlot() == coaches[i]->GetPlayerSlot()) pEvent->SetBool("silent", true); return;
		//ClientPrint(pController, HUD_PRINTTALK, "coach slot %i", coaches[i]->GetPlayerSlot());
	}
}

GAME_EVENT_F(player_death){
	//TODO: hide only coach death notice
}

GAME_EVENT_F(round_announce_last_round_half){
	if(!g_bEnableCoach) return;
	
	half_last_round = true;
}

GAME_EVENT_F(round_prestart)
{
	if (coaches.Count() < 1 || !g_bEnableCoach) return;

	FOR_EACH_VEC(coaches,i){
		CCSPlayerController *pTarget = CCSPlayerController::FromSlot(coaches[i]->GetPlayerSlot());

		if(!pTarget) return;	//avoid crash if coach is not connected
		//ClientPrint(pTarget, HUD_PRINTTALK, "coach slot  side %i", coaches[i]->m_iTeamNum());

		int currentTeam = pTarget->m_iTeamNum;
		int newTeam = CS_TEAM_SPECTATOR;

		pTarget->m_pInGameMoneyServices->m_iAccount = 0;

		pTarget->ChangeTeam(CS_TEAM_SPECTATOR);

		//pTarget->GetPawn()->CommitSuicide(false, true);

		if(half_last_round && !swapped_teams){
			if(currentTeam == CS_TEAM_CT){
				newTeam = CS_TEAM_T;
			}else{
				newTeam = CS_TEAM_CT;
			}
		}else{
				newTeam = currentTeam;
		}

		CHandle<CCSPlayerController> hController = pTarget->GetHandle();

		new CTimer(0.15f, false, [hController, newTeam]()
		{
			CCSPlayerController *pController = hController.Get();

			if(!pController) return -1.0f;	//avoid crash if coach is not connected

			pController->m_pInGameMoneyServices->m_iAccount = 0;

			pController->ChangeTeam(newTeam);

			new CTimer(0.0f, false, [hController, newTeam]()
			{
				CCSPlayerController *pController = hController.Get();
				if(!pController) return -1.0f;	//avoid crash if coach is not connected
				pController->GetPawn()->CommitSuicide(false, true);
				pController->m_pInGameMoneyServices->m_iAccount = 0;
				pController->m_pActionTrackingServices->m_matchStats().m_iDeaths = 0;
				return -1.0f;
			});

			return -0.15f;
		});

		if(half_last_round) half_last_round = false;
	}
}

GAME_EVENT_F(round_start)
{
	if(g_bEnableDamagePrint){
		for (int i = 1; i <= gpGlobals->maxClients; i++) {
			for (int j = 1; j <= gpGlobals->maxClients; j++) {
				g_DamageDone[i][j] = 0;
				g_DamageDoneHits[i][j] = 0;
			}
		}
	}

	if (coaches.Count() < 1) return;
	
	FOR_EACH_VEC(coaches,i){
		CCSPlayerController *pTarget = CCSPlayerController::FromSlot(coaches[i]->GetPlayerSlot());

		coaches[i]->m_pInGameMoneyServices->m_iAccount = 0;

		//coaches[i]->GetPawn()->CommitSuicide(false, true);
		
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iKills = 0;
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iDeaths = 0;
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iAssists = 0;
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iDamage = 0;
	}
}

GAME_EVENT_F(round_poststart)
{
	if (coaches.Count() < 1 || !g_bEnableCoach) return;
	
	FOR_EACH_VEC(coaches,i){
		CCSPlayerController *pTarget = CCSPlayerController::FromSlot(coaches[i]->GetPlayerSlot());

		coaches[i]->m_pInGameMoneyServices->m_iAccount = 0;
		
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iKills = 0;
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iDeaths = 0;
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iAssists = 0;
		coaches[i]->m_pActionTrackingServices->m_matchStats().m_iDamage = 0;
	}
}

GAME_EVENT_F(round_freeze_end)
{
	if (coaches.Count() < 1 || !g_bEnableCoach) return;
	
	FOR_EACH_VEC(coaches,i){
		CCSPlayerController *pTarget = CCSPlayerController::FromSlot(coaches[i]->GetPlayerSlot());

		if(!pTarget) return;	//avoid crash if coach is not connected

		int currentTeam = pTarget->m_iTeamNum;
		pTarget->ChangeTeam(CS_TEAM_SPECTATOR);
		pTarget->ChangeTeam(currentTeam);
		pTarget->GetPawn()->CommitSuicide(false, true);
		
		CHandle<CCSPlayerController> hController = pTarget->GetHandle();
		
		new CTimer(2.0f, false, [hController]()
		{
			CCSPlayerController *pController = hController.Get();

			if(!pController) return -2.0f;	//avoid crash if coach is not connected

			int currentTeam = pController->m_iTeamNum;
			pController->ChangeTeam(CS_TEAM_SPECTATOR);
			pController->ChangeTeam(currentTeam);
		
			return -2.0f;
		});
	}
}

GAME_EVENT_F(player_hurt){

	CCSPlayerController* pVictim = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	CCSPlayerController* pAttacker = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("attacker") + 1));

	if(!pVictim || !pAttacker) return;

	int damage = pEvent->GetFloat("dmg_health");
	int postDamageHealth = pEvent->GetFloat("health");
	int preDamageHealth = pVictim->GetPawn()->m_iHealth();

	if(g_bEnableDamagePrint && !practiceMode){
		int attacker = pEvent->GetFloat("attacker");
		int victim = pEvent->GetFloat("userid");

 		// this maxes the damage variables at 100,
        // so doing 50 damage when the player had 2 health
        // only counts as 2 damage.
        if (postDamageHealth == 0) {
            damage += preDamageHealth;
        }

        g_DamageDone[attacker][victim] += damage;
        g_DamageDoneHits[attacker][victim]++;
		return;
	}

	if(!practiceMode || !g_bEnablePractice) return;
	
	ClientPrint(pAttacker, HUD_PRINTTALK, CHAT_PREFIX "Damage done \04%d \01to \04%s\1[\04%d\01]", damage , pVictim->GetPlayerName(), postDamageHealth);
}

GAME_EVENT_F(player_blind){
	if(!practiceMode || !g_bEnablePractice) return;

	CCSPlayerController* pAttacker = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("attacker") + 1));
	CBasePlayerController *pTarget = (CBasePlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	
	if(!pAttacker || !pTarget)
		return;

	if(pTarget->GetPlayerName() && pEvent->GetFloat("blind_duration") > 0.0f)
		ClientPrint(pAttacker, HUD_PRINTTALK, CHAT_PREFIX "Flashed \04%s\1 for \x04%f\1 s", pTarget->GetPlayerName(), pEvent->GetFloat("blind_duration"));

	CCSPlayerController* pController = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	CCSPlayerPawnBase* cPlayerBase = (CCSPlayerPawnBase*)pController->GetPawn();

	if(!pController)
		return;
	if(!cPlayerBase)
		return;

	if(no_flash_mode) cPlayerBase->m_flFlashMaxAlpha = 2;
	
	if(pAttacker->GetPlayerSlot() != pController->GetPlayerSlot())
		ClientPrint(pController, HUD_PRINTTALK, CHAT_PREFIX "Flashed by \04%s\1, for \x04%f\1 s", pAttacker->GetPlayerName(), pEvent->GetFloat("blind_duration"));

}

GAME_EVENT_F(grenade_thrown){
	if (!practiceMode || !g_bEnablePractice)
		return;

	CCSPlayerController* pController = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(pEvent->GetUint64("userid") + 1));
	
	if (!pController)
		return;

	//CCSPlayerPawn* cPlayerBase = (CCSPlayerPawn*)pController->GetPawn();
	Vector currentPos = pController->GetPawn()->GetAbsOrigin();

	CCSPlayerPawnBase* cPlayerBase = (CCSPlayerPawnBase*)pController->GetPawn();
	QAngle currentAngle = cPlayerBase->m_angEyeAngles;
	
	//ClientPrintAll( HUD_PRINTTALK, CHAT_PREFIX "Pos: %f, %f, %f", currentPos.x, currentPos.y, currentPos.z);

	ZEPlayer *pPlayer = g_playerManager->GetPlayer(pController->GetPlayerSlot());
	pPlayer->lastThrow_position = currentPos;
	pPlayer->lastThrow_rotation = currentAngle;
}

GAME_EVENT_F(round_end){
	if(!g_bEnableDamagePrint)
		return;

    for (int i = 1; i <= gpGlobals->maxClients; i++) {
		CCSPlayerController* pController = CCSPlayerController::FromSlot(i);

		if(!pController)
			continue;

		PrintDamageInfo(i);
    }
}