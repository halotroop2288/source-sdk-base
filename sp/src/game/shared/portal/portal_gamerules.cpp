//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The Half-Life 2 game rules, such as the relationship tables and ammo
//			damage cvars.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "portal_gamerules.h"
#include "hl2_shareddefs.h"
#include "portal_shareddefs.h"

#ifdef CLIENT_DLL

#else
	#include "player.h"
	#include "game.h"
	#include "gamerules.h"
	#include "teamplay_gamerules.h"
	#include "portal_player.h"
	#include "globalstate.h"
	#include "ai_basenpc.h"
	#include "portal/weapon_physcannon.h"
	#include "props.h"		// For props flags used in making the portal weight box
	#include "datacache/imdlcache.h"	// For precaching box model
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


REGISTER_GAMERULES_CLASS( CPortalGameRules );

BEGIN_NETWORK_TABLE_NOBASE( CPortalGameRules, DT_PortalGameRules )
	#ifdef CLIENT_DLL
		RecvPropBool( RECVINFO( m_bMegaPhysgun ) ),
	#else
		SendPropBool( SENDINFO( m_bMegaPhysgun ) ),
	#endif
END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( portal_gamerules, CPortalGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( PortalGameRulesProxy, DT_PortalGameRulesProxy )


#ifdef CLIENT_DLL
	void RecvProxy_PortalGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CPortalGameRules *pRules = PortalGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CPortalGameRulesProxy, DT_PortalGameRulesProxy )
		RecvPropDataTable( "portal_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_PortalGameRules ), RecvProxy_PortalGameRules )
	END_RECV_TABLE()
#else
	void* SendProxy_PortalGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CPortalGameRules *pRules = PortalGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CPortalGameRulesProxy, DT_PortalGameRulesProxy )
		SendPropDataTable( "portal_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_PortalGameRules ), SendProxy_PortalGameRules )
	END_SEND_TABLE()
#endif


extern ConVar	sv_robust_explosions;

#if !defined ( CLIENT_DLL )
extern ConVar	sv_alternateticks;
#endif // !CLIENT_DLL

#define PORTAL_WEIGHT_BOX_MODEL_NAME "models/props/metal_box.mdl"

// Portal-only con commands

#ifndef CLIENT_DLL
// Create the box used for portal puzzles, named 'box'. Used for easy debugging of portal puzzles.
void CC_Create_PortalWeightBox( void )
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Try to create entity
	CBaseEntity *entity = dynamic_cast< CBaseEntity * >( CreateEntityByName("prop_physics") );
	if (entity)
	{
		entity->PrecacheModel( PORTAL_WEIGHT_BOX_MODEL_NAME );
		entity->SetModel( PORTAL_WEIGHT_BOX_MODEL_NAME );
		entity->SetName( MAKE_STRING("box") );
		entity->AddSpawnFlags( SF_PHYSPROP_ENABLE_PICKUP_OUTPUT );
		entity->Precache();
		DispatchSpawn(entity);

		// Now attempt to drop into the world
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_SOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0 )
		{
			tr.endpos.z += 12;
			entity->Teleport( &tr.endpos, NULL, NULL );
			UTIL_DropToFloor( entity, MASK_SOLID );
		}
	}
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
static ConCommand ent_create_portal_weight_box("ent_create_portal_weight_box", CC_Create_PortalWeightBox, "Creates a weight box used in portal puzzles at the location the player is looking.", FCVAR_GAMEDLL | FCVAR_CHEAT);
#endif // CLIENT_DLL


#define PORTAL_METAL_SPHERE_MODEL_NAME "models/props/sphere.mdl"

