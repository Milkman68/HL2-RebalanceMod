#ifndef hl2r_campaign_panel
#define hl2r_campaign_panel
#ifdef _WIN32
#pragma once
#endif

#include "hl2r_campaign_database.h"

#define PANEL_WIDTH 512
#define PANEL_HEIGHT 640

#define EDITPANEL_WIDTH 400
#define EDITPANEL_HEIGHT 220

//------------------------------------------------------------------------------
// Sub panel
//------------------------------------------------------------------------------
class CCampaignListPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CCampaignListPanel, vgui::EditablePanel);

public:
	CCampaignListPanel(vgui::Panel* parent );
	~CCampaignListPanel() {}

	MESSAGE_FUNC_INT( ItemSelected, "ItemSelected", itemID );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked",) {RefreshList();}
	virtual void OnCommand(const char* pcCommand);

	void RefreshList( void );

private:
	void CreateListColunms( void );
	void CreateList( void );

private:
	SectionedListPanel	*m_ListPanel;

	Button			*m_EditButton;
	Button			*m_MountButton;
	Button			*m_OpenInBrowserButton;
	Button			*m_HelpButton;

	CheckButton		*m_ViewInvalidButton;
};

//-----------------------------------------------------------------------------
// Purpose: A dialog for editing a single campaign item's parameters.
//-----------------------------------------------------------------------------
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