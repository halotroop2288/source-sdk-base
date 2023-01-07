//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BasePanel.h"
#include "NewGameDialog.h"
#include "EngineInterface.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/CheckButton.h"
#include "KeyValues.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include <vgui/ISystem.h>
#include "vgui_controls/RadioButton.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/ControllerMap.h"
#include "FileSystem.h"
#include "ModInfo.h"
#include "tier1/convar.h"
#include "GameUI_Interface.h"
#include "tier0/icommandline.h"
#include "vgui_controls/AnimationController.h"
#include "CommentaryExplanationDialog.h"
#include "vgui_controls/BitmapImagePanel.h"
#include "BonusMapsDatabase.h"

#include <stdio.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static float	g_ScrollSpeedSlow;
static float	g_ScrollSpeedFast;

// sort function used in displaying chapter list
struct chapter_t
{
	char filename[32];
};
static int __cdecl ChapterSortFunc(const void *elem1, const void *elem2)
{
	chapter_t *c1 = (chapter_t *)elem1;
	chapter_t *c2 = (chapter_t *)elem2;

	// compare chapter number first
	static int chapterlen = strlen("chapter");
	if (atoi(c1->filename + chapterlen) > atoi(c2->filename + chapterlen))
		return 1;
	else if (atoi(c1->filename + chapterlen) < atoi(c2->filename + chapterlen))
		return -1;

	// compare length second (longer string show up later in the list, eg. chapter9 before chapter9a)
	if (strlen(c1->filename) > strlen(c2->filename))
		return 1;
	else if (strlen(c1->filename) < strlen(c2->filename))
		return -1;

	// compare strings third
	return strcmp(c1->filename, c2->filename);
}

//-----------------------------------------------------------------------------
// Purpose: invisible panel used for selecting a chapter panel
//-----------------------------------------------------------------------------
class CSelectionOverlayPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CSelectionOverlayPanel, Panel );
	int m_iChapterIndex;
	CNewGameDialog *m_pSelectionTarget;
public:
	CSelectionOverlayPanel( Panel *parent, CNewGameDialog *selectionTarget, int chapterIndex ) : BaseClass( parent, NULL )
	{
		m_iChapterIndex = chapterIndex;
		m_pSelectionTarget = selectionTarget;
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
	}

	virtual void OnMousePressed( vgui::MouseCode code )
	{/*
		if (GetParent()->IsEnabled())
		{
			m_pSelectionTarget->SetSelectedChapterIndex( m_iChapterIndex );
		}*/
		return;
	}

	virtual void OnMouseDoublePressed( vgui::MouseCode code )
	{
/*		// call the panel
		OnMousePressed( code );
		if (GetParent()->IsEnabled())
		{
			PostMessage( m_pSelectionTarget, new KeyValues("Command", "command", "play") );
		}*/
		return;
	}
};

//-----------------------------------------------------------------------------
// Purpose: selectable item with screenshot for an individual chapter in the dialog
//-----------------------------------------------------------------------------
class CGameChapterPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CGameChapterPanel, vgui::EditablePanel );

	ImagePanel *m_pLevelPicBorder;
	ImagePanel *m_pLevelPic;
	ImagePanel *m_pCommentaryIcon;
	Label *m_pChapterLabel;
	Label *m_pChapterNameLabel;

	Color m_TextColor;
	Color m_TextChColor;
	Color m_DisabledColor;
	Color m_SelectedColor;
	Color m_FillColor;

	char m_szConfigFile[_MAX_PATH];
	char m_szChapter[32];

	bool m_bTeaserChapter;
	bool m_bHasBonus;
	bool m_bCommentaryMode;

public:
	CGameChapterPanel( CNewGameDialog *parent, const char *name, const char *chapterName, int chapterIndex, const char *chapterNumber, const char *chapterConfigFile, bool bCommentary ) : BaseClass( parent, name )
	{
		Q_strncpy( m_szConfigFile, chapterConfigFile, sizeof(m_szConfigFile) );
		Q_strncpy( m_szChapter, chapterNumber, sizeof(m_szChapter) );

		m_pLevelPicBorder = SETUP_PANEL( new ImagePanel( this, "LevelPicBorder" ) );
		m_pLevelPic = SETUP_PANEL( new ImagePanel( this, "LevelPic" ) );
		m_pCommentaryIcon = NULL;
		m_bCommentaryMode = bCommentary;

		wchar_t text[32];
		wchar_t num[32];
		wchar_t *chapter = g_pVGuiLocalize->Find("#GameUI_Chapter");
		g_pVGuiLocalize->ConvertANSIToUnicode( chapterNumber, num, sizeof(num) );
		_snwprintf( text, sizeof(text), L"%s %s", chapter ? chapter : L"CHAPTER", num );

		if ( ModInfo().IsSinglePlayerOnly() )
		{
			m_pChapterLabel = new Label( this, "ChapterLabel", text );
			m_pChapterNameLabel = new Label( this, "ChapterNameLabel", chapterName );
		}
		else
		{
			m_pChapterLabel = new Label( this, "ChapterLabel", chapterName );
			m_pChapterNameLabel = new Label( this, "ChapterNameLabel", "#GameUI_LoadCommentary" );
		}

		SetPaintBackgroundEnabled( false );

		// the image has the same name as the config file
		char szMaterial[ MAX_PATH ];
		Q_snprintf( szMaterial, sizeof(szMaterial), "chapters/%s", chapterConfigFile );
		char *ext = strstr( szMaterial, "." );
		if ( ext )
		{
			*ext = 0;
		}
		m_pLevelPic->SetImage( szMaterial );

		KeyValues *pKeys = NULL;
		if ( GameUI().IsConsoleUI() )
		{
			pKeys = BasePanel()->GetConsoleControlSettings()->FindKey( "NewGameChapterPanel.res" );
		}
		LoadControlSettings( "Resource/NewGameChapterPanel.res", NULL, pKeys );

		int px, py;
		m_pLevelPicBorder->GetPos( px, py );
		SetSize( m_pLevelPicBorder->GetWide(), py + m_pLevelPicBorder->GetTall() );

		// create a selection panel the size of the page
		CSelectionOverlayPanel *overlay = new CSelectionOverlayPanel( this, parent, chapterIndex );
		overlay->SetBounds(0, 0, GetWide(), GetTall());
		overlay->MoveToFront();

		// HACK: Detect new episode teasers by the "Coming Soon" text
		wchar_t w_szStrTemp[256];
		m_pChapterNameLabel->GetText( w_szStrTemp, sizeof(w_szStrTemp)  );
		m_bTeaserChapter = !wcscmp(w_szStrTemp, L"Coming Soon");
		m_bHasBonus = false;
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		m_TextColor = pScheme->GetColor( "NewGame.TextColor", Color(255, 255, 255, 255) );
		m_TextChColor = pScheme->GetColor( "OptSelectedTextColor", Color(255, 255, 255, 255) );
		m_FillColor = pScheme->GetColor( "NewGame.FillColor", Color(255, 255, 255, 255) );
		m_DisabledColor = pScheme->GetColor( "NewGame.DisabledColor", Color(255, 255, 255, 255) );
		m_SelectedColor = pScheme->GetColor( "NewGame.SelectionColor", Color(255, 255, 255, 255) );

		BaseClass::ApplySchemeSettings( pScheme );

		// Hide chapter numbers for new episode teasers
		if ( m_bTeaserChapter )
		{
			m_pChapterLabel->SetVisible( false );
		}
		if ( GameUI().IsConsoleUI() )
		{
			m_pChapterNameLabel->SetVisible( false );
		}

		m_pCommentaryIcon = dynamic_cast<ImagePanel*>( FindChildByName( "CommentaryIcon" ) );
		if ( m_pCommentaryIcon )
			m_pCommentaryIcon->SetVisible( m_bCommentaryMode );
	}

	void SetSelected( bool state )
	{
		// update the text/border colors
		if ( !IsEnabled() )
		{
			m_pChapterLabel->SetFgColor( m_DisabledColor );
			m_pChapterNameLabel->SetFgColor( Color(0, 0, 0, 0) );
			m_pLevelPicBorder->SetFillColor( m_DisabledColor );
			m_pLevelPic->SetAlpha( GameUI().IsConsoleUI() ? 64 : 128 );
			return;
		}

		if ( state )
		{
			if ( !GameUI().IsConsoleUI() )
			{
				m_pChapterLabel->SetFgColor( m_SelectedColor );
				m_pChapterNameLabel->SetFgColor( m_SelectedColor );
			}
			m_pLevelPicBorder->SetFillColor( m_SelectedColor );
		}
		else
		{
			m_pChapterLabel->SetFgColor( m_TextChColor );
			m_pChapterNameLabel->SetFgColor( m_TextColor );
			m_pLevelPicBorder->SetFillColor( m_FillColor );
		}
		m_pLevelPic->SetAlpha( 255 );
	}

	const char *GetConfigFile()
	{
		return m_szConfigFile;
	}

	const char *GetChapter()
	{
		return m_szChapter;
	}

	bool IsTeaserChapter()
	{
		return m_bTeaserChapter;
	}

	bool HasBonus()
	{
		return m_bHasBonus;
	}

	void SetCommentaryMode( bool bCommentaryMode )
	{
		m_bCommentaryMode = bCommentaryMode;
		if ( m_pCommentaryIcon )
			m_pCommentaryIcon->SetVisible( m_bCommentaryMode );
	}
};

