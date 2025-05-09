#include "cbase.h"
using namespace vgui;
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/QueryBox.h>
#include "vgui_controls/AnimationController.h"
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include "hl2r_campaign_panel.h"
#include "hl2r_campaign_database.h"

// TODO:
// Mount function only works like 1/20th of the time.
// Could add a nice loading bar for scanning maybe?

// add new tab for managing exclusively mounted campaigns.

// Actually might want to just let the player choose the starting map when "begin campaign" is pressed 
// instead of adding it to the manager. Hmmmmm...

// Every single campaign gets recreated when scanned even if already in the script.

// optimize scanning by not reading default_campaigns.txt every loop iteration.
// 
// Discolor unmountable campaigns a little.
// 
//------------------------------------------------------------------------------
// Purpose : Code for hooking into the base GameUI panel. Credit goes to: 
// https://github.com/HL2RP/HL2RP
//------------------------------------------------------------------------------
static VPANEL FindChildPanel(VPANEL parent, CUtlStringList& pathNames, int index = 0)
{
	if (index >= pathNames.Size())
	{
		return parent;
	}

	for (auto child : g_pVGuiPanel->GetChildren(parent))
	{
		if (Q_strcmp(g_pVGuiPanel->GetName(child), pathNames[index]) == 0)
		{
			return FindChildPanel(child, pathNames, index + 1);
		}
	}

	return 0;
}

static Panel* FindGameUIChildPanel(const char* pPath)
{
	CUtlStringList pathNames;
	Q_SplitString(pPath, "/", pathNames);
	return g_pVGuiPanel->GetPanel(FindChildPanel(enginevgui->GetPanel(PANEL_GAMEUIDLL), pathNames), "GameUI");
}

