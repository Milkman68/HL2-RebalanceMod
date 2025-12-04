#ifndef hl2r_campaign_panel
#define hl2r_campaign_panel
#ifdef _WIN32
#pragma once
#endif

#include <hl2r\hl2r_campaign_database.h>
#include "hl2r_game_manager.h"

#include <vgui_controls/HTML.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>

#define HL2R_PANEL_WIDTH 896
#define HL2R_PANEL_HEIGHT 640

#define EDITPANEL_WIDTH 400
#define EDITPANEL_HEIGHT 220

class CSelectedCampaignPanel;
class CCampaignBrowserPanel;
class CCampaignEditPanel;

class SelectableColumnHeader;

#define RES_CAMPAIGN_LIST_PANEL		"resource/ui/hl2r_campaign_list.res"
#define RES_CAMPAIGN_BROWSER_PANEL	"resource/ui/hl2r_campaign_browser.res"
#define RES_CAMPAIGN_EDIT_PANEL		"resource/ui/hl2r_campaign_editor.res"

//------------------------------------------------------------------------------
// Sub panel
//------------------------------------------------------------------------------
class CCampaignListPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CCampaignListPanel, EditablePanel);

public:
	CCampaignListPanel(Panel* parent );
	~CCampaignListPanel() {}

	MESSAGE_FUNC_INT( ItemSelected, "ItemSelected", itemID );
	MESSAGE_FUNC_INT( ItemDoubleLeftClick, "ItemDoubleLeftClick", itemID );

	MESSAGE_FUNC( ColumnSelected, "ColumnSelected");

	virtual void OnCommand(const char* pcCommand); 
	void RefreshList( bool bPreserveSelected = true );

	virtual void OnMouseDoublePressed(MouseCode code) {};

private:
	void CreateListColumns( void );
	void CreateList( void );

	void CreateMountDisclaimer( const char *szCampaignID );

	void HandleCampaignMount( const char *szCampaignID );
	void HandleCampaignScan( void );

private:
	SectionedListPanel		*m_ListPanel;
	SelectableColumnHeader	*m_ListPanelSortHeader;

	CSelectedCampaignPanel	*m_SelectedCampaignPanel;

	// Buttons:
	Button	*m_MountButton;
	Button	*m_CampaignScanButton;


	ESortType m_PrevSortType;
	ESortDirection m_PrevSortDir;
};




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSelectedCampaignPanel : public PropertySheet
{
	DECLARE_CLASS_SIMPLE( CSelectedCampaignPanel, PropertySheet );

public:
	CSelectedCampaignPanel(CCampaignListPanel *parent, const char *name);
	void SetSelected( CAMPAIGN_HANDLE campaign );

private:
	CCampaignBrowserPanel	*m_BrowserPanel;
	CCampaignEditPanel		*m_EditPanel;

	float m_flPageTransitionEffectTime;
};




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCampaignBrowserPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCampaignBrowserPanel, EditablePanel );

public:
	CCampaignBrowserPanel(Panel *parent, const char *name);

	void SetSelected( CAMPAIGN_HANDLE campaign ) ;

	virtual void ApplySchemeSettings(IScheme* pScheme);
	MESSAGE_FUNC( OnFinishRequest, "OnFinishRequest" );
//	MESSAGE_FUNC_CHARPTR( OnURLChanged, "OnURLChanged", url );

