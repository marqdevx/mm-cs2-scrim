#pragma once
#include "../schema.h"
#include "services.h"



class SC_CBaseEntity : public CBaseEntity
{
public:
	/*SCHEMA_FIELD(int32_t,  m_iHealth);
	SCHEMA_FIELD(int32_t,  m_iMaxHealth);
	//SCHEMA_FIELD(LifeState_t,  m_lifeState);
	SCHEMA_FIELD(uint8_t,  m_iTeamNum);
	SCHEMA_FIELD(float,  m_flGravityScale);*/
};


class CBaseAnimGraph : public CBaseModelEntity
{
public:
};

class CBaseFlex : public CBaseAnimGraph
{
public:
};

class CBaseGrenade : public CBaseFlex
{
public:
	//SCHEMA_FIELD(CHandle<CCSPlayerPawn>,  m_hThrower);
};

class CBaseCSGrenadeProjectile : public CBaseGrenade
{
public:
};

class CSmokeGrenadeProjectile : public CBaseCSGrenadeProjectile
{
public:
	//SCHEMA_FIELD(Vector, m_vSmokeColor);
};