//------------------------------------------------------------------------------
// Purpose: Helper function for determining screen proportion scaling values
//------------------------------------------------------------------------------
static int GetAdjustedSize( int iValue )
{
	int screenW, screenH;
	surface()->GetScreenSize( screenW, screenH );
	
	float flRatio = MAX( MAX( 1.0f, (float)screenW / 1920.0f ), MAX( 1.0f, (float)screenH / 1080.0f ) );
	iValue *= (float)flRatio;
	
	return (int)iValue;
}
//------------------------------------------------------------------------------
// Sub panel
//------------------------------------------------------------------------------
CCampaignListPanel::CCampaignListPanel(vgui::Panel* parent ) : EditablePanel(parent, NULL)
{
	m_ListPanel = new SectionedListPanel(this, "listpanel_campaignlist");

	m_MountButton = new Button(this, "MountButton", "");
	m_MountButton->SetCommand("mountitem");

	m_CampaignScanButton = new Button(this, "CampaignScanButton", "");
	m_CampaignScanButton->SetCommand("scancampaigns");

	m_SelectedCampaignPanel = new CSelectedCampaignPanel(this, "SelectedCampaignPanel" );
	m_SelectedCampaignPanel->SetSelected(NULL);

	LoadControlSettings("resource/ui/hl2r_campaignlist.res");

	CreateListColunms();
	CreateList();
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::ItemSelected(int itemID)
{
	// Buttons:
//	bool bHasItemSelected = m_ListPanel->IsItemIDValid(itemID);
//	bool bCanItemBeMounted = m_ListPanel->GetItemSection(itemID) == 1;

//	m_MountButton->SetEnabled(bHasItemSelected && bCanItemBeMounted);

	if (itemID != -1)
	{
		const char *SelectedItemID = m_ListPanel->GetSelectedItemData()->GetString("id");
		CampaignData_t *pSelectedCampaign = GetCampaignDatabase()->GetCampaignDataFromID(SelectedItemID);
		
		m_SelectedCampaignPanel->SetSelected(pSelectedCampaign);
		m_MountButton->SetEnabled( Q_stricmp( pSelectedCampaign->name, "undefined" ) && pSelectedCampaign->game != -1 );
	}
	else	
	{
		m_SelectedCampaignPanel->SetSelected(NULL);
	}
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::RefreshList(void)
{
	bool bHasItemSelected = m_ListPanel->GetSelectedItem() != -1;
	char SelectedItemID[CAMPAIGN_ID_LENGTH];

	if ( bHasItemSelected )
		V_strcpy(SelectedItemID, m_ListPanel->GetSelectedItemData()->GetString("id"));

	m_ListPanel->RemoveAll();
	m_ListPanel->RemoveAllSections();

	CreateListColunms();
	CreateList();

	if ( !bHasItemSelected )
		return;

	for (int i = 0; i < m_ListPanel->GetItemCount(); i++ )
	{
		if ( !Q_stricmp( m_ListPanel->GetItemData(i)->GetString("id"), SelectedItemID ) )
		{
			m_ListPanel->SetSelectedItem(i);
			m_ListPanel->ScrollToItem(i);
		}
	}
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
#define NAME_WIDTH	256
#define SIZE_WIDTH	112
#define DATE_WIDTH	84
void CCampaignListPanel::CreateListColunms(void)
{
	m_ListPanel->AddSection(0, "column0");
	m_ListPanel->AddColumnToSection(0, "namelabel", "Label:", SectionedListPanel::COLUMN_BRIGHT, NAME_WIDTH);	// FIX: UNLOCALIZED STRING!!!
	m_ListPanel->AddColumnToSection(0, "size", "Filesize:", SectionedListPanel::COLUMN_BRIGHT, SIZE_WIDTH);		// FIX: UNLOCALIZED STRING!!!
	m_ListPanel->AddColumnToSection(0, "date", "Date:", SectionedListPanel::COLUMN_BRIGHT, DATE_WIDTH);			// FIX: UNLOCALIZED STRING!!!
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::CreateList(void)
{
	for (int i = 0; i < GetCampaignDatabase()->GetCampaignCount(); i++ )
	{
		CampaignData_t *pDatabaseCampaign = GetCampaignDatabase()->GetCampaignData(i);
		KeyValues *pCampaign = GetCampaignDatabase()->GetKeyValuesFromCampaign(pDatabaseCampaign);

		// Don't do anything if this shouldn't be visible.
		if (!pCampaign->GetBool("installed"))
			continue;

		const char *pCampaignName = pCampaign->GetString("Name");
		pCampaignName = !Q_stricmp(pCampaignName, "undefined" ) ? "#hl2r_list_undefined_label" : pCampaignName;

		char pFilesize[CAMPAIGN_FILESIZE_LENGTH];
		V_sprintf_safe(pFilesize, "%s MB", pCampaign->GetString("filesize") );

		pCampaign->SetString("namelabel", pCampaignName);
		pCampaign->SetString("size", pFilesize );
		pCampaign->SetString("date", "0/00/0000");

		int itemID = m_ListPanel->AddItem(0, pCampaign);

		if ( !Q_stricmp( pCampaign->GetString("Name"), "undefined" ) || pCampaign->GetInt("Game") == -1 )
			m_ListPanel->SetItemFgColor(itemID, Color( 160, 160, 160, 255 )); // FIX: MAGIC COLOR!!!

		if ( pCampaign->GetBool("mounted") )
			m_ListPanel->SetItemFgColor(itemID, COLOR_GREEN);
	}
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::OnCommand(const char* pcCommand)
{
	if ( !stricmp(pcCommand, "scancampaigns") || !stricmp(pcCommand, "mountitem") )
	{
		if ( !GetCampaignDatabase()->HLExtractInstalled() )
		{
			MessageBox *box = new MessageBox("#hl2r_list_mounterror_title", "#hl2r_list_missing_hlextract", this);
			box->DoModal();

			BaseClass::OnCommand(pcCommand);
			return;
		}
	}

/*	if (!stricmp(pcCommand, "edititem"))
	{
		// Create a temporary editor panel for the campaign we selected.
		const char *SelectedItemID = m_ListPanel->GetSelectedItemData()->GetString("id");
		CampaignData_t *pSelectedCampaign = GetCampaignDatabase()->GetCampaignDataFromID(SelectedItemID);

		CCampaignEditPanel *pEditPanel = new CCampaignEditPanel(this, pSelectedCampaign);

		pEditPanel->Activate();
		pEditPanel->AddActionSignalTarget(this);
	}
	else */
	
	if (!stricmp(pcCommand, "mountitem"))
	{
		// Create a disclaimer box confirming if this is our intended action.
		QueryBox *box = new QueryBox("#hl2r_list_mountdisclaimer_title", "#hl2r_list_mountdisclaimer_text", this);
		box->SetOKButtonText("#hl2r_list_mountdisclaimer_accept");
		box->SetOKCommand(new KeyValues("Command", "command", "mountconfirmed"));
		box->AddActionSignalTarget(this);
		box->DoModal();
	}
	else if (!stricmp(pcCommand, "mountconfirmed"))
	{
		const char* szCampaignID = m_ListPanel->GetItemData(m_ListPanel->GetSelectedItem())->GetString("id");
		HandleCampaignMount(szCampaignID);
	}
	else if (!stricmp(pcCommand, "scancampaigns"))
	{
		// Create a disclaimer box confirming if this is our intended action.
		QueryBox *box = new QueryBox("#hl2r_list_scandisclaimer_title", "#hl2r_list_scandisclaimer_text", this);
		box->SetOKButtonText("#hl2r_list_scandisclaimer_accept");
		box->SetOKCommand(new KeyValues("Command", "command", "scanconfirmed"));
		box->AddActionSignalTarget(this);
		box->DoModal();
	}
	else if (!stricmp(pcCommand, "scanconfirmed"))
	{
		HandleCampaignScan();
	}

	BaseClass::OnCommand(pcCommand);
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::HandleCampaignMount( const char *szCampaignID )
{

switch (GetCampaignDatabase()->MountCampaign(szCampaignID))
	{
	case SUCESSFULLY_MOUNTED:
		{
			MessageBox *box = new MessageBox("#hl2r_list_mountsucess_title", "#hl2r_list_mountsucess_text", this);
			box->SetOKButtonText("#hl2r_list_mountsucess_accept");
			box->DoModal();
			break;
		}

	case FAILED_TO_EXTRACT_VPK:
		{
			MessageBox *box = new MessageBox("#hl2r_list_mounterror_title", "#hl2r_list_mounterror_text_1", this);
			box->DoModal();
			break;
		}
	}
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::HandleCampaignScan( void )
{
	CCampaignDatabase *database = GetCampaignDatabase();

	database->DoCampaignScan();
	database->SortCampaignList(database->GetSortType(), database->GetSortDir());
	database->WriteListToScript();

	RefreshList();
}



//------------------------------------------------------------------------------
// Edit panel
//------------------------------------------------------------------------------
CSelectedCampaignPanel::CSelectedCampaignPanel(CCampaignListPanel *parent, const char *name) : PropertySheet(parent, name)
{
	m_BrowserPanel = new CCampaignBrowserPanel(this, "browserpanel");
	AddPage(m_BrowserPanel, "View Workshop");

	m_EditPanel = new CCampaignEditPanel(parent, "editpanel");
	AddPage(m_EditPanel, "Properties");
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CSelectedCampaignPanel::SetSelected( CampaignData_t *campaign )
{
	if ( GetActivePage()->GetName() == m_EditPanel->GetName() )
	{
		GetActivePage()->SetAlpha(0);
		GetAnimationController()->RunAnimationCommand(GetActivePage(), "Alpha", 255.0f, 0.15, 0.15, AnimationController::INTERPOLATOR_LINEAR);
	}

	m_BrowserPanel->SetSelected(campaign);
	m_EditPanel->SetCampaign(campaign);
}


/*
//------------------------------------------------------------------------------
// Edit panel
//------------------------------------------------------------------------------
CCampaignEditPanel::CCampaignEditPanel(CCampaignListPanel *parent, CampaignData_t *pCampaign) : PropertyDialog(FindGameUIChildPanel("BaseGameUIPanel"), "OtherSettings")
{
	SetTitle("#hl2r_listeditor_title", true);
	SetBounds(0, 0, GetAdjustedSize(EDITPANEL_WIDTH), GetAdjustedSize(EDITPANEL_HEIGHT) );
	MoveToCenterOfScreen();
	SetSizeable( false );
	SetCloseButtonVisible(false);
	SetMenuButtonResponsive(false);
	SetDeleteSelfOnClose( true );

	m_pCampaign = pCampaign;
	m_pParent = parent;

	m_bControlsInvalidated = false;
	m_pPrevSettings = new KeyValues("PreviousSettings");

	InitControls();
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignEditPanel::InitControls(void)
{
	// Text entry
	m_pNameEntry = new TextEntry(this, "NameEntry");

	if (Q_stricmp(m_pCampaign->name, "undefined"))
		m_pNameEntry->SetText(m_pCampaign->name);

	// Dropdown box:
	m_pGameSelectBox = new ComboBox(this, "GameSelectBox", 3, false);
	m_pGameSelectBox->AddItem("Half-Life 2", NULL);
	m_pGameSelectBox->AddItem("Episode 1", NULL);
	m_pGameSelectBox->AddItem("Episode 2", NULL);

	if (m_pCampaign->game != -1)
		m_pGameSelectBox->ActivateItem(m_pCampaign->game);

	// Button:
	m_pResetButton = new Button(this, "ResetButton", "");
	m_pResetButton->SetCommand("reset");

	// Checkbutton:
	m_MarkInvalidButton = new CheckButton(this, "MarkInvalidButton", "");
	m_MarkInvalidButton->SetSelected(m_pCampaign->invalid);

	if ( m_MarkInvalidButton->IsSelected() )
	{
		m_bControlsInvalidated = true;
		InvalidateControls();
	}

	LoadControlSettings( "resource/ui/hl2r_campaignlisteditor.res" );
}
//------------------------------------------------------------------------------
// Purpose: Reset all controls back to default.
//------------------------------------------------------------------------------
void CCampaignEditPanel::ResetControls( void )
{
	m_pPrevSettings->Clear();
	m_bControlsInvalidated = false;

	m_pNameEntry->SetText("");

	m_pGameSelectBox->SetText("");
	m_pGameSelectBox->SetActiveItemInvalid();
}
//------------------------------------------------------------------------------
// Purpose: Disable all controls and display editible ones as "INVALID".
//------------------------------------------------------------------------------
void CCampaignEditPanel::InvalidateControls()
{
	char entrytext[256];
	m_pNameEntry->GetText(entrytext, 256);

	// Don't remeber our previous controls if we're invalidated from the start.
	if ( !m_bControlsInvalidated )
	{
		m_pPrevSettings->SetString("name", entrytext);
		m_pPrevSettings->SetInt("game", m_pGameSelectBox->GetActiveItem());
	}

	m_pNameEntry->SetText("#hl2r_list_invalid_label");
	m_pNameEntry->SetEnabled(false);

	m_pGameSelectBox->SetText("#hl2r_list_invalid_label");
	m_pGameSelectBox->SetActiveItemInvalid();
	m_pGameSelectBox->SetEnabled(false);

	m_pResetButton->SetEnabled(false);
}
//------------------------------------------------------------------------------
// Purpose: Disable the invalidated state.
//------------------------------------------------------------------------------
void CCampaignEditPanel::ReinstateControls(void)
{
	if (!m_bControlsInvalidated)
		return;

	m_pNameEntry->SetText(m_pPrevSettings->GetString("name"));

	if ( m_pPrevSettings->GetInt("game", -1) == -1 )
	{
		m_pGameSelectBox->SetActiveItemInvalid();
		m_pGameSelectBox->SetText("");
	}
	else
	{
		m_pGameSelectBox->ActivateItem(m_pPrevSettings->GetInt("game"));
	}

	m_pNameEntry->SetEnabled(true);
	m_pGameSelectBox->SetEnabled(true);
	m_pResetButton->SetEnabled(true);
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignEditPanel::Activate()
{
	BaseClass::Activate();
	input()->SetAppModalSurface(GetVPanel());
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignEditPanel::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		char entrytext[256];
		m_pNameEntry->GetText(entrytext, 256);

		V_strcpy( m_pCampaign->name, (!stricmp(entrytext, "") || m_bControlsInvalidated) ? "undefined" : entrytext );
		m_pCampaign->game = m_pGameSelectBox->GetActiveItem();
		m_pCampaign->invalid = m_MarkInvalidButton->IsSelected();

		GetCampaignDatabase()->WriteListToScript();
		m_pParent->RefreshList();

		Close();
	}
	else if (!stricmp(command, "reset"))
	{
		ResetControls();
	}
	else if (!stricmp(command, "Cancel"))
	{
		Close();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignEditPanel::OnKeyCodeTyped(KeyCode code)
{
	if (code == KEY_ESCAPE)
	{
		Close();
	}
	else
	{
		BaseClass::OnKeyCodeTyped(code);
	}
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignEditPanel::OnCheckButtonChecked(vgui::Panel *panel)
{ 
	if (m_MarkInvalidButton->IsSelected())
	{
		InvalidateControls();
		m_bControlsInvalidated = true;
	}
	else
	{
		ReinstateControls();
		m_bControlsInvalidated = false;
	}
}
*/
//------------------------------------------------------------------------------
// Parent panel
//------------------------------------------------------------------------------
CHL2RCampaignPanel::CHL2RCampaignPanel() : PropertyDialog(FindGameUIChildPanel("BaseGameUIPanel"), "OtherSettings")
{
	SetDeleteSelfOnClose(true);

	SetOKButtonVisible(true);
	SetCancelButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable( false );
		
	SetBounds(0, 0, GetAdjustedSize(PANEL_WIDTH), GetAdjustedSize(PANEL_HEIGHT) );
	MoveToCenterOfScreen();

	SetTitle("#hl2r_campaign_menu_title", true);

	CCampaignListPanel *listpanel = new CCampaignListPanel(this);
	AddPage(listpanel, "#hl2r_campaign_list_title");
}

void CHL2RCampaignPanel::Activate()
{
	BaseClass::Activate();
	EnableApplyButton(false);
}

void CHL2RCampaignPanel::OnThink()
{
	BaseClass::OnThink();
	if (engine->IsPaused() || engine->IsLevelMainMenuBackground() || !engine->IsInGame())
		SetVisible(true);
	else
		SetVisible(false);
}

bool isCampaignPanelActive = false;
void CHL2RCampaignPanel::OnClose()
{
	BaseClass::OnClose();
	isCampaignPanelActive = false;
}

void CHL2RCampaignPanel::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);
	
	BaseClass::OnClose();
	isCampaignPanelActive = false;
}
//------------------------------------------------------------------------------
// Purpose: Console Command
//------------------------------------------------------------------------------
CHL2RCampaignPanel* CampaignPanel = NULL;
CON_COMMAND(OpenHL2RCampaignDialog, "Opens the campaign manager ui")
{
	if (!isCampaignPanelActive)
	{
		CampaignPanel = new CHL2RCampaignPanel();
		isCampaignPanelActive = true;
	}

	CampaignPanel->Activate();
}