private:

	HTML		*m_CampaignWindow;
	ImagePanel	*m_CampaignWindowBackground;
	Divider		*m_CampaignWindowDivider;
};
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCampaignBrowserPanel::CCampaignBrowserPanel(Panel *parent, const char *name) : EditablePanel(parent, name) 
{
	m_CampaignWindowBackground = new ImagePanel(this, "CampaignWindowBackground");
	m_CampaignWindowDivider = new Divider(this, "CampaignWindowDivider");

	m_CampaignWindow = new HTML(this, "CampaignWindow");
	m_CampaignWindow->DisableBrowserClicks(true);
	m_CampaignWindow->SetContextMenuEnabled(false);

	LoadControlSettings(RES_CAMPAIGN_BROWSER_PANEL);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define BROWSER_MAX_URL 256
void CCampaignBrowserPanel::SetSelected(CAMPAIGN_HANDLE campaign)
{
	CCampaignDatabase* database = GetCampaignDatabase();
	if ( database->ValidCampaign(campaign) )
	{
		// Get the ID of the selected campaign and parse it into a url that can be opened.
		char szURL[BROWSER_MAX_URL];
		Q_snprintf(szURL, sizeof(szURL), "https://steamcommunity.com/sharedfiles/filedetails?id=%s", database->GetCampaign(campaign)->id);

		m_CampaignWindow->OpenURL(szURL, "");
	}

	m_CampaignWindow->SetAlpha(0);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignBrowserPanel::ApplySchemeSettings(IScheme* pScheme)
{
	m_CampaignWindowBackground->SetFillColor(GetSchemeColor("AchievementsLightGrey", pScheme));
	BaseClass::ApplySchemeSettings(pScheme);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignBrowserPanel::OnFinishRequest(void)
{
	GetAnimationController()->RunAnimationCommand(m_CampaignWindow, "Alpha", 255.0f, 0.15, 0.15, AnimationController::INTERPOLATOR_LINEAR);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*void CCampaignBrowserPanel::OnURLChanged(const char *url)
{
	if ( !m_pCampaign )
		return;

	if ( !Q_stristr(url, m_pCampaign->id) )
		SetSelected(m_pCampaign);
}
*/


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
enum EPageState
{
	NO_CAMPAIGN_SELECTED,
	UNMOUNTED_CAMPAIGN,
	MOUNTED_CAMPAIGN
};

class CCampaignEditPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCampaignEditPanel, EditablePanel );

public:
	CCampaignEditPanel(CCampaignListPanel *parent, const char *name);

	void SetCampaign( CAMPAIGN_HANDLE campaign );
	MESSAGE_FUNC_INT( ItemSelected, "ItemSelected", itemID );
	MESSAGE_FUNC_INT( ItemDoubleLeftClick, "ItemDoubleLeftClick", itemID ){ SetStartingMap(itemID); }


	void OnCommand(const char* pcCommand);
	void ApplySchemeSettings(IScheme *pScheme);

	MESSAGE_FUNC( OnTextChanged, "TextChanged" ) { CheckApplyButton(); }

private:
	void SetPageState( EPageState type);

	void CreateMapList(void);
	void RefreshMapList(void);

	void SetStartingMap(int selecteditemID);

	void ResetPage(void);
	void CheckApplyButton( void );
	bool ApplyChanges( void );

private:
	CCampaignListPanel	*m_Parent;
	CAMPAIGN_HANDLE		m_Campaign;

	Label				*m_CampaignIDLabel;
	Label				*m_CampaignInfoLabel;

	Label				*m_pNameEntryLabel;
	Label				*m_pGameSelectBoxLabel;
	
	TextEntry			*m_pNameEntry;
	ComboBox			*m_pGameSelectBox;

	Button				*m_ApplyButton;

	Button				*m_StartingMapButton;
	Button				*m_BackgroundMapButton;

	ImagePanel			*m_EditBoxBackground;
	Divider				*m_EditBoxDivider;

	SectionedListPanel	*m_MapListPanel;
	Label				*m_MapListPanelTitle;
	Label				*m_MapListPanelTitleMapname;

	Color	m_TextDisabledColor;
	Color	m_TextEnabledColor;

	char		m_pPrevName[CAMPAIGN_NAME_LENGTH];
	int			m_iPrevGame;

	int			m_iPrevStartMap;
	char		m_iPrevBgMap;
};
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCampaignEditPanel::CCampaignEditPanel(CCampaignListPanel *parent, const char *name) : EditablePanel(parent, name) 
{
	m_Parent = parent;

	m_EditBoxBackground = new ImagePanel(this, "EditBoxBackground");
	m_EditBoxDivider = new Divider(this, "EditBoxDivider");

	m_MapListPanel = new SectionedListPanel(this, "listpanel_maplist");
	m_MapListPanel->AddSection(0, "column0");
	m_MapListPanel->AddColumnToSection(0, "map", "#hl2r_maps_label", SectionedListPanel::COLUMN_BRIGHT, 384);

	m_MapListPanelTitle = new Label(this, "MapListPanelTitle", "");
	m_MapListPanelTitleMapname = new Label(this, "MapListPanelTitleMapname", "");

	m_CampaignIDLabel = new Label(this, "CampaignIDLabel", "");
	m_CampaignInfoLabel = new Label(this, "CampaignInfoLabel", "");

	m_ApplyButton = new Button(this, "ApplyButton", "");
	m_ApplyButton->SetCommand("applychanges");

	m_StartingMapButton = new Button(this, "StartingMapButton", "");
	m_StartingMapButton->SetCommand("setstartmap");

	m_BackgroundMapButton = new Button(this, "BackgroundMapButton", "");
	m_BackgroundMapButton->SetCommand("setbgmap");

	m_pNameEntry = new TextEntry(this, "NameEntry");
	m_pNameEntryLabel = new Label(this, "NameEntryLabel", "");

	m_pGameSelectBox = new ComboBox(this, "GameSelectBox", 3, false);
	m_pGameSelectBox->AddItem("Half-Life 2", NULL);
	m_pGameSelectBox->AddItem("Episode 1", NULL);
	m_pGameSelectBox->AddItem("Episode 2 [Recomended]", NULL);
	m_pGameSelectBoxLabel = new Label(this, "GameSelectBoxLabel", "");

	LoadControlSettings(RES_CAMPAIGN_EDIT_PANEL);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::SetCampaign(CAMPAIGN_HANDLE campaign)
{
	m_Campaign = campaign;
	ResetPage();

	CCampaignDatabase* db = GetCampaignDatabase();
	if ( db->ValidCampaign(campaign) )
	{
		CreateMapList();
		if (db->GetCampaign(campaign)->mounted)
		{
			SetPageState(MOUNTED_CAMPAIGN);
		}
		else
		{
			SetPageState(UNMOUNTED_CAMPAIGN);
		}

		if (Q_stricmp(db->GetCampaign(campaign)->name, "undefined"))
			m_pNameEntry->SetText(db->GetCampaign(campaign)->name);

		if (db->GetCampaign(campaign)->game != -1)
			m_pGameSelectBox->ActivateItem(db->GetCampaign(campaign)->game);

		if (db->GetCampaign(campaign)->startingmap != -1)
			m_iPrevStartMap = db->GetCampaign(campaign)->startingmap;

		m_pNameEntry->GetText(m_pPrevName, 512);
		m_iPrevGame = m_pGameSelectBox->GetActiveItem();
	}
	else
	{
		SetPageState(NO_CAMPAIGN_SELECTED);
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::OnCommand(const char* pcCommand)
{
	if ( !stricmp(pcCommand, "applychanges") )
	{
		ApplyChanges();

		ResetPage();
		m_Parent->RefreshList();
	}
	if ( !stricmp(pcCommand, "applychangesmounted") )
	{
		// Create a disclaimer box confirming if this is our intended action.
		QueryBox *box = new QueryBox("#hl2r_warning_title", "#hl2r_editpanel_applymounted_disclaimer", this);
		box->SetOKButtonText("#hl2r_editpanel_applybutton_label");
		box->SetOKCommand(new KeyValues("Command", "command", "applychangesmountedconfirmed"));
		box->AddActionSignalTarget(this);
		box->DoModal();
	}
	if ( !stricmp(pcCommand, "applychangesmountedconfirmed") )
	{
		CGameManager *manager = GetGameManager();

		int iNewGametype = m_pGameSelectBox->GetActiveItem();
		if ( !manager->SetGameType(iNewGametype) )
		{
			MessageBox *box = new MessageBox("#hl2r_error_title", "#hl2r_mounterror_3", this);
			box->DoModal();
		}
		else
		{
			ApplyChanges();

			ResetPage();
			m_Parent->RefreshList();

			engine->ClientCmd("_restart");
		}
	}
	if ( !stricmp(pcCommand, "setstartmap") )
	{
		SetStartingMap( m_MapListPanel->GetSelectedItem() );
	}
	BaseClass::OnCommand(pcCommand);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::SetStartingMap(int selecteditemID)
{
	CCampaignDatabase* db = GetCampaignDatabase();
	m_iPrevStartMap = selecteditemID;

	db->GetCampaign(m_Campaign)->startingmap = selecteditemID;
	db->WriteListToScript();

	RefreshMapList();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::ItemSelected(int itemID)
{
	if (itemID != -1)
	{
		m_StartingMapButton->SetEnabled(itemID != m_iPrevStartMap);
		m_BackgroundMapButton->SetEnabled(itemID != m_iPrevBgMap);
	}
	else
	{
		m_StartingMapButton->SetEnabled(false);
		m_BackgroundMapButton->SetEnabled(false);

		return;
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::ApplySchemeSettings(IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_TextDisabledColor = GetSchemeColor("Label.TextDullColor", pScheme);
	m_TextEnabledColor = GetSchemeColor("Label.TextBrightColor", pScheme);

	m_EditBoxBackground->SetFillColor(GetSchemeColor("AchievementsLightGrey", pScheme));
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::ResetPage( void )
{
	m_MapListPanel->RemoveAll();

	V_strcpy(m_pPrevName, "");
	m_iPrevGame = -1;

	m_iPrevStartMap = -1;
	m_iPrevBgMap = -1;

	m_pNameEntry->SetText("");

	m_pGameSelectBox->SetText("");
	m_pGameSelectBox->SetActiveItemInvalid();

	m_StartingMapButton->SetEnabled(false);
	m_BackgroundMapButton->SetEnabled(false);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::RefreshMapList( void )
{
	m_MapListPanel->RemoveAll();
	CreateMapList();

	m_StartingMapButton->SetEnabled(false);
	m_BackgroundMapButton->SetEnabled(false);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::CreateMapList( void )
{
	CCampaignDatabase* db = GetCampaignDatabase();

	m_MapListPanelTitle->SetText("#hl2r_editpanel_maplist_title_1");
	m_MapListPanelTitleMapname->SetText("");

	for (int i = 0; i < db->GetCampaign(m_Campaign)->maplist.Count(); i++ )
	{
		KeyValues *pMap = new KeyValues("map");
		if ( !pMap )
			continue;

		char szMapLabel[CAMPAIGN_NAME_LENGTH];
		if ( db->GetCampaign(m_Campaign)->startingmap == i )
		{
			char szStartingMapString[64];
			wchar_t *pLocalizedStartingMapLabel = g_pVGuiLocalize->Find("hl2r_startmap_label");

			g_pVGuiLocalize->ConvertUnicodeToANSI( pLocalizedStartingMapLabel, szStartingMapString, sizeof(szStartingMapString) );
			V_sprintf_safe( szMapLabel, "%s%s", db->GetCampaign(m_Campaign)->maplist[i], szStartingMapString);

			m_MapListPanelTitle->SetText("#hl2r_editpanel_maplist_title_2");
			m_MapListPanelTitle->SetFgColor(m_TextEnabledColor);

			m_MapListPanelTitleMapname->SetText(db->GetCampaign(m_Campaign)->maplist[i]);
		}
		else
		{
			V_sprintf_safe( szMapLabel, "%s", db->GetCampaign(m_Campaign)->maplist[i]);
		}

		pMap->SetString("map", szMapLabel);
		int itemID = m_MapListPanel->AddItem(0, pMap );

		if ( db->GetCampaign(m_Campaign)->startingmap == i )
			m_MapListPanel->SetItemFgColor(itemID, COLOR_BLUE);
	}

}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::SetPageState( EPageState state)
{
	CCampaignDatabase *db = GetCampaignDatabase();
	bool bEditControlsVisible = false;
	if ( state == NO_CAMPAIGN_SELECTED )
	{
		m_CampaignInfoLabel->SetFgColor(m_TextDisabledColor);
		m_MapListPanelTitle->SetFgColor(m_TextDisabledColor);

		m_EditBoxBackground->SetVisible(true);
		m_CampaignIDLabel->SetText("");

		m_MapListPanelTitle->SetText("#hl2r_editpanel_maplist_title_1");
		m_MapListPanelTitleMapname->SetText("");
	}
	else
	{
		bEditControlsVisible = true;

		m_CampaignInfoLabel->SetFgColor(m_TextEnabledColor);
		m_MapListPanelTitle->SetFgColor(m_TextEnabledColor);

		m_EditBoxBackground->SetVisible(false);

		char szId[CAMPAIGN_ID_LENGTH + 2];
		V_sprintf_safe( szId, "(%s)", db->GetCampaign(m_Campaign)->id);
		m_CampaignIDLabel->SetText(szId);
	}

	m_pNameEntry->SetVisible(bEditControlsVisible);
	m_pNameEntryLabel->SetVisible(bEditControlsVisible);

	m_pGameSelectBox->SetVisible(bEditControlsVisible);
	m_pGameSelectBoxLabel->SetVisible(bEditControlsVisible);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::CheckApplyButton( void )
{
	m_ApplyButton->SetEnabled(true);
	m_ApplyButton->SetText("#hl2r_editpanel_applybutton_label");
	m_ApplyButton->SetCommand("applychanges");

	char nameEntry[CAMPAIGN_NAME_LENGTH];
	m_pNameEntry->GetText(nameEntry, CAMPAIGN_NAME_LENGTH);

	CCampaignDatabase* db = GetCampaignDatabase();
	if ( m_pGameSelectBox->GetActiveItem() == m_iPrevGame )
	{
		if ( !Q_stricmp(nameEntry, m_pPrevName) )
			m_ApplyButton->SetEnabled(false);
	}
	else
	{
		if ( db->GetCampaign(m_Campaign)->mounted )
		{
			m_ApplyButton->SetText("#hl2r_editpanel_applybutton_mounted_label");
			m_ApplyButton->SetCommand("applychangesmounted");
		}
	}

}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCampaignEditPanel::ApplyChanges( void )
{
	char entrytext[CAMPAIGN_NAME_LENGTH];
	m_pNameEntry->GetText(entrytext, CAMPAIGN_NAME_LENGTH);

	CCampaignDatabase* db = GetCampaignDatabase();
	for (int i = 0; i < db->GetCampaignCount(); i++)
	{
		if ( !Q_stricmp( db->GetCampaign(i)->id, db->GetCampaign(m_Campaign)->id ) )
			continue;

		if ( !Q_stricmp( db->GetCampaign(i)->name, entrytext ) )
		{
			MessageBox *box = new MessageBox("#hl2r_warning_title", "#hl2r_editpanel_error_1", this);
			box->DoModal();

			return false;
		}
	}

	V_strcpy(db->GetCampaign(m_Campaign)->name, !stricmp(entrytext, "") ? "undefined" : entrytext);
	db->GetCampaign(m_Campaign)->game = m_pGameSelectBox->GetActiveItem();

	db->SortCampaignList(db->GetSortType(), db->GetSortDir());
	db->WriteListToScript();

	return true;

}


//------------------------------------------------------------------------------
// Parent panel
//------------------------------------------------------------------------------
class CHL2RCampaignPanel : public vgui::PropertyDialog
{
	DECLARE_CLASS_SIMPLE(CHL2RCampaignPanel, vgui::PropertyDialog);

public:
	CHL2RCampaignPanel();
	~CHL2RCampaignPanel() {}

	virtual void OnThink();
	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);

	virtual void Activate();
	virtual void OnClose();
};

//-----------------------------------------------------------------------------
// Purpose: An extension of SectionedListPanelHeader that allows individual columns to be
// selected via togglebuttons that sends the currently selected column's index to a target panel.
//-----------------------------------------------------------------------------
class SelectableColumnHeader : public SectionedListPanelHeader
{
	DECLARE_CLASS_SIMPLE( SelectableColumnHeader, SectionedListPanelHeader );

public:
	SelectableColumnHeader(Panel* signaltarget, SectionedListPanel *parent, const char *name, int sectionID);

	virtual void PerformLayout();
	virtual void OnChildAdded(VPANEL child);

	virtual void ApplySchemeSettings(IScheme *pScheme);
	void OnCommand(const char* pcCommand);

	// Get the currently selected column.
	int		GetSelectedColumn( void );

	// Set what column is initially selected on startup.
	void	SetSelectedColumn( int iColumn, bool bDepressed );

private:
	CUtlVector<ToggleButton *>	m_ColumnButtons;

	int		m_iSelectedColumn;
	bool	m_iSelectedColumnDepressed;

	Color	m_SelectedColor;
	Color	m_UnselectedColor;
};


SelectableColumnHeader::SelectableColumnHeader(Panel* signaltarget, SectionedListPanel *parent, const char *name, int sectionID) : SectionedListPanelHeader(parent, name, sectionID)
{
	m_iSelectedColumn = -1;
	m_iSelectedColumnDepressed = true;

	AddActionSignalTarget(signaltarget);
}

// Just overriding this from the baseclass cause it throws errors: (professional programming practices)
void SelectableColumnHeader::OnChildAdded(VPANEL child)
{
//	Assert( !_flags.IsFlagSet( IN_PERFORM_LAYOUT ) );
}

void SelectableColumnHeader::PerformLayout() 
{
	int colCount = m_pListPanel->GetColumnCountBySection(m_iSectionID);

	int xpos = 0;
	for (int i = 0; i < colCount; i++)
	{
		int columnWidth = m_pListPanel->GetColumnWidthBySection(m_iSectionID, i);

		int x, y, wide, tall;
		GetBounds(x, y, wide, tall);

		// Create a button for this column:
		char szColumnLabel[64];
		const wchar_t *pColumnLabel = m_pListPanel->GetColumnTextBySection(m_iSectionID, i);
		g_pVGuiLocalize->ConvertUnicodeToANSI(pColumnLabel, szColumnLabel, sizeof(szColumnLabel));

		char szButtonName[32];
		V_sprintf_safe(szButtonName, "columnbutton_%d", i);

		ToggleButton *pColumnButton = new ToggleButton(this, szButtonName, szColumnLabel);
		pColumnButton->AddActionSignalTarget(this->GetVPanel());
		pColumnButton->SetCommand(m_pListPanel->GetColumnNameBySection(m_iSectionID, i));
		pColumnButton->SetSize(columnWidth, tall);
		pColumnButton->SetPos(xpos, 0);

		// SetFgColor doesn't want work on these buttons.
	//	pColumnButton->SetFgColor(m_UnselectedColor);

		if ( m_iSelectedColumn == i )
		{
			pColumnButton->SetSelected(true);

			if ( m_iSelectedColumnDepressed )
				pColumnButton->ForceDepressed(true);

		//	pColumnButton->SetFgColor(m_SelectedColor);
		}

		m_ColumnButtons.AddToTail(pColumnButton);
		xpos += columnWidth;
	}
};


void SelectableColumnHeader::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetFont(pScheme->GetFont("Default", IsProportional()));

	m_SelectedColor = GetSchemeColor("Label.TextDullColor", pScheme);
	m_UnselectedColor = GetSchemeColor("Label.TextBrightColor", pScheme);
}

void SelectableColumnHeader::OnCommand(const char* pcCommand)
{
	// Only allow 1 column to be selected at a time.
	int colCount = m_pListPanel->GetColumnCountBySection(m_iSectionID);
	for (int i = 0; i < colCount; i++ )
	{
		const char *pColumnName = m_pListPanel->GetColumnNameBySection(m_iSectionID, i);
		if ( !stricmp(pcCommand, pColumnName) )
		{
			m_iSelectedColumn = i;
			PostActionSignal(new KeyValues("ColumnSelected", "column", m_iSelectedColumn));

			m_pListPanel->ClearSelection();
		}
		else
		{
			m_ColumnButtons[i]->SetSelected(false);
			m_ColumnButtons[i]->ForceDepressed(false);
		}
	}

	BaseClass::OnCommand(pcCommand);
}

int SelectableColumnHeader::GetSelectedColumn( void )
{
	return m_iSelectedColumn;
}

void SelectableColumnHeader::SetSelectedColumn( int iColumn, bool bDepressed )
{
	m_iSelectedColumn = iColumn;
	m_iSelectedColumnDepressed = bDepressed;
}
#endif