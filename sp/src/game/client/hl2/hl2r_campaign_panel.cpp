#include "cbase.h"
using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/ScrollableEditablePanel.h>
#include "ienginevgui.h"
#include <filesystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/Button.h>
#include "hl2r_campaign_panel.h"
#include "hl2_gamerules.h"
#include "gamerules.h"
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Checkbutton.h>
#include "vgui/ISystem.h"
#include "vgui/Iinput.h"
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/MessageBox.h>
#include "hl2r_campaign_database.h"

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

	m_EditButton = new Button(this, "EditButton", "");
	m_EditButton->SetCommand("edititem");

	m_MountButton = new Button(this, "MountButton", "");
	m_MountButton->SetCommand("mountitem");

	m_OpenInBrowserButton = new Button(this, "OpenInBrowserButton", "");
	m_OpenInBrowserButton->SetCommand("openiteminbrowser");

	m_EditButton->SetEnabled(false);
	m_MountButton->SetEnabled(false);
	m_OpenInBrowserButton->SetEnabled(false);

	m_ViewInvalidButton = new CheckButton(this, "ViewInvalidButton", "");
	m_ViewInvalidButton->SetSelected(false);

	LoadControlSettings("resource/ui/hl2r_campaignlist.res");

	CreateListColunms();
	CreateList();
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::ItemSelected(int itemID)
{
	bool bHasItemSelected = m_ListPanel->IsItemIDValid(itemID);
	bool bCanItemBeMounted = m_ListPanel->GetItemSection(itemID) == 1;

	m_MountButton->SetEnabled(bHasItemSelected && bCanItemBeMounted);
	m_EditButton->SetEnabled(bHasItemSelected);
	m_OpenInBrowserButton->SetEnabled(bHasItemSelected);
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::RefreshList(void)
{
	int iSelected = m_ListPanel->GetSelectedItem();

	m_ListPanel->RemoveAll();
	m_ListPanel->RemoveAllSections();

	CreateListColunms();
	CreateList();

	m_ListPanel->SetSelectedItem(iSelected);
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
#define NAME_WIDTH	256
#define ID_WIDTH	112
#define GAME_WIDTH	84
void CCampaignListPanel::CreateListColunms(void)
{
	m_ListPanel->AddSection(0, "column0");
	m_ListPanel->AddColumnToSection(0, "namelabel", "#hl2r_list_mounted", SectionedListPanel::COLUMN_BRIGHT, NAME_WIDTH);
	m_ListPanel->AddColumnToSection(0, "id", "#hl2r_list_id", SectionedListPanel::COLUMN_BRIGHT, ID_WIDTH);
	m_ListPanel->AddColumnToSection(0, "gamelabel", "#hl2r_list_gametype", SectionedListPanel::COLUMN_BRIGHT, GAME_WIDTH);

	m_ListPanel->AddSection(1, "column1");
	m_ListPanel->AddColumnToSection(1, "namelabel", "#hl2r_list_unmounted", SectionedListPanel::COLUMN_BRIGHT, NAME_WIDTH);
	m_ListPanel->AddColumnToSection(1, "id", "#hl2r_list_id", SectionedListPanel::COLUMN_BRIGHT, ID_WIDTH);
	m_ListPanel->AddColumnToSection(1, "gamelabel", "#hl2r_list_gametype", SectionedListPanel::COLUMN_BRIGHT, GAME_WIDTH);

	m_ListPanel->AddSection(2, "column2");
	m_ListPanel->AddColumnToSection(2, "namelabel", "#hl2r_list_undefined", SectionedListPanel::COLUMN_BRIGHT, NAME_WIDTH);
	m_ListPanel->AddColumnToSection(2, "id", "#hl2r_list_id", SectionedListPanel::COLUMN_BRIGHT, ID_WIDTH);
	m_ListPanel->AddColumnToSection(2, "gamelabel", "#hl2r_list_gametype", SectionedListPanel::COLUMN_BRIGHT, GAME_WIDTH);

	if ( !m_ViewInvalidButton->IsSelected() )
		return;

	m_ListPanel->AddSection(3, "column3");
	m_ListPanel->AddColumnToSection(3, "namelabel", "#hl2r_list_invalid", SectionedListPanel::COLUMN_BRIGHT, NAME_WIDTH);
	m_ListPanel->AddColumnToSection(3, "id", "#hl2r_list_id", SectionedListPanel::COLUMN_BRIGHT, ID_WIDTH);
	m_ListPanel->AddColumnToSection(3, "gamelabel", "#hl2r_list_gametype", SectionedListPanel::COLUMN_BRIGHT, GAME_WIDTH);
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::CreateList(void)
{
	for (int i = 0; i < GetCampaignDatabase()->GetCampaignCount(); i++ )
	{
		KeyValues *pCampaign = GetCampaignDatabase()->GetKeyValuesFromData(i);

		// Don't do anything if this shouldn't be visible.
		if (!pCampaign->GetBool("visible"))
			continue;

		// If invalid, add to the invalid column only if enabled.
		if (pCampaign->GetBool("invalid") )
		{
			if ( !m_ViewInvalidButton->IsSelected() )
				continue;

			pCampaign->SetString("gamelabel", "#hl2r_list_invalid_label");
			pCampaign->SetString("namelabel", "#hl2r_list_invalid_label");

			m_ListPanel->AddItem(3, pCampaign);
			continue;
		}

		const char *pCampaignName = pCampaign->GetString("Name");
		pCampaignName = !Q_stricmp(pCampaignName, "undefined" ) ? "#hl2r_list_undefined_label" : pCampaignName;

		pCampaign->SetString("namelabel", pCampaignName);

		switch (pCampaign->GetInt("Game"))
		{
		case GAME_INVALID:
			pCampaign->SetString("gamelabel", "#hl2r_list_undefined_label");
			break;

		case GAME_HL2:
			pCampaign->SetString("gamelabel", "Half-Life 2");
			break;

		case GAME_EPISODE_1:
			pCampaign->SetString("gamelabel", "Episode 1");
			break;

		case GAME_EPISODE_2:
			pCampaign->SetString("gamelabel", "Episode 2");
			break;
		}

		if ( !Q_stricmp(pCampaignName, "undefined" ) || pCampaign->GetInt("Game") == GAME_INVALID)
		{
			// Add to the "undefined" column
			m_ListPanel->AddItem(2, pCampaign);
		}
		else
		{
			int column = pCampaign->GetBool("mounted") ? 0 : 1;
			m_ListPanel->AddItem(column, pCampaign);
		}
	}
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::OnCommand(const char* pcCommand)
{
	if (!stricmp(pcCommand, "edititem"))
	{
		// Create a temporary editor panel for the campaign we selected.
		const char *SelectedItemID = m_ListPanel->GetSelectedItemData()->GetString("id");
		CampaignData_t *pSelectedCampaign = GetCampaignDatabase()->GetCampaignDataFromID(SelectedItemID);

		CCampaignEditPanel *pEditPanel = new CCampaignEditPanel(this, pSelectedCampaign);

		pEditPanel->Activate();
		pEditPanel->AddActionSignalTarget(this);
	}
	else if (!stricmp(pcCommand, "openiteminbrowser"))
	{
		// Get the ID of the selected campaign and parse it into a url that can be opened.
		const char* szCampaignID = m_ListPanel->GetSelectedItemData()->GetString("id");

		char szURL[128];
		Q_snprintf(szURL, sizeof(szURL), "https://steamcommunity.com/sharedfiles/filedetails?id=%s", szCampaignID );

		system()->ShellExecute("open", szURL);
	}
	else if (!stricmp(pcCommand, "mountitem"))
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

		switch (GetCampaignDatabase()->MountCampaign(szCampaignID))
		{
		case SUCESSFULLY_MOUNTED:
			{
			//	engine->ClientCmd("Quit");
			}
			break;

		case FAILED_TO_UNPACK_VPK:
			{
				MessageBox *box = new MessageBox("#hl2r_list_mounterror_title", "#hl2r_list_mounterror_1", this);
				box->DoModal();
				break;
			}

		default:
			break;
		}
	}

	BaseClass::OnCommand(pcCommand);
}
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