#ifndef CLIENT_DLL
// Create a very reflective bouncy metal sphere
void CC_Create_PortalMetalSphere( void )
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Try to create entity
	CBaseEntity *entity = dynamic_cast< CBaseEntity * >( CreateEntityByName("prop_physics") );
	if (entity)
	{
		entity->PrecacheModel( PORTAL_METAL_SPHERE_MODEL_NAME );
		entity->SetModel( PORTAL_METAL_SPHERE_MODEL_NAME );
		entity->SetName( MAKE_STRING("sphere") );
		entity->AddSpawnFlags( SF_PHYSPROP_ENABLE_PICKUP_OUTPUT );
		entity->Precache();
		DispatchSpawn(entity);

		// Now attempt to drop into the world
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_SOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0 )
		{
			tr.endpos.z += 12;
			entity->Teleport( &tr.endpos, NULL, NULL );
			UTIL_DropToFloor( entity, MASK_SOLID );
		}
	}
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
static ConCommand ent_create_portal_metal_sphere("ent_create_portal_metal_sphere", CC_Create_PortalMetalSphere, "Creates a reflective metal sphere where the player is looking.", FCVAR_GAMEDLL | FCVAR_CHEAT);
#endif // CLIENT_DLL



#ifdef CLIENT_DLL //{


#else //}{

	extern bool		g_fGameOver;
	
	//-----------------------------------------------------------------------------
	// Purpose:
	// Input  :
	// Output :
	//-----------------------------------------------------------------------------
	CPortalGameRules::CPortalGameRules()
	{
		m_bMegaPhysgun = false;
		g_pCVar->FindVar( "sv_maxreplay" )->SetValue( "1.5" );
	}


	//-----------------------------------------------------------------------------
	// Purpose: called each time a player uses a "cmd" command
	// Input  : *pEdict - the player who issued the command
	//			Use engine.Cmd_Argv,  engine.Cmd_Argv, and engine.Cmd_Argc to get 
	//			pointers the character string command.
	//-----------------------------------------------------------------------------
	bool CPortalGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
	{
		if( BaseClass::ClientCommand( pEdict, args ) )
			return true;

		CPortal_Player *pPlayer = (CPortal_Player *) pEdict;

		if ( pPlayer->ClientCommand( args ) )
			return true;

		return false;
	}

	void CPortalGameRules::Think( void )
	{
		BaseClass::Think();
	}

	ConVar sk_falldamage("sk_falldamage","0",FCVAR_REPLICATED);

	float CPortalGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
	{
		// No fall damage in Portal!
		if (sk_falldamage.GetBool())
			return CHalfLife2::FlPlayerFallDamage(pPlayer);
		else
			return 0.0f;
	}


#endif //} !CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: On starting a game, make global state changes specific to portal
//-----------------------------------------------------------------------------
bool CPortalGameRules::Init()
{
#if !defined ( CLIENT_DLL )
	// Portal never wants alternate ticks. Some low end hardware sets it in dxsupport.cfg so this will catch those cases.
	sv_alternateticks.SetValue( 0 );
#endif // !CLIENT_DLL

	return BaseClass::Init();
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CPortalGameRules::ShouldUseRobustRadiusDamage(CBaseEntity *pEntity)
{
#ifdef CLIENT_DLL
	return false;
#endif

	if( !sv_robust_explosions.GetBool() )
		return false;

	if( !pEntity->IsNPC() )
	{
		// Only NPC's
		return false;
	}

#ifndef CLIENT_DLL
	CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
	if( pNPC->CapabilitiesGet() & bits_CAP_SIMPLE_RADIUS_DAMAGE )
	{
		// This NPC only eligible for simple radius damage.
		return false;
	}
#endif//CLIENT_DLL

	return true;
}

#ifdef CLIENT_DLL

bool CPortalGameRules::IsBonusChallengeTimeBased( void )
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex( 1 );
	if ( !pPlayer )
		return true;

	int iBonusChallenge = pPlayer->GetBonusChallenge();
	if ( iBonusChallenge == PORTAL_CHALLENGE_TIME || iBonusChallenge == PORTAL_CHALLENGE_NONE )
		return true;

	return false;
}

#endif
