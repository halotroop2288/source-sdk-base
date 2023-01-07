//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "vgui_controls/MessageDialog.h"
#include "vgui/ILocalize.h"
#include <vgui/IInput.h>
#include "vgui/ISurface.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// CMessageDialog
//-----------------------------------------------------------------------------
CMessageDialog::CMessageDialog( vgui::Panel *pParent, const uint nType, const char *pTitle, const char *pMsg, const char *pCmdA, const char *pCmdB, vgui::Panel *pCreator, bool bShowActivity  ) 
	: BaseClass( pParent, "MessageDialog" )
{
	SetSize( 500, 200 );
	SetDeleteSelfOnClose( true );
	SetTitleBarVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );

	m_pControlSettings = NULL;
	m_pCreator = pCreator ? pCreator : pParent;

	m_nType = nType;
	m_pTitle = new vgui::Label( this, "TitleLabel", pTitle );
	m_pMsg = new vgui::Label( this, "MessageLabel", pMsg );
	m_pAnimatingPanel = new vgui::AnimatingImagePanel( this, "AnimatingPanel" );

	m_bShowActivity = bShowActivity;

	if ( nType & MD_SIMPLEFRAME )
	{
		SetPaintBackgroundEnabled( true );
		m_pBackground = NULL;
	}
	else
	{
		m_pBackground = new vgui::ImagePanel( this, "Background" );
 		if ( nType & MD_WARNING )
 		{
 			m_pBackground->SetName( "WarningBackground" );
 		}
 		else if ( nType & MD_ERROR )
 		{
 			m_pBackground->SetName( "ErrorBackground" );
 		}
	}

	Q_memset( m_pCommands, 0, sizeof( m_pCommands ) );
	Q_memset( m_Buttons, 0, sizeof( m_Buttons ) );

	if ( pCmdA )
	{
		const int len = Q_strlen( pCmdA ) + 1;
		m_pCommands[BTN_A] = (char*)malloc( len );
		Q_strncpy( m_pCommands[BTN_A], pCmdA, len );
	}

	if ( pCmdB )
	{
		const int len = Q_strlen( pCmdB ) + 1;
		m_pCommands[BTN_B] = (char*)malloc( len );
		Q_strncpy( m_pCommands[BTN_B], pCmdB, len );
	}

	// invalid until pressed
	m_ButtonPressed = BTN_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMessageDialog::~CMessageDialog()
{
	if ( m_ButtonPressed != BTN_INVALID && ( m_nType & MD_COMMANDAFTERCLOSE ) )
	{
		// caller wants the command sent after closure
		m_pCreator->OnCommand( m_pCommands[m_ButtonPressed] );
	}
	else if ( m_nType & MD_COMMANDONFORCECLOSE )
	{
		// caller wants the command sent after closure
		m_pCreator->OnCommand( m_pCommands[BTN_A] );
	}

	for ( int i = 0; i < MAX_BUTTONS; ++i )
	{
		free( m_pCommands[i] );
		m_pCommands[i] = NULL;

		delete m_Buttons[i].pIcon;
		delete m_Buttons[i].pText;
	}

	delete m_pTitle;
	m_pTitle = NULL;

	delete m_pMsg;
	m_pMsg = NULL;

	delete m_pBackground;
	m_pBackground = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Set the keyvalues to pass to LoadControlSettings()
//-----------------------------------------------------------------------------
void CMessageDialog::SetControlSettingsKeys( KeyValues *pKeys )
{
	m_pControlSettings = pKeys;
}

//-----------------------------------------------------------------------------
// Purpose: Create a new button
//-----------------------------------------------------------------------------
void CMessageDialog::CreateButtonLabel( ButtonLabel_s *pButton, const char *pIcon, const char *pText )
{
	pButton->nWide = m_ButtonIconLabelSpace;
	pButton->bCreated = true;

	pButton->pIcon = new vgui::Label( this, "icon", pIcon );
	SETUP_PANEL( pButton->pIcon );
	pButton->pIcon->SetFont( m_hButtonFont );
	pButton->pIcon->SizeToContents();
	pButton->nWide += pButton->pIcon->GetWide();

	pButton->pText = new vgui::Label( this, "text", pText );
	SETUP_PANEL( pButton->pText );
	pButton->pText->SetFont( m_hTextFont );
	pButton->pText->SizeToContents();
	pButton->pText->SetFgColor( m_ButtonTextColor );
	pButton->nWide += pButton->pText->GetWide();
}

//-----------------------------------------------------------------------------
// Purpose: Create and arrange the panel button labels according to the dialog type
//-----------------------------------------------------------------------------
void CMessageDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/MessageDialog.res", "GAME", m_pControlSettings );

	m_hButtonFont = pScheme->GetFont( "GameUIButtons" );
	m_hTextFont = pScheme->GetFont( "MenuLarge" );

	if ( m_nType & MD_OK )
	{
		CreateButtonLabel( &m_Buttons[BTN_A], "#GameUI_Icons_A_BUTTON", "#GameUI_OK" );
	}
	else if ( m_nType & MD_CANCEL )
	{
		CreateButtonLabel( &m_Buttons[BTN_B], "#GameUI_Icons_B_BUTTON", "#GameUI_Cancel" );
	}
	else if ( m_nType & MD_OKCANCEL )
	{
		CreateButtonLabel( &m_Buttons[BTN_A], "#GameUI_Icons_B_BUTTON", "#GameUI_Cancel" );
		CreateButtonLabel( &m_Buttons[BTN_B], "#GameUI_Icons_A_BUTTON", "#GameUI_OK" );
	}
	else if ( m_nType & MD_YESNO )
	{
		CreateButtonLabel( &m_Buttons[BTN_A], "#GameUI_Icons_B_BUTTON", "#GameUI_No" );
		CreateButtonLabel( &m_Buttons[BTN_B], "#GameUI_Icons_A_BUTTON", "#GameUI_Yes" );
	}

	// count the buttons and add up their widths
	int cButtons = 0;
	int nTotalWide = 0;
	for ( int i = 0; i < MAX_BUTTONS; ++i )
	{
		if ( m_Buttons[i].bCreated )
		{
			++cButtons;
			nTotalWide += m_Buttons[i].nWide;
		}
	}

	// make sure text and icons are center-aligned vertically with each other
	int nButtonTall = vgui::surface()->GetFontTall( m_hButtonFont );
	int nTextTall = vgui::surface()->GetFontTall( m_hTextFont );
	int nVerticalAdjust = ( nButtonTall - nTextTall ) / 2;

	// position the buttons with even horizontal spacing
	int xpos = 0;


	//STAVAAS Res

//480p/720p
#if defined(_X360UI480p) || defined(_X360UI720p)
	int ypos = GetTall() - 48;
#endif

//1080p
#ifdef _X360UI1080p
	int ypos = GetTall() - 72;
#endif

//1440p
#ifdef _X360UI1440p
	int ypos = GetTall() - 96;
#endif

//2160p
#ifdef _X360UI2160p
	int ypos = GetTall() - 144;
#endif


	int nSpacing = ( GetWide() - nTotalWide ) / ( cButtons + 1 );
	for ( int i = 0; i < MAX_BUTTONS; ++i )
	{
		if ( m_Buttons[i].bCreated )
		{
			xpos += nSpacing;
			m_Buttons[i].pIcon->SetPos( xpos, ypos );
			xpos += m_Buttons[i].pIcon->GetWide() + m_ButtonIconLabelSpace;
			m_Buttons[i].pText->SetPos( xpos, ypos + nVerticalAdjust );
			xpos += m_Buttons[i].pText->GetWide();
		}
	}

	m_clrNotSimpleBG = pScheme->GetColor( "MessageDialog.MatchmakingBG", Color( 200, 184, 151, 255 ) );
	m_clrNotSimpleBGBlack = pScheme->GetColor( "MessageDialog.MatchmakingBGBlack", Color( 52, 48, 55, 255 ) );

	if ( !m_bShowActivity )
	{
		if ( m_pAnimatingPanel )
		{
			if ( m_pAnimatingPanel->IsVisible() )
			{

				m_pAnimatingPanel->SetVisible( false );
			}

			m_pAnimatingPanel->StopAnimation();
		}
	}
	else
	{
		if ( m_pAnimatingPanel )
		{
			if ( !m_pAnimatingPanel->IsVisible() )
			{
				m_pAnimatingPanel->SetVisible( true );
			}

			m_pAnimatingPanel->StartAnimation();
		}
	}

	MoveToCenterOfScreen();

	if ( m_bShowActivity && m_ActivityIndent )
	{
		// If we're animating, we push our text label in, and reduce its width
		int iX,iY,iW,iH;
		m_pMsg->GetBounds( iX, iY, iW, iH );
		m_pMsg->SetBounds( iX + m_ActivityIndent, iY, max(0,iW-m_ActivityIndent), iH );
	}

	// Invalidate the scheme on our message label so that it recalculates 
	// its line breaks in case it was resized when we loaded our .res file.
	m_pMsg->InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: Create and arrange the panel button labels according to the dialog type
//-----------------------------------------------------------------------------
void CMessageDialog::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_pTitle->SetFgColor( inResourceData->GetColor( "titlecolor" ) );

	m_pMsg->SetFgColor( inResourceData->GetColor( "messagecolor" ) );

	m_ButtonTextColor = inResourceData->GetColor( "buttontextcolor" );


	//STAVAAS Res

//480p/720p
#if defined(_X360UI480p) || defined(_X360UI720p)
	m_FooterTall = inResourceData->GetInt("footer_tall", 48);
	m_ButtonMargin = inResourceData->GetInt("button_margin", 25);
	m_ButtonIconLabelSpace = inResourceData->GetInt("button_separator", 10);
#endif

//1080p
#ifdef _X360UI1080p
	m_FooterTall = inResourceData->GetInt("footer_tall", 72);
	m_ButtonMargin = inResourceData->GetInt("button_margin", 38);
	m_ButtonIconLabelSpace = inResourceData->GetInt("button_separator", 15);
#endif

//1440p
#ifdef _X360UI1440p
	m_FooterTall = inResourceData->GetInt("footer_tall", 96);
	m_ButtonMargin = inResourceData->GetInt("button_margin", 50);
	m_ButtonIconLabelSpace = inResourceData->GetInt("button_separator", 20);
#endif

//2160p
#ifdef _X360UI2160p
	m_FooterTall = inResourceData->GetInt( "footer_tall", 144 );
	m_ButtonMargin = inResourceData->GetInt( "button_margin", 75 );
	m_ButtonIconLabelSpace = inResourceData->GetInt( "button_separator", 30 );
#endif


	m_ActivityIndent = inResourceData->GetInt( "activity_indent", 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
uint CMessageDialog::GetType( void )
{
	return m_nType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageDialog::DoCommand( int button )
{
	if ( button == BTN_INVALID || ( m_nType & MD_COMMANDONFORCECLOSE ) )
	{
		return;
	}

	if ( m_pCommands[button] )
	{
		m_ButtonPressed = button;
		if ( !( m_nType & MD_COMMANDAFTERCLOSE ) )
		{
			// caller wants the command sent before closure
			m_pCreator->OnCommand( m_pCommands[m_ButtonPressed] );
		}
		m_pCreator->OnCommand( "ReleaseModalWindow" );
		Close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageDialog::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( m_ButtonPressed != BTN_INVALID || GetAlpha() != 255 )
	{
		// inhibit any further key activity or during transitions
		return;
	}

	switch ( GetBaseButtonCode( code ) )
	{
	case KEY_XBUTTON_A:
	case KEY_ENTER:
		DoCommand( BTN_A );
		break;

	case KEY_XBUTTON_B:
	case KEY_BACKSPACE:
		DoCommand( BTN_B );
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageDialog::PaintBackground( void )
{

	int wide, tall;
	GetSize( wide, tall );

	if ( !( m_nType & MD_SIMPLEFRAME ) )
	{
		int nAlpha = GetAlpha();

		m_clrNotSimpleBG[3] = nAlpha;
		m_clrNotSimpleBGBlack[3] = nAlpha;

		DrawBox( 0, 0, wide, tall, m_clrNotSimpleBGBlack, 1.0f );	
		DrawBox( 0, 0, wide, tall - m_FooterTall, m_clrNotSimpleBG, 1.0f );	

		return;
	}

	Color col = GetBgColor();
	DrawBox( 0, 0, wide, tall, col, 1.0f );

	// offset the inset by title
	int titleX, titleY, titleWide, titleTall;
	m_pTitle->GetBounds( titleX, titleY, titleWide, titleTall );	
	// int y = titleY + titleTall;

	// draw an inset
	Color darkColor;
	darkColor.SetColor( 0.70f * (float)col.r(), 0.70f * (float)col.g(), 0.70f * (float)col.b(), col.a() );
	vgui::surface()->DrawSetColor( darkColor );

	//STAVAAS Res

//480p/720p
#if defined(_X360UI480p) || defined(_X360UI720p)
	vgui::surface()->DrawFilledRect(8, 35, wide - 8, tall - 8);
#endif

//1080p
#ifdef _X360UI1080p
	vgui::surface()->DrawFilledRect(12, 54, wide - 12, tall - 12);
#endif

//1440p
#ifdef _X360UI1440p
	vgui::surface()->DrawFilledRect(16, 70, wide - 16, tall - 16);
#endif

//2160p
#ifdef _X360UI2160p
	vgui::surface()->DrawFilledRect(24, 108, wide - 24, tall - 24);
#endif

}
