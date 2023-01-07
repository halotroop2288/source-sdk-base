//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "TGAImagePanel.h"
#include "bitmap/tgaloader.h"
#include "vgui/ISurface.h"
#include "BasePanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CTGAImagePanel::CTGAImagePanel( vgui::Panel *parent, const char *name ) : BaseClass( parent, name )
{
	m_iTextureID = -1;
	m_bHasValidTexture = false;
	m_bLoadedTexture = false;
	m_szTGAName[0] = 0;

	SetPaintBackgroundEnabled( false );
}

CTGAImagePanel::~CTGAImagePanel()
{
	// release the texture memory
}

void CTGAImagePanel::SetTGA( const char *filename )
{
	Q_snprintf( m_szTGAName, sizeof(m_szTGAName), "//MOD/%s", filename );
}

void CTGAImagePanel::SetTGANonMod( const char *filename )
{
	Q_strcpy( m_szTGAName, filename );
}

void CTGAImagePanel::Paint()
{
	if ( !m_bLoadedTexture )
	{
		m_bLoadedTexture = true;
		// get a texture id, if we haven't already
		if ( m_iTextureID < 0 )
		{
			m_iTextureID = vgui::surface()->CreateNewTextureID( true );

			//STAVAAS Res
		
		//480p
#ifdef _X360UI480p
			SetSize(107, 61);
#endif

		//720p
#ifdef _X360UI720p
			SetSize( 152, 86 );
#endif

		//1080p
#ifdef _X360UI1080p
			SetSize( 228, 129 );
#endif

		//1440p
#ifdef _X360UI1440p
			SetSize( 304, 172 );
#endif

		//2160p
#ifdef _X360UI2160p
			SetSize( 456, 258 );
#endif
		
		}

		// load the file
		CUtlMemory<unsigned char> tga;
		if ( TGALoader::LoadRGBA8888( m_szTGAName, tga, m_iImageWidth, m_iImageHeight ) )
		{
			// set the textureID
			surface()->DrawSetTextureRGBA( m_iTextureID, tga.Base(), m_iImageWidth, m_iImageHeight, false, true );
			m_bHasValidTexture = true;
			// set our size to be the size of the tga
			SetSize( m_iImageWidth, m_iImageHeight );
		}
		else
		{
			m_bHasValidTexture = false;
		}
	}

	// draw the image
	int wide, tall;
	if ( m_bHasValidTexture )
	{
		surface()->DrawGetTextureSize( m_iTextureID, wide, tall );
		surface()->DrawSetTexture( m_iTextureID );
		surface()->DrawSetColor( 255, 255, 255, 255 );
		surface()->DrawTexturedRect( 0, 0, wide, tall );
	}
	else
	{
		// draw a black fill instead

		//STAVAAS Res
	
	//480p
#ifdef _X360UI480p
		wide = 107, tall = 61;
#endif

	//720p
#ifdef _X360UI720p
		wide = 152, tall = 86;
#endif

	//1080p
#ifdef _X360UI1080p
		wide = 228, tall = 129;
#endif

	//1440p
#ifdef _X360UI1440p
		wide = 304, tall = 172;
#endif

	//2160p
#ifdef _X360UI2160p
		wide = 456, tall = 258;
#endif


		surface()->DrawSetColor( 0, 0, 0, 0 );
		surface()->DrawFilledRect( 0, 0, wide, tall );
	}
}
