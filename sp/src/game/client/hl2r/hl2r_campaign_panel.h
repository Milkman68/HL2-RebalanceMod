#ifndef hl2r_campaign_panel
#define hl2r_campaign_panel
#ifdef _WIN32
#pragma once
#endif

#include "hl2r_campaign_database.h"

#include <vgui_controls/HTML.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Divider.h>

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
	CSelectedCampaignPanel(Panel *parent, const char *name);
	void SetSelected( CampaignData_t *campaign );

private:
	CCampaignBrowserPanel	*m_BrowserPanel;
	CCampaignEditPanel		*m_EditPanel;
};
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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
		m_BackgroundColor = GetSchemeColor("AchievementsLightGrey", pScheme);
		m_CampaignWindowBackground->SetFillColor(m_BackgroundColor);

		BaseClass::ApplySchemeSettings(pScheme);
	}

	void Open( CampaignData_t *campaign ) 
	{
		if ( campaign != NULL )
		{
			// Get the ID of the selected campaign and parse it into a url that can be opened.
			char szURL[128];
			Q_snprintf(szURL, sizeof(szURL), "https://steamcommunity.com/sharedfiles/filedetails?id=%s", campaign->id );

			m_CampaignWindow->OpenURL(szURL, "" );
		}
		else
		{
			m_CampaignWindow->SetVisible(false);
		}
	}

	MESSAGE_FUNC( OnFinishRequest, "OnFinishRequest" )
	{
		m_CampaignWindow->SetVisible(true);
	}

private:
	HTML		*m_CampaignWindow;
	ImagePanel	*m_CampaignWindowBackground;
	Divider		*m_CampaignWindowDivider;

	Color		m_BackgroundColor;
};
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCampaignEditPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCampaignEditPanel, EditablePanel );

public:
	CCampaignEditPanel(Panel *parent, const char *name) : EditablePanel(parent, name) 
	{
		LoadControlSettings("resource/ui/hl2r_editpanel.res");
	}
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