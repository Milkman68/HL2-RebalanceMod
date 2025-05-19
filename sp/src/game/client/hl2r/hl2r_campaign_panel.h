#ifndef hl2r_campaign_panel
#define hl2r_campaign_panel
#ifdef _WIN32
#pragma once
#endif

#include "hl2r_campaign_database.h"

#include <vgui_controls/HTML.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/ComboBox.h>

#define PANEL_WIDTH 896
#define PANEL_HEIGHT 640

#define EDITPANEL_WIDTH 400
#define EDITPANEL_HEIGHT 220

class CSelectedCampaignPanel;
class CCampaignBrowserPanel;
class CCampaignEditPanel;

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
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked",) {RefreshList();}

	virtual void OnCommand(const char* pcCommand); 
	void RefreshList( void );

private:
	void CreateListColunms( void );
	void CreateList( void );

	void HandleCampaignMount( const char *szCampaignID );
	void HandleCampaignUnmount( void );
	void HandleCampaignScan( void );

private:
	SectionedListPanel		*m_ListPanel;
	CSelectedCampaignPanel	*m_SelectedCampaignPanel;

	// Buttons:
	Button	*m_MountButton;
	Button	*m_CampaignScanButton;
};




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSelectedCampaignPanel : public PropertySheet
{
	DECLARE_CLASS_SIMPLE( CSelectedCampaignPanel, PropertySheet );

public:
	CSelectedCampaignPanel(CCampaignListPanel *parent, const char *name);
	void SetSelected( CampaignData_t *campaign );

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

	void SetSelected( CampaignData_t *campaign ) ;

	virtual void ApplySchemeSettings(IScheme* pScheme);
	MESSAGE_FUNC( OnFinishRequest, "OnFinishRequest" );

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

	LoadControlSettings("resource/ui/hl2r_browserwindow.res");
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define BROWSER_MAX_URL 256
void CCampaignBrowserPanel::SetSelected(CampaignData_t* campaign)
{
	if (campaign != NULL)
	{
		// Get the ID of the selected campaign and parse it into a url that can be opened.
		char szURL[BROWSER_MAX_URL];
		Q_snprintf(szURL, sizeof(szURL), "https://steamcommunity.com/sharedfiles/filedetails?id=%s", campaign->id);

		DevMsg("URL is: %s\n", szURL);

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

	void SetCampaign( CampaignData_t *campaign );
	MESSAGE_FUNC_INT( ItemSelected, "ItemSelected", itemID );

	void OnCommand(const char* pcCommand);
	void ApplySchemeSettings(IScheme *pScheme);

	MESSAGE_FUNC( OnTextChanged, "TextChanged" ) { CheckApplyButton(); }
	MESSAGE_FUNC( OnMenuItemSelected, "MenuItemSelected" ){	CheckApplyButton();	}

private:
	CampaignData_t *GetCampaign( void ) { return m_Campaign; }

	void SetPageState( EPageState type);

	void CreateMapList(void);
	void RefreshMapList(void);

	void ResetPage(void);
	void CheckApplyButton( void );
	bool ApplyChanges( void );

private:
	CCampaignListPanel	*m_Parent;
	CampaignData_t		*m_Campaign;

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

	LoadControlSettings("resource/ui/hl2r_editpanel.res");
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::SetCampaign(CampaignData_t* campaign)
{
	m_Campaign = campaign;

	ResetPage();

	if (GetCampaign())
	{
		CreateMapList();
		if (GetCampaign()->mounted)
		{
			SetPageState(MOUNTED_CAMPAIGN);
		}
		else
		{
			SetPageState(UNMOUNTED_CAMPAIGN);
		}

		if (Q_stricmp(GetCampaign()->name, "undefined"))
			m_pNameEntry->SetText(GetCampaign()->name);

		if (GetCampaign()->game != -1)
			m_pGameSelectBox->ActivateItem(GetCampaign()->game);

		if (GetCampaign()->startingmap != -1)
			m_iPrevStartMap = GetCampaign()->startingmap;

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
		CCampaignDatabase *database = GetCampaignDatabase();

		int iCurrentGametype = database->GetMountedCampaign()->game;
		int iNewGametype = m_pGameSelectBox->GetActiveItem();

		if ( !database->TransferGameinfoFiles(iCurrentGametype, iNewGametype))
		{
			MessageBox *box = new MessageBox("#hl2r_error_title", "#hl2r_mounterror_3", this);
			box->DoModal();
		}
		else
		{
			ApplyChanges();

			ResetPage();
			m_Parent->RefreshList();

			engine->ClientCmd("quit");
		}
	}
	if ( !stricmp(pcCommand, "setstartmap") )
	{
		int selecteditemID = m_MapListPanel->GetSelectedItem();

		m_iPrevStartMap = selecteditemID;
		GetCampaign()->startingmap = selecteditemID;

		GetCampaignDatabase()->WriteListToScript();
		RefreshMapList();
	}
	BaseClass::OnCommand(pcCommand);
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
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::CreateMapList( void )
{
	m_MapListPanelTitle->SetText("#hl2r_editpanel_maplist_title_1");
	m_MapListPanelTitleMapname->SetText("");

	for (int i = 0; i < GetCampaign()->maplist.Count(); i++ )
	{
		KeyValues *pMap = new KeyValues("map");
		if ( !pMap )
			continue;

		char szMapLabel[CAMPAIGN_NAME_LENGTH];
		if ( GetCampaign()->startingmap == i )
		{
			char szStartingMapString[64];
			wchar_t *pLocalizedStartingMapLabel = g_pVGuiLocalize->Find("hl2r_startmap_label");

			g_pVGuiLocalize->ConvertUnicodeToANSI( pLocalizedStartingMapLabel, szStartingMapString, sizeof(szStartingMapString) );
			V_sprintf_safe( szMapLabel, "%s%s", GetCampaign()->maplist[i], szStartingMapString);

			m_MapListPanelTitle->SetText("#hl2r_editpanel_maplist_title_2");
			m_MapListPanelTitle->SetFgColor(m_TextEnabledColor);

			m_MapListPanelTitleMapname->SetText(GetCampaign()->maplist[i]);
		}
		else
		{
			V_sprintf_safe( szMapLabel, "%s", GetCampaign()->maplist[i]);
		}

		pMap->SetString("map", szMapLabel);
		int itemID = m_MapListPanel->AddItem(0, pMap );

		if ( GetCampaign()->startingmap == i )
			m_MapListPanel->SetItemFgColor(itemID, COLOR_BLUE);
	}

}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCampaignEditPanel::SetPageState( EPageState state)
{
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
		V_sprintf_safe( szId, "(%s)", GetCampaign()->id);
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

	if ( m_pGameSelectBox->GetActiveItem() == m_iPrevGame )
	{
		if ( !Q_stricmp(nameEntry, m_pPrevName) )
			m_ApplyButton->SetEnabled(false);
	}
	else
	{
		if ( GetCampaign()->mounted )
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

	CCampaignDatabase* database = GetCampaignDatabase();
	for (int i = 0; i < database->GetCampaignCount(); i++)
	{
		if (!Q_stricmp(database->GetCampaignData(i)->id, GetCampaign()->id))
			continue;

		if (!Q_stricmp(database->GetCampaignData(i)->name, entrytext))
		{
			MessageBox *box = new MessageBox("#hl2r_warning_title", "#hl2r_editpanel_error_1", this);
			box->DoModal();

			return false;
		}
	}

	V_strcpy(GetCampaign()->name, !stricmp(entrytext, "") ? "undefined" : entrytext);
	GetCampaign()->game = m_pGameSelectBox->GetActiveItem();

	database->SortCampaignList(database->GetSortType(), database->GetSortDir());
	database->WriteListToScript();

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
#endif