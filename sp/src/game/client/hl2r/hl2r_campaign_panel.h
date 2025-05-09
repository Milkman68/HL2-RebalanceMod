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
/*
class CCampaignSectionedListPanel : public SectionedListPanel
{
	DECLARE_CLASS_SIMPLE( CCampaignSectionedListPanel, SectionedListPanel );

	CCampaignSectionedListPanel(CCampaignListPanel *parent, const char *name);
};
*/
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
#define BROWSER_MAX_URL 256

class CCampaignBrowserPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCampaignBrowserPanel, EditablePanel );

public:
	CCampaignBrowserPanel(Panel *parent, const char *name) : EditablePanel(parent, name) 
	{
		m_CampaignWindowBackground = new ImagePanel( this, "CampaignWindowBackground" );
		m_CampaignWindowDivider = new Divider( this, "CampaignWindowDivider" );

		m_CampaignWindow = new HTML(this, "CampaignWindow");
		m_CampaignWindow->DisableBrowserClicks(true);
		m_CampaignWindow->SetContextMenuEnabled(false);

		LoadControlSettings("resource/ui/hl2r_browserwindow.res");
	}

	virtual void ApplySchemeSettings(IScheme* pScheme)
	{
		m_CampaignWindowBackground->SetFillColor(GetSchemeColor("AchievementsLightGrey", pScheme));
		BaseClass::ApplySchemeSettings(pScheme);
	}

	void SetSelected( CampaignData_t *campaign ) 
	{
		if ( campaign != NULL )
		{
			// Get the ID of the selected campaign and parse it into a url that can be opened.
			char szURL[BROWSER_MAX_URL];
			Q_snprintf(szURL, sizeof(szURL), "https://steamcommunity.com/sharedfiles/filedetails?id=%s", campaign->id );

			DevMsg("URL is: %s\n", szURL );

			m_CampaignWindow->OpenURL(szURL, "" );
		}

		m_CampaignWindow->SetAlpha(0);
	}

	MESSAGE_FUNC( OnFinishRequest, "OnFinishRequest" )
	{
		GetAnimationController()->RunAnimationCommand(m_CampaignWindow, "Alpha", 255.0f, 0.15, 0.15, AnimationController::INTERPOLATOR_LINEAR);
	}

private:
	HTML		*m_CampaignWindow;
	ImagePanel	*m_CampaignWindowBackground;
	Divider		*m_CampaignWindowDivider;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

class CCampaignEditPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCampaignEditPanel, EditablePanel );

public:
	CCampaignEditPanel(CCampaignListPanel *parent, const char *name) : EditablePanel(parent, name) 
	{
		m_Parent = parent;

		m_EditBoxBackground = new ImagePanel( this, "EditBoxBackground" );
		m_EditBoxDivider = new Divider( this, "EditBoxDivider" );

		m_MapListPanel = new SectionedListPanel(this, "listpanel_maplist");

		m_MapListPanel->AddSection(0, "column0");
		m_MapListPanel->AddColumnToSection(0, "map", "#hl2r_maplist_maplabel", SectionedListPanel::COLUMN_BRIGHT, 384);
		m_MapListPanelLabel= new Label(this, "MapListPanelLabel", "");

		m_CampaignIDLabel = new Label(this, "CampaignIDLabel", "");
		m_CampaignInfoLabel = new Label(this, "CampaignInfoLabel", "");

		m_ApplyButton = new Button(this, "ApplyButton", "" );
		m_ApplyButton->SetCommand("applychanges");

		m_pNameEntry = new TextEntry(this, "NameEntry");
		m_pNameEntryLabel = new Label(this, "NameEntryLabel", "");

		m_pGameSelectBox = new ComboBox(this, "GameSelectBox", 3, false);
		m_pGameSelectBox->AddItem("Half-Life 2", NULL);
		m_pGameSelectBox->AddItem("Episode 1", NULL);
		m_pGameSelectBox->AddItem("Episode 2", NULL);
		m_pGameSelectBoxLabel = new Label(this, "GameSelectBoxLabel", "");

		LoadControlSettings("resource/ui/hl2r_editpanel.res");
	}

	void SetCampaign( CampaignData_t *campaign )
	{
		m_Campaign = campaign;

		ResetPage();

		if ( GetCampaign() )
		{
			FillMapList();
			SetEditboxEnabled(true);

			if (Q_stricmp(GetCampaign()->name, "undefined"))
				m_pNameEntry->SetText(GetCampaign()->name);

			if (GetCampaign()->game != -1)
				m_pGameSelectBox->ActivateItem(GetCampaign()->game);

			m_pNameEntry->GetText(m_pPrevName, 512);
			m_iPrevGame = m_pGameSelectBox->GetActiveItem();

		}
		else
		{
			SetEditboxEnabled(false);
		}
	}

	void OnCommand(const char* pcCommand)
	{
		if ( !stricmp(pcCommand, "applychanges") )
		{
			char entrytext[CAMPAIGN_NAME_LENGTH];
			m_pNameEntry->GetText(entrytext, CAMPAIGN_NAME_LENGTH);

			CCampaignDatabase *database = GetCampaignDatabase();
			for ( int i = 0; i < database->GetCampaignCount(); i++ )
			{
				if ( !Q_stricmp(database->GetCampaignData(i)->id, GetCampaign()->id ) )
					continue;

				if ( !Q_stricmp(database->GetCampaignData(i)->name, entrytext ) )
				{
					MessageBox *box = new MessageBox("#hl2r_warning_title", "#hl2r_editerror_1", this);
					box->DoModal();

					return;
				}
			}

			V_strcpy( GetCampaign()->name, !stricmp(entrytext, "")  ? "undefined" : entrytext );
			GetCampaign()->game = m_pGameSelectBox->GetActiveItem();

			database->SortCampaignList(database->GetSortType(), database->GetSortDir());
			database->WriteListToScript();

			ResetPage();
			m_Parent->RefreshList();
		}
		BaseClass::OnCommand(pcCommand);
	}

	void ApplySchemeSettings(IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		m_TextDisabledColor = GetSchemeColor("Label.TextDullColor", pScheme);
		m_TextEnabledColor = GetSchemeColor("Label.TextBrightColor", pScheme);

		m_EditBoxBackground->SetFillColor(GetSchemeColor("AchievementsLightGrey", pScheme));
	}

	MESSAGE_FUNC( OnTextChanged, "TextChanged" ) { CheckApplyButton(); }
	MESSAGE_FUNC( OnMenuItemSelected, "MenuItemSelected" ){	CheckApplyButton();	}