const char *COM_GetModDirectory()
{
	static char modDir[MAX_PATH];
	if ( Q_strlen( modDir ) == 0 )
	{
		const char *gamedir = CommandLine()->ParmValue("-game", CommandLine()->ParmValue( "-defaultgamedir", "hl2" ) );
		Q_strncpy( modDir, gamedir, sizeof(modDir) );
		if ( strchr( modDir, '/' ) || strchr( modDir, '\\' ) )
		{
			Q_StripLastDir( modDir, sizeof(modDir) );
			int dirlen = Q_strlen( modDir );
			Q_strncpy( modDir, gamedir + dirlen, sizeof(modDir) - dirlen );
		}
	}

	return modDir;
}

//-----------------------------------------------------------------------------
// Purpose: new game chapter selection
//-----------------------------------------------------------------------------
CNewGameDialog::CNewGameDialog(vgui::Panel *parent, bool bCommentaryMode) : BaseClass(parent, "NewGameDialog")
{
	SetDeleteSelfOnClose(true);
	SetBounds(0, 0, 372, 160);
	SetSizeable( false );
	m_iSelectedChapter = -1;
	m_ActiveTitleIdx = 0;

	m_bCommentaryMode = bCommentaryMode;
	m_bMapStarting = false;
	m_bScrolling = false;
	m_ScrollCt = 0;
	m_ScrollSpeed = 0.f;
	m_ButtonPressed = SCROLL_NONE;
	m_ScrollDirection = SCROLL_NONE;
	m_pCommentaryLabel = NULL;

	m_iBonusSelection = 0;
	m_bScrollToFirstBonusMap = false;

	SetTitle("#GameUI_NewGame", true);

	m_pNextButton = new Button( this, "Next", "#gameui_next" );
	m_pPrevButton = new Button( this, "Prev", "#gameui_prev" );
	m_pPlayButton = new vgui::Button( this, "Play", "#GameUI_Play" );
	m_pPlayButton->SetCommand( "Play" );

	vgui::Button *cancel = new vgui::Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	m_pCenterBg = SETUP_PANEL( new Panel( this, "CenterBG" ) );
	m_pCenterBg->SetVisible( false );

	if ( GameUI().IsConsoleUI() )
	{
		m_pNextButton->SetVisible( false );
		m_pPrevButton->SetVisible( false );
		m_pPlayButton->SetVisible( false );
		cancel->SetVisible( false );

		m_pCenterBg->SetPaintBackgroundType( 2 );
		m_pCenterBg->SetVisible( true );

		m_pChapterTitleLabels[0] = SETUP_PANEL( new Label( this, "ChapterTitleLabel", "" ) );
		m_pChapterTitleLabels[0]->SetVisible( true );
		m_pChapterTitleLabels[0]->SetFgColor( Color( 255, 255, 255, 255 ) );

		m_pChapterTitleLabels[1] = SETUP_PANEL( new Label( this, "ChapterTitleLabel2", "" ) );
		m_pChapterTitleLabels[1]->SetVisible( true );
		m_pChapterTitleLabels[1]->SetAlpha( 0 );
		m_pChapterTitleLabels[1]->SetFgColor( Color( 255, 255, 255, 255 ) );

		m_pBonusSelection = SETUP_PANEL( new Label( this, "BonusSelectionLabel", "#GameUI_BonusMapsStandard" ) );
		m_pBonusSelectionBorder = SETUP_PANEL( new ImagePanel( this, "BonusSelectionBorder" ) );

		m_pFooter = new CFooterPanel( parent, "NewGameFooter" );
		m_pFooter->AddNewButtonLabel( "#GameUI_Close", "#GameUI_Icons_B_BUTTON" );
		m_pFooter->AddNewButtonLabel( "#GameUI_Play", "#GameUI_Icons_A_BUTTON" );
	}
	else
	{
		m_pFooter = NULL;
	}

	// parse out the chapters off disk
	static const int MAX_CHAPTERS = 32;
	chapter_t chapters[MAX_CHAPTERS];

	char szFullFileName[MAX_PATH];
	int chapterIndex = 0;

	FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	const char *fileName = "cfg/chapter*.cfg";
	fileName = g_pFullFileSystem->FindFirst( fileName, &findHandle );
	while ( fileName && chapterIndex < MAX_CHAPTERS )
	{
		// Only load chapter configs from the current mod's cfg dir
		// or else chapters appear that we don't want!
		Q_snprintf( szFullFileName, sizeof(szFullFileName), "cfg/%s", fileName );
		FileHandle_t f = g_pFullFileSystem->Open( szFullFileName, "rb", "MOD" );
		if ( f )
		{	
			// don't load chapter files that are empty, used in the demo
			if ( g_pFullFileSystem->Size(f) > 0	)
			{
				Q_strncpy(chapters[chapterIndex].filename, fileName, sizeof(chapters[chapterIndex].filename));
				++chapterIndex;
			}
			g_pFullFileSystem->Close( f );
		}
		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	bool bBonusesUnlocked = false;

	if ( GameUI().IsConsoleUI() )
	{
		if ( !m_bCommentaryMode )
		{
			// Scan to see if the bonus maps have been unlocked
			bBonusesUnlocked = BonusMapsDatabase()->BonusesUnlocked();
		}
	}

	// sort the chapters
	qsort(chapters, chapterIndex, sizeof(chapter_t), &ChapterSortFunc);

	// work out which chapters are unlocked
	ConVarRef var( "sv_unlockedchapters" );

	if ( bBonusesUnlocked )
	{
		// Bonuses are unlocked so we need to unlock all the chapters too
		var.SetValue( 15 );
	}

	const char *unlockedChapter = var.IsValid() ? var.GetString() : "1";
	int iUnlockedChapter = atoi(unlockedChapter);

	// add chapters to combobox
	for (int i = 0; i < chapterIndex; i++)
	{
		const char *fileName = chapters[i].filename;
		char chapterID[32] = { 0 };
		sscanf(fileName, "chapter%s", chapterID);
		// strip the extension
		char *ext = V_stristr(chapterID, ".cfg");
		if (ext)
		{
			*ext = 0;
		}

		const char *pGameDir = COM_GetModDirectory();

		char chapterName[64];
		Q_snprintf(chapterName, sizeof(chapterName), "#%s_Chapter%s_Title", pGameDir, chapterID);

		Q_snprintf( szFullFileName, sizeof( szFullFileName ), "%s", fileName );
		CGameChapterPanel *chapterPanel = SETUP_PANEL( new CGameChapterPanel( this, NULL, chapterName, i, chapterID, szFullFileName, m_bCommentaryMode ) );
		chapterPanel->SetVisible( false );

		UpdatePanelLockedStatus( iUnlockedChapter, i + 1, chapterPanel );

		if ( GameUI().IsConsoleUI() )
		{
			if ( bBonusesUnlocked )
			{
				// check to see if it has associated challenges
				for ( int iBonusMap = 0; iBonusMap < BonusMapsDatabase()->BonusCount(); ++iBonusMap )
				{
					BonusMapDescription_t *pMap = BonusMapsDatabase()->GetBonusData( iBonusMap );
					if ( Q_stricmp( pMap->szChapterName, szFullFileName ) == 0 && !pMap->bLocked )
					{
						chapterPanel->m_bHasBonus = true;
						chapterPanel->SetControlVisible( "HasBonusLabel", true );
					}
				}
			}
		}

		m_ChapterPanels.AddToTail( chapterPanel );
	}

	KeyValues *pKeys = NULL;
	if ( GameUI().IsConsoleUI() )
	{
		 pKeys = BasePanel()->GetConsoleControlSettings()->FindKey( "NewGameDialog.res" );
	}
	LoadControlSettings( "Resource/NewGameDialog.res", NULL, pKeys );

	// Reset all properties
	for ( int i = 0; i < NUM_SLOTS; ++i )
	{
		m_PanelIndex[i] = INVALID_INDEX;
	}

	if ( !m_ChapterPanels.Count() )
	{
		UpdateMenuComponents( SCROLL_NONE );
		return;
	}

	// Layout panel positions relative to the dialog center.

	//STAVAAS Res

//480p/720p
#if defined(_X360UI480p) || defined(_X360UI720p)
	int panelWidth = m_ChapterPanels[0]->GetWide() + 16;
	int dialogWidth = GetWide();
	m_PanelXPos[2] = (dialogWidth - panelWidth) / 2 + 8;
#endif

//1080p
#ifdef _X360UI1080p
	int panelWidth = m_ChapterPanels[0]->GetWide() + 24;
	int dialogWidth = GetWide();
	m_PanelXPos[2] = ( dialogWidth - panelWidth ) / 2 + 12;
#endif

//1440p
#ifdef _X360UI1440p
	int panelWidth = m_ChapterPanels[0]->GetWide() + 32;
	int dialogWidth = GetWide();
	m_PanelXPos[2] = (dialogWidth - panelWidth) / 2 + 16;
#endif

//2160p
#ifdef _X360UI2160p
	int panelWidth = m_ChapterPanels[0]->GetWide() + 48;
	int dialogWidth = GetWide();
	m_PanelXPos[2] = (dialogWidth - panelWidth) / 2 + 24;
#endif


	m_PanelXPos[1] = m_PanelXPos[2] - panelWidth;
	m_PanelXPos[0] = m_PanelXPos[1];
	m_PanelXPos[3] = m_PanelXPos[2] + panelWidth;
	m_PanelXPos[4] = m_PanelXPos[3];

	m_PanelAlpha[0] = 0;
	m_PanelAlpha[1] = 64;
	m_PanelAlpha[2] = 255;
	m_PanelAlpha[3] = 64;
	m_PanelAlpha[4] = 0;

	int panelHeight;
	m_ChapterPanels[0]->GetSize( panelWidth, panelHeight );

	//STAVAAS Res

//480p/720p
#if defined(_X360UI480p) || defined(_X360UI720p)
	m_pCenterBg->SetWide( panelWidth + 16 );
	m_pCenterBg->SetPos(m_PanelXPos[2] - 8, 43);
#endif

//1080p
#ifdef _X360UI1080p
	m_pCenterBg->SetWide( 284 );
	m_pCenterBg->SetPos( m_PanelXPos[2] - 12, 67 );
#endif

//1440p
#ifdef _X360UI1440p
	m_pCenterBg->SetWide( panelWidth + 32 );
	m_pCenterBg->SetPos( m_PanelXPos[2] - 16, 86 );
#endif

//2160p
#ifdef _X360UI2160p
	m_pCenterBg->SetWide( 568 );
	m_pCenterBg->SetPos( m_PanelXPos[2] - 24, 134 );
#endif


	vgui::HScheme scheme = vgui::scheme()->GetScheme("ClientScheme");
	Color color = GetSchemeColor( "NGChapterBGColor", Color( 190, 115, 0, 255 ), vgui::scheme()->GetIScheme( scheme ) );

	m_pCenterBg->SetBgColor( color );

	// start the first item selected
	SetSelectedChapterIndex( 0 );

	m_KeyRepeat.SetKeyRepeatTime(KEY_XBUTTON_LEFT, 0.2);
	m_KeyRepeat.SetKeyRepeatTime(KEY_XBUTTON_RIGHT, 0.2);
	m_KeyRepeat.SetKeyRepeatTime(KEY_LEFT, 0.2);
	m_KeyRepeat.SetKeyRepeatTime(KEY_RIGHT, 0.2);
}

CNewGameDialog::~CNewGameDialog()
{
	delete m_pFooter;
	m_pFooter = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Do a fancy slide-out when we're first displayed
//-----------------------------------------------------------------------------
void CNewGameDialog::AnimateDialogStart(void)
{
	const float flAnimInTime = 0.5f;
	const float flOffset = 0.1f;

	for (int i = 0; i < 4; i++)
	{
		CGameChapterPanel *chapterPanel = m_ChapterPanels[m_PanelIndex[i]];
		if (m_PanelIndex[i] == INVALID_INDEX)
			continue;
		// Start us at the "opening" position
		if (chapterPanel)
		{
			chapterPanel->SetPos(m_PanelXPos[0], m_PanelYPos[0]);
			chapterPanel->SetAlpha(m_PanelAlpha[0]);
			chapterPanel->SetVisible(true);
			chapterPanel->SetEnabled(true);
		}
		// Now make them slide out where they're going
		GetAnimationController()->RunAnimationCommand(chapterPanel, "xpos", m_PanelXPos[i], 0, flAnimInTime + (flOffset * i), vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
		GetAnimationController()->RunAnimationCommand(chapterPanel, "ypos", m_PanelYPos[i], 0, flAnimInTime + (flOffset * i), vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
		// Panel alpha
		GetAnimationController()->RunAnimationCommand(chapterPanel, "alpha", m_PanelAlpha[i], 0, flAnimInTime + (flOffset * i), vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
	}
	// Move and fade the back label
	m_pCenterBg->SetAlpha(0);
	int nX, nY;
	m_pCenterBg->GetPos(nX, nY);


	//STAVAAS Res

//480p/720p
#if defined(_X360UI480p) || defined(_X360UI720p)
	m_pCenterBg->SetPos(m_PanelXPos[0], 43);
	GetAnimationController()->RunAnimationCommand(m_pCenterBg, "xpos", m_PanelXPos[2] - 8, 0, flAnimInTime + (flOffset * 2), vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
#endif

//1080p
#ifdef _X360UI1080p
	m_pCenterBg->SetPos( m_PanelXPos[0], 67 );
	GetAnimationController()->RunAnimationCommand(m_pCenterBg, "xpos", m_PanelXPos[2] - 12, 0, flAnimInTime + (flOffset * 2), vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
#endif

//1440p
#ifdef _X360UI1440p
	m_pCenterBg->SetPos(m_PanelXPos[0], 86);
	GetAnimationController()->RunAnimationCommand(m_pCenterBg, "xpos", m_PanelXPos[2] - 16, 0, flAnimInTime + (flOffset * 2), vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
#endif

//2160p
#ifdef _X360UI2160p
	m_pCenterBg->SetPos( m_PanelXPos[0], 134 );
	GetAnimationController()->RunAnimationCommand(m_pCenterBg, "xpos", m_PanelXPos[2] - 24, 0, flAnimInTime + (flOffset * 2), vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
#endif


	GetAnimationController()->RunAnimationCommand(m_pCenterBg, "alpha", 255, 0, (flAnimInTime + (flOffset * 2))*2.0f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);

	int panelWidth, panelHeight;
	m_ChapterPanels[1]->GetSize( panelHeight, panelWidth );

	m_pCenterBg->SetTall( panelHeight );
}

void CNewGameDialog::Activate( void )
{
	m_bMapStarting = false;

	if ( GameUI().IsConsoleUI() )
	{
		// Stop blinking the menu item now that we've seen the unlocked stuff
		CBasePanel *pBasePanel = BasePanel();
		if ( pBasePanel )
			pBasePanel->SetMenuItemBlinkingState( "OpenNewGameDialog", false );

		BonusMapsDatabase()->SetBlink( false );
	}

	// Commentary stuff is set up on activate because in XBox the new game menu is never deleted
	SetTitle( ( ( m_bCommentaryMode ) ? ( "#GameUI_LoadCommentary" ) : ( "#GameUI_NewGame") ), true);

	if ( m_pCommentaryLabel )
		m_pCommentaryLabel->SetVisible( m_bCommentaryMode );

	// work out which chapters are unlocked
	ConVarRef var( "sv_unlockedchapters" );
	const char *unlockedChapter = var.IsValid() ? var.GetString() : "1";
	int iUnlockedChapter = atoi(unlockedChapter);

	for ( int i = 0; i < m_ChapterPanels.Count(); i++)
	{
		CGameChapterPanel *pChapterPanel = m_ChapterPanels[ i ];

		if ( pChapterPanel )
		{
			pChapterPanel->SetCommentaryMode( m_bCommentaryMode );

			UpdatePanelLockedStatus( iUnlockedChapter, i + 1, pChapterPanel );
		}
	}

	// Animate the opening animation
	AnimateDialogStart();

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Apply special properties of the menu
//-----------------------------------------------------------------------------
void CNewGameDialog::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	int ypos = inResourceData->GetInt( "chapterypos", 40 );
	for ( int i = 0; i < NUM_SLOTS; ++i )
	{
		m_PanelYPos[i] = ypos;
	}

	m_pCenterBg->SetTall( inResourceData->GetInt( "centerbgtall", 0 ) );

	g_ScrollSpeedSlow = inResourceData->GetFloat( "scrollslow", 0.0f );
	g_ScrollSpeedFast = inResourceData->GetFloat( "scrollfast", 0.0f );
	SetFastScroll( false );
}

void CNewGameDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pFooter )
	{
		KeyValues *pFooterControlSettings = BasePanel()->GetConsoleControlSettings()->FindKey( "NewGameFooter.res" );
		m_pFooter->LoadControlSettings( "null", NULL, pFooterControlSettings );
	}

	UpdateMenuComponents( SCROLL_NONE );

	m_pCommentaryLabel = dynamic_cast<vgui::Label*>( FindChildByName( "CommentaryUnlock" ) );
	if ( m_pCommentaryLabel )
		m_pCommentaryLabel->SetVisible( m_bCommentaryMode );

	if ( GameUI().IsConsoleUI() )
	{
		if ( !m_bCommentaryMode && BonusMapsDatabase()->BonusesUnlocked() && !m_ChapterPanels[ m_PanelIndex[SLOT_CENTER] ]->HasBonus() )
		{
			// Find the first bonus
			ScrollSelectionPanels( SCROLL_LEFT );
			m_bScrollToFirstBonusMap = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the correct properties for visible components
//-----------------------------------------------------------------------------
void CNewGameDialog::UpdateMenuComponents( EScrollDirection dir )
{
	// This is called prior to any scrolling, 
	// so we need to look ahead to the post-scroll state
	int centerIdx = SLOT_CENTER;
	if ( dir == SCROLL_LEFT )
	{
		++centerIdx;
	}
	else if ( dir == SCROLL_RIGHT )
	{
		--centerIdx;
	}
	int leftIdx = centerIdx - 1;
	int rightIdx = centerIdx + 1;

	if ( GameUI().IsConsoleUI() )
	{
		bool bHasBonus = false;
		if ( m_PanelIndex[centerIdx] != INVALID_INDEX )
		{
			wchar_t buffer[ MAX_PATH ];
			m_ChapterPanels[ m_PanelIndex[centerIdx] ]->m_pChapterNameLabel->GetText( buffer, sizeof(buffer) );
			m_pChapterTitleLabels[m_ActiveTitleIdx]->SetText( buffer );

			// If it has bonuses show the scroll up and down arrows
			bHasBonus = m_ChapterPanels[ m_PanelIndex[centerIdx] ]->HasBonus();
		}

		vgui::Panel *leftArrow = this->FindChildByName( "LeftArrow" );
		vgui::Panel *rightArrow = this->FindChildByName( "RightArrow" );
		if ( leftArrow )
		{
			if ( m_PanelIndex[leftIdx] != INVALID_INDEX )
			{
				leftArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
			}
			else
			{
				leftArrow->SetFgColor( Color( 128, 128, 128, 64 ) );
			}
		}
		if ( rightArrow )
		{
			if ( m_PanelIndex[rightIdx] != INVALID_INDEX )
			{
				rightArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
			}
			else
			{
				rightArrow->SetFgColor( Color( 128, 128, 128, 64 ) );
			}
		}

		if ( bHasBonus )
		{
			// Find the bonus description for this panel
			for ( int iBonus = 0; iBonus < BonusMapsDatabase()->BonusCount(); ++iBonus )
			{
				m_pBonusMapDescription = BonusMapsDatabase()->GetBonusData( iBonus );
				if ( Q_stricmp( m_pBonusMapDescription->szChapterName, m_ChapterPanels[ m_PanelIndex[centerIdx] ]->GetConfigFile() ) == 0 )
					break;
			}
		}
		else
		{
			m_pBonusMapDescription = NULL;
		}

		vgui::Panel *upArrow = this->FindChildByName( "UpArrow" );
		vgui::Panel *downArrow = this->FindChildByName( "DownArrow" );

		if ( upArrow )
			upArrow->SetVisible( bHasBonus );
		if ( downArrow )
			downArrow->SetVisible( bHasBonus );

		m_pBonusSelection->SetVisible( bHasBonus );
		m_pBonusSelectionBorder->SetVisible( bHasBonus );

		UpdateBonusSelection();
	}

	// No buttons in the xbox ui
	if ( !GameUI().IsConsoleUI() )
	{
		if ( m_PanelIndex[leftIdx] == INVALID_INDEX || m_PanelIndex[leftIdx] == 0 )
		{
			m_pPrevButton->SetVisible( false );
			m_pPrevButton->SetEnabled( false );
		}
		else
		{
			m_pPrevButton->SetVisible( true );
			m_pPrevButton->SetEnabled( true );
		}

		if ( m_ChapterPanels.Count() < 4 ) // if there are less than 4 chapters show the next button but disabled
		{
			m_pNextButton->SetVisible( true );
			m_pNextButton->SetEnabled( false );
		}
		else if ( m_PanelIndex[rightIdx] == INVALID_INDEX || m_PanelIndex[rightIdx] == m_ChapterPanels.Count()-1 )
		{
			m_pNextButton->SetVisible( false );
			m_pNextButton->SetEnabled( false );
		}
		else
		{
			m_pNextButton->SetVisible( true );
			m_pNextButton->SetEnabled( true );
		}
	}
}

void CNewGameDialog::UpdateBonusSelection( void )
{
	int iNumChallenges = 0;
	if ( m_pBonusMapDescription )
	{
		if ( m_pBonusMapDescription->m_pChallenges )
			iNumChallenges = m_pBonusMapDescription->m_pChallenges->Count();

		// Wrap challenge selection to fit number of possible selections
		if ( m_iBonusSelection < 0 )
			m_iBonusSelection = iNumChallenges + 1;
		else if ( m_iBonusSelection >= iNumChallenges + 2 )
			m_iBonusSelection = 0;
	}
	else
	{
		// No medals to show
		SetControlVisible( "ChallengeEarnedMedal", false );
		SetControlVisible( "ChallengeBestLabel", false );
		SetControlVisible( "ChallengeNextMedal", false );
		SetControlVisible( "ChallengeNextLabel", false );
		return;
	}

	if ( m_iBonusSelection == 0 )
	{
		m_pBonusSelection->SetText( "#GameUI_BonusMapsStandard" );
		SetControlVisible( "ChallengeEarnedMedal", false );
		SetControlVisible( "ChallengeBestLabel", false );
		SetControlVisible( "ChallengeNextMedal", false );
		SetControlVisible( "ChallengeNextLabel", false );
	}
	else if ( m_iBonusSelection == 1 )
	{
		m_pBonusSelection->SetText( "#GameUI_BonusMapsAdvanced" );
		SetControlVisible( "ChallengeEarnedMedal", false );
		SetControlVisible( "ChallengeBestLabel", false );
		SetControlVisible( "ChallengeNextMedal", false );
		SetControlVisible( "ChallengeNextLabel", false );

		char szMapAdvancedName[ 256 ] = "";
		if ( m_pBonusMapDescription )
		{
			Q_snprintf( szMapAdvancedName, sizeof( szMapAdvancedName ), "%s_advanced", m_pBonusMapDescription->szMapFileName );
		}

		BonusMapDescription_t *pAdvancedDescription = NULL;

		// Find the bonus description for this panel
		for ( int iBonus = 0; iBonus < BonusMapsDatabase()->BonusCount(); ++iBonus )
		{
			pAdvancedDescription = BonusMapsDatabase()->GetBonusData( iBonus );
			if ( Q_stricmp( szMapAdvancedName, pAdvancedDescription->szMapFileName ) == 0 )
				break;
		}

		if ( pAdvancedDescription && pAdvancedDescription->bComplete )
		{
			CBitmapImagePanel *pBitmap = dynamic_cast<CBitmapImagePanel*>( FindChildByName( "ChallengeEarnedMedal" ) );
			SetControlVisible( "ChallengeEarnedMedal", true );
			pBitmap->SetVisible( true );
			pBitmap->setTexture( "hud/icon_complete" );
		}
	}
	else
	{
		int iChallenge = m_iBonusSelection - 2;
		ChallengeDescription_t *pChallengeDescription = &((*m_pBonusMapDescription->m_pChallenges)[ iChallenge ]);

		// Set the display text for the selected challenge
		m_pBonusSelection->SetText( pChallengeDescription->szName );

		int iBest, iEarnedMedal, iNext, iNextMedal;
		GetChallengeMedals( pChallengeDescription, iBest, iEarnedMedal, iNext, iNextMedal );

		char szBuff[ 512 ];

		// Set earned medal
		if ( iEarnedMedal > -1 && iBest != -1 )
		{
			if ( iChallenge < 10 )
				Q_snprintf( szBuff, sizeof( szBuff ), "medals/medal_0%i_%s", iChallenge, g_pszMedalNames[ iEarnedMedal ] );
			else
				Q_snprintf( szBuff, sizeof( szBuff ), "medals/medal_%i_%s", iChallenge, g_pszMedalNames[ iEarnedMedal ] );

			CBitmapImagePanel *pBitmap = dynamic_cast<CBitmapImagePanel*>( FindChildByName( "ChallengeEarnedMedal" ) );
			pBitmap->SetVisible( true );
			pBitmap->setTexture( szBuff );
		}
		else
		{
			CBitmapImagePanel *pBitmap = dynamic_cast<CBitmapImagePanel*>( FindChildByName( "ChallengeEarnedMedal" ) );
			pBitmap->SetVisible( false );
		}

		// Set next medal
		if ( iNextMedal > 0 )
		{
			if ( iChallenge < 10 )
				Q_snprintf( szBuff, sizeof( szBuff ), "medals/medal_0%i_%s", iChallenge, g_pszMedalNames[ iNextMedal ] );
			else
				Q_snprintf( szBuff, sizeof( szBuff ), "medals/medal_%i_%s", iChallenge, g_pszMedalNames[ iNextMedal ] );

			CBitmapImagePanel *pBitmap = dynamic_cast<CBitmapImagePanel*>( FindChildByName( "ChallengeNextMedal" ) );
			pBitmap->SetVisible( true );
			pBitmap->setTexture( szBuff );
		}
		else
		{
			SetControlVisible( "ChallengeNextMedal", false );
		}

		wchar_t szWideBuff[ 64 ];
		wchar_t szWideBuff2[ 64 ];

		// Best label
		if ( iBest != -1 )
		{
			Q_snprintf( szBuff, sizeof( szBuff ), "%i", iBest );
			g_pVGuiLocalize->ConvertANSIToUnicode( szBuff, szWideBuff2, sizeof( szWideBuff2 ) );
			g_pVGuiLocalize->ConstructString( szWideBuff, sizeof( szWideBuff ), g_pVGuiLocalize->Find( "#GameUI_BonusMapsBest" ), 1, szWideBuff2 );
			g_pVGuiLocalize->ConvertUnicodeToANSI( szWideBuff, szBuff, sizeof( szBuff ) );

			SetControlString( "ChallengeBestLabel", szBuff );
			SetControlVisible( "ChallengeBestLabel", true );
		}
		else
		{
			SetControlVisible( "ChallengeBestLabel", false );
		}

		// Next label
		if ( iNext != -1 )
		{
			Q_snprintf( szBuff, sizeof( szBuff ), "%i", iNext );
			g_pVGuiLocalize->ConvertANSIToUnicode( szBuff, szWideBuff2, sizeof( szWideBuff2 ) );
			g_pVGuiLocalize->ConstructString( szWideBuff, sizeof( szWideBuff ), g_pVGuiLocalize->Find( "#GameUI_BonusMapsGoal" ), 1, szWideBuff2 );
			g_pVGuiLocalize->ConvertUnicodeToANSI( szWideBuff, szBuff, sizeof( szBuff ) );

			SetControlString( "ChallengeNextLabel", szBuff );
			SetControlVisible( "ChallengeNextLabel", true );
		}
		else
		{
			SetControlVisible( "ChallengeNextLabel", false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets a chapter as selected
//-----------------------------------------------------------------------------
void CNewGameDialog::SetSelectedChapterIndex( int index )
{
	m_iSelectedChapter = index;

	for (int i = 0; i < m_ChapterPanels.Count(); i++)
	{
		if ( i == index )
		{
			m_ChapterPanels[i]->SetSelected( true );
		}
		else
		{
			m_ChapterPanels[i]->SetSelected( false );
		}
	}

	if ( m_pPlayButton )
	{
		m_pPlayButton->SetEnabled( true );
	}

	// Setup panels to the left of the selected panel
	int selectedSlot = GameUI().IsConsoleUI() ? SLOT_CENTER : index % 3 + 1;
	int currIdx = index;
	for ( int i = selectedSlot; i >= 0 && currIdx >= 0; --i )
	{
		m_PanelIndex[i] = currIdx;
		--currIdx;
		InitPanelIndexForDisplay( i );
	}

	// Setup panels to the right of the selected panel
	currIdx = index + 1;
	for ( int i = selectedSlot + 1; i < NUM_SLOTS && currIdx < m_ChapterPanels.Count(); ++i )
	{
		m_PanelIndex[i] = currIdx;
		++currIdx;
		InitPanelIndexForDisplay( i );
	}

	UpdateMenuComponents( SCROLL_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: sets a chapter as selected
//-----------------------------------------------------------------------------
void CNewGameDialog::SetSelectedChapter( const char *chapter )
{
	Assert( chapter );
	for (int i = 0; i < m_ChapterPanels.Count(); i++)
	{
		if ( chapter && !Q_stricmp(m_ChapterPanels[i]->GetChapter(), chapter) )
		{
			m_iSelectedChapter = i;
			m_ChapterPanels[m_iSelectedChapter]->SetSelected( true );
		}
		else
		{
			m_ChapterPanels[i]->SetSelected( false );
		}
	}

	if ( m_pPlayButton )
	{
		m_pPlayButton->SetEnabled( true );
	}
}


//-----------------------------------------------------------------------------
// iUnlockedChapter - the value of sv_unlockedchapters, 1-based. A value of 0
//		is treated as a 1, since at least one chapter must be unlocked.
//
// i - the 1-based index of the chapter we're considering.
//-----------------------------------------------------------------------------
void CNewGameDialog::UpdatePanelLockedStatus( int iUnlockedChapter, int i, CGameChapterPanel *pChapterPanel )
{
	if ( iUnlockedChapter <= 0 )
	{
		iUnlockedChapter = 1;
	}

	// Commentary mode requires chapters to be finished before they can be chosen
	bool bLocked = false;

	if ( m_bCommentaryMode )
	{
		bLocked = ( iUnlockedChapter <= i );
	}
	else
	{
		if ( iUnlockedChapter < i )
		{
			// Never lock the first chapter
			bLocked = ( i != 0 );
		}
	}

	pChapterPanel->SetEnabled( !bLocked );
}

//-----------------------------------------------------------------------------
// Purpose: Called before a panel scroll starts.
//-----------------------------------------------------------------------------
void CNewGameDialog::PreScroll( EScrollDirection dir )
{
	int hideIdx = INVALID_INDEX;
	if ( dir == SCROLL_LEFT )
	{
		hideIdx = m_PanelIndex[SLOT_LEFT];
	}
	else if ( dir == SCROLL_RIGHT )
	{
		hideIdx = m_PanelIndex[SLOT_RIGHT];
	}
	if ( hideIdx != INVALID_INDEX )
	{
		// Push back the panel that's about to be hidden
		// so the next panel scrolls over the top of it.
		m_ChapterPanels[hideIdx]->SetZPos( 0 );
	}

	// Flip the active title label prior to the crossfade
	m_ActiveTitleIdx ^= 0x01;
}

//-----------------------------------------------------------------------------
// Purpose: Called after a panel scroll finishes.
//-----------------------------------------------------------------------------
void CNewGameDialog::PostScroll( EScrollDirection dir )
{
	int index = INVALID_INDEX;
	if ( dir == SCROLL_LEFT )
	{
		index = m_PanelIndex[SLOT_RIGHT];
	}
	else if ( dir == SCROLL_RIGHT )
	{
		index = m_PanelIndex[SLOT_LEFT];
	}

	// Fade in the revealed panel
	if ( index != INVALID_INDEX )
	{
		CGameChapterPanel *panel = m_ChapterPanels[index];
		panel->SetZPos( 50 );
		GetAnimationController()->RunAnimationCommand( panel, "alpha", 64, 0, m_ScrollSpeed, vgui::AnimationController::INTERPOLATOR_LINEAR );
	}

	if ( GameUI().IsConsoleUI() )
	{
		if ( BonusMapsDatabase()->BonusesUnlocked() && m_bScrollToFirstBonusMap )
		{
			if ( !m_ChapterPanels[ m_PanelIndex[SLOT_CENTER] ]->HasBonus() )
			{
				// Find the first bonus
				ScrollSelectionPanels( SCROLL_LEFT );
			}
			else
			{
				// Found a bonus, stop scrolling
				m_bScrollToFirstBonusMap = false;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initiates a panel scroll and starts the animation.
//-----------------------------------------------------------------------------
void CNewGameDialog::ScrollSelectionPanels( EScrollDirection dir )
{
	// Only initiate a scroll if panels aren't currently scrolling
	if ( !m_bScrolling )
	{
		// Handle any pre-scroll setup
		PreScroll( dir );

		if ( dir == SCROLL_LEFT)
		{
			m_ScrollCt += SCROLL_LEFT;
		}
		else if ( dir == SCROLL_RIGHT && m_PanelIndex[SLOT_CENTER] != 0 )
		{
			m_ScrollCt += SCROLL_RIGHT;
		}

		m_bScrolling = true;
		AnimateSelectionPanels();

		// Update the arrow colors, help text, and buttons. Doing it here looks better than having
		// the components change after the entire scroll animation has finished.
		UpdateMenuComponents( m_ScrollDirection );
	}
}

void CNewGameDialog::ScrollBonusSelection( EScrollDirection dir )
{
	// Don't scroll if there's no bonuses for this panel
	if ( !m_pBonusMapDescription )
		return;

	m_iBonusSelection += dir;

	vgui::surface()->PlaySound( "UI/buttonclick.wav" );

	UpdateBonusSelection();
}

//-----------------------------------------------------------------------------
// Purpose: Initiates the scripted scroll and fade effects of all five slotted panels 
//-----------------------------------------------------------------------------
void CNewGameDialog::AnimateSelectionPanels( void )
{
	int idxOffset = 0;
	int startIdx = SLOT_LEFT;
	int endIdx = SLOT_RIGHT;

	// Don't scroll outside the bounds of the panel list
	if ( m_ScrollCt >= SCROLL_LEFT && (m_PanelIndex[SLOT_CENTER] < m_ChapterPanels.Count() - 1 || !GameUI().IsConsoleUI()) )
	{
		idxOffset = -1;
		endIdx = SLOT_OFFRIGHT;
		m_ScrollDirection = SCROLL_LEFT;
	}
	else if ( m_ScrollCt <= SCROLL_RIGHT && (m_PanelIndex[SLOT_CENTER] > 0 || !GameUI().IsConsoleUI()) )
	{
		idxOffset = 1;
		startIdx = SLOT_OFFLEFT;
		m_ScrollDirection = SCROLL_RIGHT;
	}

	if ( 0 == idxOffset )
	{
		// Kill the scroll, it's outside the bounds
		m_ScrollCt = 0;
		m_bScrolling = false;
		m_ScrollDirection = SCROLL_NONE;
		vgui::surface()->PlaySound( "player/suit_denydevice.wav" );
		return;
	}

	// Should never happen
	if ( startIdx > endIdx )
		return;

	for ( int i = startIdx; i <= endIdx; ++i )
	{
		if ( m_PanelIndex[i] != INVALID_INDEX )
		{
			int nextIdx = i + idxOffset;
			CGameChapterPanel *panel = m_ChapterPanels[ m_PanelIndex[i] ];
			GetAnimationController()->RunAnimationCommand( panel, "xpos",  m_PanelXPos[nextIdx],  0, m_ScrollSpeed, vgui::AnimationController::INTERPOLATOR_LINEAR );
			GetAnimationController()->RunAnimationCommand( panel, "ypos",  m_PanelYPos[nextIdx],  0, m_ScrollSpeed, vgui::AnimationController::INTERPOLATOR_LINEAR );
			GetAnimationController()->RunAnimationCommand( panel, "alpha", m_PanelAlpha[nextIdx], 0, m_ScrollSpeed, vgui::AnimationController::INTERPOLATOR_LINEAR );
		}
	}

	if ( GameUI().IsConsoleUI() )
	{
		vgui::surface()->PlaySound( "UI/buttonclick.wav" );

		// Animate the center background panel
		GetAnimationController()->RunAnimationCommand( m_pCenterBg, "alpha", 0, 0, m_ScrollSpeed * 0.25f, vgui::AnimationController::INTERPOLATOR_LINEAR );
		
		// Crossfade the chapter title labels
		int inactiveTitleIdx = m_ActiveTitleIdx ^ 0x01;
		GetAnimationController()->RunAnimationCommand( m_pChapterTitleLabels[m_ActiveTitleIdx], "alpha", 255, 0, m_ScrollSpeed, vgui::AnimationController::INTERPOLATOR_LINEAR );
		GetAnimationController()->RunAnimationCommand( m_pChapterTitleLabels[inactiveTitleIdx], "alpha", 0, 0, m_ScrollSpeed, vgui::AnimationController::INTERPOLATOR_LINEAR );
		
		// Scrolling up through chapters, offset is negative
		m_iSelectedChapter -= idxOffset;
	}

	PostMessage( this, new KeyValues( "FinishScroll" ), m_ScrollSpeed );
}

//-----------------------------------------------------------------------------
// Purpose: After a scroll, each panel slot holds the index of a panel that has 
//			scrolled to an adjacent slot. This function updates each slot so
//			it holds the index of the panel that is actually in that slot's position.
//-----------------------------------------------------------------------------
void CNewGameDialog::ShiftPanelIndices( int offset )
{
	// Shift all the elements over one slot, then calculate what the last slot's index should be.
	int lastSlot = NUM_SLOTS - 1;
	if ( offset > 0 )
	{
		// Hide the panel that's dropping out of the slots
		if ( IsValidPanel( m_PanelIndex[0] ) )
		{
			m_ChapterPanels[ m_PanelIndex[0] ]->SetVisible( false );
		}

		// Scrolled panels to the right, so shift the indices one slot to the left
		Q_memmove( &m_PanelIndex[0], &m_PanelIndex[1], lastSlot * sizeof( m_PanelIndex[0] ) );
		if ( m_PanelIndex[lastSlot] != INVALID_INDEX )
		{
			int num = m_PanelIndex[ lastSlot ] + 1;
			if ( IsValidPanel( num ) )
			{
				m_PanelIndex[lastSlot] = num;
				InitPanelIndexForDisplay( lastSlot );
			}
			else
			{
				m_PanelIndex[lastSlot] = INVALID_INDEX;
			}
		}
	}
	else
	{
		// Hide the panel that's dropping out of the slots
		if ( IsValidPanel( m_PanelIndex[lastSlot] ) )
		{
			m_ChapterPanels[ m_PanelIndex[lastSlot] ]->SetVisible( false );
		}

		// Scrolled panels to the left, so shift the indices one slot to the right
		Q_memmove( &m_PanelIndex[1], &m_PanelIndex[0], lastSlot * sizeof( m_PanelIndex[0] ) );
		if ( m_PanelIndex[0] != INVALID_INDEX )
		{
			int num = m_PanelIndex[0] - 1;
			if ( IsValidPanel( num ) )
			{
				m_PanelIndex[0] = num;
				InitPanelIndexForDisplay( 0 );
			}
			else
			{
				m_PanelIndex[0] = INVALID_INDEX;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Validates an index into the selection panels vector
//-----------------------------------------------------------------------------
bool CNewGameDialog::IsValidPanel( const int idx )
{
	if ( idx < 0 || idx >= m_ChapterPanels.Count() )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sets up a panel's properties before it is displayed
//-----------------------------------------------------------------------------
void CNewGameDialog::InitPanelIndexForDisplay( const int idx )
{
	CGameChapterPanel *panel = m_ChapterPanels[ m_PanelIndex[idx] ];
	if ( panel )
	{
		panel->SetPos( m_PanelXPos[idx], m_PanelYPos[idx] );
		panel->SetAlpha( m_PanelAlpha[idx] );
		panel->SetVisible( true );
		if ( m_PanelAlpha[idx] )
		{
			panel->SetZPos( 50 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets which scroll speed should be used
//-----------------------------------------------------------------------------
void CNewGameDialog::SetFastScroll( bool fast )
{
	m_ScrollSpeed = fast ? g_ScrollSpeedFast : g_ScrollSpeedSlow;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if a button is being held down, and speeds up the scroll 
//-----------------------------------------------------------------------------
void CNewGameDialog::ContinueScrolling( void )
{
	if ( !GameUI().IsConsoleUI() )
	{
		if ( m_PanelIndex[SLOT_CENTER-1] % 3 )
		{
	//		m_ButtonPressed = m_ScrollDirection;
			ScrollSelectionPanels( m_ScrollDirection );
		}
		return;
	}

	if ( m_ButtonPressed == m_ScrollDirection )
	{
		SetFastScroll( true );
		ScrollSelectionPanels( m_ScrollDirection );
	}
	else if ( m_ButtonPressed != SCROLL_NONE )
	{
		// The other direction has been pressed - start a slow scroll
		SetFastScroll( false );
		ScrollSelectionPanels( (EScrollDirection)m_ButtonPressed );
	}
	else
	{
		SetFastScroll( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a scroll distance of one slot has been completed
//-----------------------------------------------------------------------------
void CNewGameDialog::FinishScroll( void )
{
	// Fade the center bg panel back in
	GetAnimationController()->RunAnimationCommand( m_pCenterBg, "alpha", 255, 0, m_ScrollSpeed * 0.25f, vgui::AnimationController::INTERPOLATOR_LINEAR );

	ShiftPanelIndices( m_ScrollDirection );
	m_bScrolling = false;
	m_ScrollCt = 0;
	
	// End of scroll step
	PostScroll( m_ScrollDirection );

	// Continue scrolling if necessary
	ContinueScrolling();
}

//-----------------------------------------------------------------------------
// Purpose: starts the game at the specified skill level
//-----------------------------------------------------------------------------
void CNewGameDialog::StartGame( void )
{
	if ( m_ChapterPanels.IsValidIndex( m_iSelectedChapter ) )
	{
		char mapcommand[512];
		mapcommand[0] = 0;
		Q_snprintf( mapcommand, sizeof( mapcommand ), "disconnect\ndeathmatch 0\nprogress_enable\nexec %s\n", m_ChapterPanels[m_iSelectedChapter]->GetConfigFile() );

		// Set commentary
		ConVarRef commentary( "commentary" );
		commentary.SetValue( m_bCommentaryMode );

		ConVarRef sv_cheats( "sv_cheats" );
		sv_cheats.SetValue( m_bCommentaryMode );

		BasePanel()->FadeToBlackAndRunEngineCommand( mapcommand );

		if ( m_ChapterPanels[m_iSelectedChapter]->HasBonus() && m_iBonusSelection > 0 )
		{
			if ( m_iBonusSelection == 1 )
			{
				// Run the advanced chamber instead of the config file
				char *pLastSpace = Q_strrchr( mapcommand, '\n' );
				pLastSpace[ 0 ] = '\0';
				pLastSpace = Q_strrchr( mapcommand, '\n' );

				Q_snprintf( pLastSpace, sizeof( mapcommand ) - Q_strlen( mapcommand ), "\nmap %s_advanced\n", m_pBonusMapDescription->szMapFileName );
			}
			else
			{
				char sz[ 256 ];

				int iChallenge = m_iBonusSelection - 1;

				// Set up the challenge mode
				Q_snprintf( sz, sizeof( sz ), "sv_bonus_challenge %i\n", iChallenge );
				engine->ClientCmd_Unrestricted( sz );

				ChallengeDescription_t *pChallengeDescription = &((*m_pBonusMapDescription->m_pChallenges)[ iChallenge - 1 ]);

				// Set up medal goals
				BonusMapsDatabase()->SetCurrentChallengeObjectives( pChallengeDescription->iBronze, pChallengeDescription->iSilver, pChallengeDescription->iGold );
				BonusMapsDatabase()->SetCurrentChallengeNames( m_pBonusMapDescription->szFileName, m_pBonusMapDescription->szMapName, pChallengeDescription->szName );
			}
		}

		m_bMapStarting = true;
		BasePanel()->FadeToBlackAndRunEngineCommand( mapcommand );

		OnClose();
	}
}

void CNewGameDialog::OnClose( void )
{
	m_KeyRepeat.Reset();

	if ( GameUI().IsConsoleUI() && !m_bMapStarting )
	{
		BasePanel()->RunCloseAnimation( "CloseNewGameDialog_OpenMainMenu" );			
		BonusMapsDatabase()->WriteSaveData();	// Closing this dialog is a good time to save
	}
	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: handles button commands
//-----------------------------------------------------------------------------
void CNewGameDialog::OnCommand( const char *command )
{
	bool bReset = true;

	if ( !stricmp( command, "Play" ) )
	{
		if ( m_bMapStarting )
			return;

		if ( GameUI().IsConsoleUI() )
		{
			if ( m_ChapterPanels[m_iSelectedChapter]->IsEnabled() )
			{
				if ( !GameUI().HasSavedThisMenuSession() && GameUI().IsInLevel() && engine->GetMaxClients() == 1 )
				{
					vgui::surface()->PlaySound( "UI/buttonclickrelease.wav" );
					BasePanel()->ShowMessageDialog( MD_SAVE_BEFORE_NEW_GAME, this );
				}
				else
				{
					OnCommand( "StartNewGame" );
				}
			}
			else
			{
				// This chapter isn't unlocked!
				m_bMapStarting = false;
				vgui::surface()->PlaySound( "player/suit_denydevice.wav" );

				if ( m_bCommentaryMode )
				{
					BasePanel()->ShowMessageDialog( MD_COMMENTARY_CHAPTER_UNLOCK_EXPLANATION, this );
				}
			}
		}
		else
		{
			StartGame();
		}
	}
	else if ( !stricmp( command, "StartNewGame" ) )
	{
		ConVarRef commentary( "commentary" );

		if ( m_bCommentaryMode && !commentary.GetBool() )
		{
			// Using the commentary menu, but not already in commentary mode, explain the rules
			PostMessage( (vgui::Panel*)this, new KeyValues( "command", "command", "StartNewGameWithCommentaryExplanation" ), 0.2f );
		}
		else
		{
			// Don't allow other inputs
			m_bMapStarting = true;

			// Remind them how autosaves work
			PostMessage( (vgui::Panel*)this, new KeyValues( "command", "command", "StartNewGameWithAutosaveExplanation" ), 0.2f );
		}
	}
	else if ( !stricmp( command, "StartNewGameWithAutosaveExplanation" ) )
	{
		BasePanel()->ShowMessageDialog( MD_AUTOSAVE_EXPLANATION, this );
	}
	else if ( !stricmp( command, "StartNewGameWithCommentaryExplanation" ) )
	{
		if ( ModInfo().IsSinglePlayerOnly() )
		{
			// Don't allow other inputs
			m_bMapStarting = true;
			BasePanel()->ShowMessageDialog( MD_COMMENTARY_EXPLANATION, this );
		}
		else
		{
			// Don't allow other inputs
			m_bMapStarting = true;
			BasePanel()->ShowMessageDialog( MD_COMMENTARY_EXPLANATION_MULTI, this );
		}
	}
	else if ( !stricmp( command, "StartNewGameNoCommentaryExplanation" ) )
	{
		vgui::surface()->PlaySound( "UI/buttonclickrelease.wav" );
		BasePanel()->RunAnimationWithCallback( this, "CloseNewGameDialog", new KeyValues( "StartGame" ) );
	}
	else if ( !stricmp( command, "Next" ) )
	{
		if ( m_bMapStarting )
			return;

		ScrollSelectionPanels( SCROLL_LEFT );
		bReset = false;
	}
	else if ( !stricmp( command, "Prev" ) )
	{
		if ( m_bMapStarting )
			return;

		ScrollSelectionPanels( SCROLL_RIGHT );
		bReset = false;
	}
	else if ( !stricmp( command, "Mode_Next" ) )
	{
		if ( m_bMapStarting )
			return;

		ScrollBonusSelection( SCROLL_LEFT );
		bReset = false;
	}
	else if ( !stricmp( command, "Mode_Prev" ) )
	{
		if ( m_bMapStarting )
			return;

		ScrollBonusSelection( SCROLL_RIGHT );
		bReset = false;
	}
	else if ( !Q_stricmp( command, "ReleaseModalWindow" ) )
	{
		vgui::surface()->RestrictPaintToSinglePanel(NULL);
	}
	else
	{
		BaseClass::OnCommand( command );
	}

	if ( bReset )
	{
		m_KeyRepeat.Reset();
	}
}

void CNewGameDialog::PaintBackground()
{
	if ( !GameUI().IsConsoleUI() )
	{
		BaseClass::PaintBackground();
		return;
	}

	int wide, tall;
	GetSize( wide, tall );

	Color col = GetBgColor();
	DrawBox( 0, 0, wide, tall, col, 1.0f );

	int y = 0;
	if ( m_pChapterTitleLabels[0] )
	{
		// offset by title
		int titleX, titleY, titleWide, titleTall;
		m_pChapterTitleLabels[0]->GetBounds( titleX, titleY, titleWide, titleTall );	
		y += titleY + titleTall;
	}
	else
	{
		//STAVAAS Res

	//480p/720p
#if defined(_X360UI480p) || defined(_X360UI720p)
		y = 8;
#endif

	//1080p
#ifdef _X360UI1080p
		y = 12;
#endif

	//1440p
#ifdef _X360UI1440p
		y = 16;
#endif

	//2160p
#ifdef _X360UI2160p
		y = 24;
#endif

	}

	// draw an inset
	Color darkColor;
	darkColor.SetColor( 0.70f * (float)col.r(), 0.70f * (float)col.g(), 0.70f * (float)col.b(), col.a() );
	vgui::surface()->DrawSetColor( darkColor );

	//STAVAAS Res

//480p/720p
#if defined(_X360UI480p) || defined(_X360UI720p)
	vgui::surface()->DrawFilledRect( 8, 35, wide - 8, tall - 8 );
#endif

//1080p
#ifdef _X360UI1080p
	vgui::surface()->DrawFilledRect( 12, 54, wide - 12, tall - 12 );
#endif

//1440p
#ifdef _X360UI1440p
	vgui::surface()->DrawFilledRect( 16, 70, wide - 16, tall - 16 );
#endif

//2160p
#ifdef _X360UI2160p
	vgui::surface()->DrawFilledRect( 24, 108, wide - 24, tall - 24 );
#endif

}

void CNewGameDialog::OnKeyCodePressed( KeyCode code )
{
	// Inhibit key activity during transitions
	if (GetAlpha() != 255 )
		return;

	m_KeyRepeat.KeyDown( code );

	switch (code)
	{
	case KEY_XBUTTON_A:
	case KEY_ENTER:
		OnCommand("Play");
		break;

	case KEY_XBUTTON_B:
	case KEY_ESCAPE:
	case KEY_BACKSPACE:
		vgui::surface()->PlaySound("common/wpn_select.wav");
		OnClose();
		break;

		// Move the selection left and right
	case KEY_XSTICK1_LEFT:
	case KEY_XBUTTON_LEFT:
	case KEY_LEFT:
		ScrollSelectionPanels(SCROLL_RIGHT);
		break;

	case KEY_XSTICK1_RIGHT:
	case KEY_XBUTTON_RIGHT:
	case KEY_RIGHT:
		ScrollSelectionPanels(SCROLL_LEFT);
		break;

		// Move the selection up and down
	case KEY_XSTICK1_UP:
	case KEY_XBUTTON_UP:
	case KEY_UP:
		OnCommand("Mode_Prev");
		break;

	case KEY_XSTICK1_DOWN:
	case KEY_XBUTTON_DOWN:
	case KEY_DOWN:
		OnCommand("Mode_Next");
		break;
		
	default:
		break;
	}
}

void CNewGameDialog::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}

void CNewGameDialog::OnThink()
{
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodeTyped( code );
	}

	BaseClass::OnThink();
}
