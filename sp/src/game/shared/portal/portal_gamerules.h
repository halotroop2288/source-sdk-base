//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game rules for Portal.
//
//=============================================================================//

#ifdef PORTAL_MP



#include "portal_mp_gamerules.h" //redirect to multiplayer gamerules in multiplayer builds



#else

#ifndef PORTAL_GAMERULES_H
#define PORTAL_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "gamerules.h"
#include "hl2_gamerules.h"

#ifdef CLIENT_DLL
	#define CPortalGameRules C_PortalGameRules
	#define CPortalGameRulesProxy C_PortalGameRulesProxy
#endif


class CPortalGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CPortalGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};


class CPortalGameRules : public CHalfLife2
{
public:
	DECLARE_CLASS( CPortalGameRules, CHalfLife2 );

	virtual bool	Init();
	
	virtual bool	ShouldUseRobustRadiusDamage(CBaseEntity *pEntity);

#ifdef CLIENT_DLL
	virtual bool IsBonusChallengeTimeBased( void );
#endif

private:
	// Rules change for the mega physgun
	CNetworkVar( bool, m_bMegaPhysgun );

	
	DECLARE_NETWORKCLASS_NOBASE(); // This makes datatables able to access our private vars.

#ifndef CLIENT_DLL

	CPortalGameRules();
	virtual ~CPortalGameRules() {}

	virtual void			Think( void );

	virtual bool			ClientCommand( CBaseEntity *pEdict, const CCommand &args );

	
public:

	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );

	bool	MegaPhyscannonActive( void ) { return m_bMegaPhysgun;	}

private:

	int						DefaultFOV( void ) { return 75; }
#endif
};


//-----------------------------------------------------------------------------
// Gets us at the Half-Life 2 game rules
//-----------------------------------------------------------------------------
inline CPortalGameRules* PortalGameRules()
{
	return static_cast<CPortalGameRules*>(g_pGameRules);
}



#endif // PORTAL_GAMERULES_H
#endif
