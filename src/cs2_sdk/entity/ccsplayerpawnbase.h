#pragma once
#include "CBasePlayerPawn.h"
#include "ccsplayercontroller.h"
#include "cbaseentity.h"
#include "ehandle.h"
#include "cbaseentity.h"
#include "ccsplayerpawn.h"


class CCSPlayerPawnBase : public CBasePlayerPawn
{
public:
    DECLARE_SCHEMA_CLASS(CCSPlayerPawnBase)

    SCHEMA_FIELD(CHandle<CCSPlayerController>, m_hOriginalController)
	SCHEMA_FIELD(float, m_flFlashMaxAlpha)
    SCHEMA_FIELD(int32_t , m_ArmorValue)
    SCHEMA_FIELD(QAngle, m_angEyeAngles)


    CHandle<CCSPlayerController> GetCCS() { return m_hOriginalController.Get(); }
};