private:
	CampaignData_t *GetCampaign( void )
	{
		return m_Campaign;
	}

	void ResetPage()
	{
		m_MapListPanel->RemoveAll();

		V_strcpy( m_pPrevName, "");
		m_iPrevGame = -1;

		m_pNameEntry->SetText("");

		m_pGameSelectBox->SetText("");
		m_pGameSelectBox->SetActiveItemInvalid();
	}

	void FillMapList()
	{
		for (int i = 0; i < GetCampaign()->maplist.Count(); i++ )
		{
			KeyValues *pMap = new KeyValues("map");
			if ( !pMap )
				continue;

			pMap->SetString("map", GetCampaign()->maplist[i]);
			m_MapListPanel->AddItem(0, pMap );
		}

	}

	void SetEditboxEnabled( bool bEnable )
	{
		if ( bEnable )
		{
			char szId[CAMPAIGN_ID_LENGTH + 2];
			V_sprintf_safe( szId, "(%s)", GetCampaign()->id);
			m_CampaignIDLabel->SetText(szId);

			m_CampaignInfoLabel->SetFgColor(m_TextEnabledColor);

			m_MapListPanelLabel->SetFgColor(m_TextEnabledColor);
		}
		else
		{
			m_CampaignIDLabel->SetText("");
			m_CampaignInfoLabel->SetFgColor(m_TextDisabledColor);

			m_MapListPanelLabel->SetFgColor(m_TextDisabledColor);
		}


		m_EditBoxBackground->SetVisible(!bEnable);

		m_pNameEntry->SetVisible(bEnable);
		m_pNameEntryLabel->SetVisible(bEnable);

		m_pGameSelectBox->SetVisible(bEnable);
		m_pGameSelectBoxLabel->SetVisible(bEnable);
	}

	void CheckApplyButton( void )
	{
		m_ApplyButton->SetEnabled(true);

		char nameEntry[CAMPAIGN_NAME_LENGTH];
		m_pNameEntry->GetText(nameEntry, CAMPAIGN_NAME_LENGTH);

		if ( !Q_stricmp(nameEntry, m_pPrevName) && m_pGameSelectBox->GetActiveItem() == m_iPrevGame )
			m_ApplyButton->SetEnabled(false);
	}

private:
	CCampaignListPanel	*m_Parent;
	CampaignData_t		*m_Campaign;

	Label				*m_CampaignIDLabel;
	Label				*m_CampaignInfoLabel;

	Label				*m_pNameEntryLabel;
	Label				*m_pGameSelectBoxLabel;
	
	Label				*m_MapListPanelLabel;

	TextEntry			*m_pNameEntry;
	ComboBox			*m_pGameSelectBox;

	Button				*m_ApplyButton;

	ImagePanel			*m_EditBoxBackground;
	Divider				*m_EditBoxDivider;

	SectionedListPanel	*m_MapListPanel;

	Color	m_TextDisabledColor;
	Color	m_TextEnabledColor;

	char		m_pPrevName[CAMPAIGN_NAME_LENGTH];
	int			m_iPrevGame;
};
//-----------------------------------------------------------------------------
// Purpose: A dialog for editing a single campaign item's parameters.
//-----------------------------------------------------------------------------
/*
class CCampaignEditPanel : public PropertyDialog
{
	DECLARE_CLASS_SIMPLE( CCampaignEditPanel, Frame );
public:
	CCampaignEditPanel(CCampaignListPanel *parent, CampaignData_t *pCampaign);
	void InitControls(void);

	void ResetControls(void);
	void InvalidateControls( void );
	void ReinstateControls( void );

	virtual void Activate();
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodeTyped(KeyCode code);
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel);

private:
	CampaignData_t *m_pCampaign;
	CCampaignListPanel *m_pParent;

	TextEntry		*m_pNameEntry;
	ComboBox		*m_pGameSelectBox;
	Button			*m_pResetButton;
	CheckButton		*m_MarkInvalidButton;

	KeyValues	*m_pPrevSettings;
	bool		m_bControlsInvalidated;
};
*/
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