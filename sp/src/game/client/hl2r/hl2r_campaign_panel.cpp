#include "cbase.h"
using namespace vgui;
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/QueryBox.h>
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/ToggleButton.h"
#include <vgui/ISurface.h>
#include "vgui/ILocalize.h"
#include "ienginevgui.h"
#include "hl2r_campaign_panel.h"
#include "hl2r_campaign_database.h"

// TODO:

// Give a return time of how long it took to mount a campaign on the notification panel. "Finished in:"

// Make mount fails less catostrophic by reversing whatever the previous function did if it failed.

// Finish error handling.

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
	m_PrevSortType = GetCampaignDatabase()->GetSortType();
	m_PrevSortDir = GetCampaignDatabase()->GetSortDir();

	m_ListPanel = new SectionedListPanel(this, "listpanel_campaignlist");

	m_MountButton = new Button(this, "MountButton", "");
	m_MountButton->SetCommand("mountitem");

	m_CampaignScanButton = new Button(this, "CampaignScanButton", "");
	m_CampaignScanButton->SetCommand("scancampaigns");

	m_SelectedCampaignPanel = new CSelectedCampaignPanel(this, "SelectedCampaignPanel" );
	m_SelectedCampaignPanel->SetSelected(CAMPAIGN_HANDLE_INVALID);

	LoadControlSettings(RES_CAMPAIGN_LIST_PANEL);

	CreateListColumns();
	CreateList();
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::ItemSelected(int itemID)
{
	if (itemID != -1)
	{
		CCampaignDatabase *database = GetCampaignDatabase();

		const char *campaignID = m_ListPanel->GetSelectedItemData()->GetString("id");
		m_SelectedCampaignPanel->SetSelected( database->GetCampaignHandleFromID(campaignID) );

		// Don't enable mounting if we're already mounted!
		m_MountButton->SetEnabled( !database->GetCampaignFromID(campaignID)->mounted );
	}
	else	
	{
		m_SelectedCampaignPanel->SetSelected(CAMPAIGN_HANDLE_INVALID);
		m_MountButton->SetEnabled(false);
	}
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::ItemDoubleLeftClick(int itemID)
{
	const char *pCampaignID = m_ListPanel->GetSelectedItemData()->GetString("id");
	CreateMountDisclaimer(pCampaignID);
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::ColumnSelected()
{
	int i = m_ListPanelSortHeader->GetSelectedColumn();
	if ( i == -1 )
		return;

	ESortType type;
	ESortDirection dir = DECENDING_ORDER;

	// Get the sort type from the column we selected.
	switch ( i )
	{
	case 0:
		type = BY_NAME;
		break;

	case 1:
		type = BY_SIZE;
		break;

	case 2:
		type = BY_DATE;
		break;

	default:
		type = BY_NAME;
		break;
	}

	// If the same column is selected twice, invert the sort direction.
	if ( type == m_PrevSortType )
	{
		if ( m_PrevSortDir == dir )
		{
			dir = ASCENDING_ORDER;
		}
	}

	m_PrevSortType = type;
	m_PrevSortDir = dir;

	m_SelectedCampaignPanel->SetSelected(CAMPAIGN_HANDLE_INVALID);

	GetCampaignDatabase()->SortCampaignList(type, dir);
	RefreshList(false);
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::RefreshList(bool bPreserveSelected)
{
	m_PrevSortType = GetCampaignDatabase()->GetSortType();
	m_PrevSortDir = GetCampaignDatabase()->GetSortDir();

	bool bHasItemSelected = m_ListPanel->GetSelectedItem() != -1;
	char SelectedItemID[CAMPAIGN_ID_LENGTH];

	if ( bPreserveSelected && bHasItemSelected )
		V_strcpy(SelectedItemID, m_ListPanel->GetSelectedItemData()->GetString("id"));

	m_ListPanel->RemoveAll();
	//m_ListPanel->RemoveAllSections();

	//CreateListColumns();
	CreateList();

	if ( !bPreserveSelected && !bHasItemSelected )
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
#define NAME_WIDTH	224
#define SIZE_WIDTH	72
#define DATE_WIDTH	148
void CCampaignListPanel::CreateListColumns(void)
{
	m_ListPanelSortHeader = new SelectableColumnHeader(this, m_ListPanel, "header", 0);
	m_ListPanel->AddSection(0, m_ListPanelSortHeader);

	m_ListPanel->AddColumnToSection(0, "label", "#hl2r_label_label",	SectionedListPanel::COLUMN_BRIGHT, NAME_WIDTH);
	m_ListPanel->AddColumnToSection(0, "size",	"#hl2r_filesize_label", SectionedListPanel::COLUMN_BRIGHT, SIZE_WIDTH);
	m_ListPanel->AddColumnToSection(0, "date",	"#hl2r_date_label",		SectionedListPanel::COLUMN_BRIGHT, DATE_WIDTH);

	int sorttype = GetCampaignDatabase()->GetSortType();
	m_ListPanelSortHeader->SetSelectedColumn(sorttype, GetCampaignDatabase()->GetSortDir() == DECENDING_ORDER);
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
static const char *FixupDateChar( const char *pIn )
{
	char output[8];
	V_strcpy_safe(output, pIn);

	char substr[8];
	V_StrSubst(output, "!", "", substr, 8 );

	char *pOut = new char[8];
	V_memcpy( pOut, substr, 8 );

	return pOut;
}
//------------------------------------------------------------------------------
// Purpose: Fill our campaign list with campaigns from the database.
//------------------------------------------------------------------------------
void CCampaignListPanel::CreateList(void)
{
	CCampaignDatabase *database = GetCampaignDatabase();
	for (CAMPAIGN_HANDLE i = 0; i < database->GetCampaignCount(); i++ )
	{
		// Don't do anything if this campaign is not currently not installed.
		if ( !database->GetCampaign(i)->installed )
			continue;

		KeyValues *pCampaign = database->GetKeyValuesFromCampaign( i );

		// Set our filesize label.
		char szFilesize[CAMPAIGN_FILESIZE_LENGTH + 2];
		V_sprintf_safe(szFilesize, "%s MB", pCampaign->GetString("filesize") );
		pCampaign->SetString("size", szFilesize );

		// Set our date label. (This is really ugly I know)
		char szDate[64];
		KeyValues *pDateTable = pCampaign->FindKey("datetable");

		V_sprintf_safe(szDate, "%s/%s/%s : %s:%s %s", 
		FixupDateChar(pDateTable->GetString("month")),
		FixupDateChar(pDateTable->GetString("day")),
		FixupDateChar(pDateTable->GetString("year")),
		FixupDateChar(pDateTable->GetString("hour")),
		FixupDateChar(pDateTable->GetString("minute")),
		FixupDateChar(pDateTable->GetString("period")));
		pCampaign->SetString("date", szDate );

		// Get our campaign's name. If not defined yet, use a label from our resource file to list it as such.
		const char *pCampaignName = pCampaign->GetString("Name");
		pCampaignName = !Q_stricmp(pCampaignName, "undefined" ) ? "#hl2r_undefined_label" : pCampaignName;

		// Is this the currently mounted campaign?
		if ( pCampaign->GetBool("mounted") )
		{
			// Get our "[Mounted]" suffix.
			char szMountedString[64];
			wchar_t *pLocalizedMountedLabel = g_pVGuiLocalize->Find("hl2r_mounted_label");
			g_pVGuiLocalize->ConvertUnicodeToANSI( pLocalizedMountedLabel, szMountedString, sizeof(szMountedString) );

			// Add it to the end of our campaign's name.
			char szNameMounted[CAMPAIGN_NAME_LENGTH];
			V_sprintf_safe(szNameMounted, "%s%s", pCampaignName, szMountedString );

			pCampaign->SetString("label", szNameMounted);

			int itemID = m_ListPanel->AddItem(0, pCampaign);

			// Make it have a distinct color!
			m_ListPanel->SetItemFgColor(itemID, COLOR_GREEN ); // FIX: MAGIC COLOR!!!
		}
		else
		{
			pCampaign->SetString("label", pCampaignName);
			int itemID = m_ListPanel->AddItem(0, pCampaign);

			// Make this campaign greyed-out if it can't currently be mounted.
			bool bMountable = ( Q_stricmp( pCampaign->GetString("Name"), "undefined" ) && pCampaign->GetInt("Game") != -1 );
			if ( !bMountable )
				m_ListPanel->SetItemFgColor(itemID, Color( 160, 160, 160, 255 )); // FIX: MAGIC COLOR!!!
		}
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
			MessageBox *box = new MessageBox("#hl2r_error_title", "#hl2r_missing_hlextract", this);
			box->DoModal();

			BaseClass::OnCommand(pcCommand);
			return;
		}
	}

	if (!stricmp(pcCommand, "mountitem"))
	{
		const char* szCampaignID = m_ListPanel->GetSelectedItemData()->GetString("id");
		CreateMountDisclaimer(szCampaignID);
	}
	if (!stricmp(pcCommand, "mountconfirmed"))
	{
		engine->ClientCmd("disconnect");

		const char* szCampaignID = m_ListPanel->GetSelectedItemData()->GetString("id");
		HandleCampaignMount(szCampaignID);
	}
	if (!stricmp(pcCommand, "quitcommand"))
	{
		engine->ClientCmd("quit");
	}


	if (!stricmp(pcCommand, "scancampaigns"))
	{
		// Create a disclaimer box confirming if this is our intended action.
		QueryBox *box = new QueryBox("#hl2r_scandisclaimer_title", "#hl2r_scandisclaimer_text", this);
		box->SetOKButtonText("#hl2r_scandisclaimer_accept");
		box->SetOKCommand(new KeyValues("Command", "command", "scanconfirmed"));
		box->AddActionSignalTarget(this);
		box->DoModal();
	}
	if (!stricmp(pcCommand, "scanconfirmed"))
	{
		HandleCampaignScan();
	}

	BaseClass::OnCommand(pcCommand);
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCampaignListPanel::CreateMountDisclaimer( const char *pID )
{
	CCampaignDatabase *database = GetCampaignDatabase();
	if ( !Q_stricmp( database->GetCampaignFromID(pID)->name, "undefined" ) || database->GetCampaignFromID(pID)->game == -1)
	{
		MessageBox *box = new MessageBox("#hl2r_warning_title", "#hl2r_mountwarning_1", this);
		box->DoModal();

		return;
	}

	if ( database->GetCampaignFromID(pID)->startingmap == -1 )
	{
		MessageBox *box = new MessageBox("#hl2r_warning_title", "#hl2r_mountwarning_2", this);
		box->DoModal();

		return;
	}

	// Create a disclaimer box confirming if this is our intended action.
	QueryBox *box = new QueryBox("#hl2r_mountdisclaimer_title", "#hl2r_mountdisclaimer", this);
	box->SetOKButtonText("#hl2r_mountdisclaimer_accept");
	box->SetOKCommand(new KeyValues("Command", "command", "mountconfirmed"));
	box->AddActionSignalTarget(this);
	box->DoModal();
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
	/*		MessageBox *box = new MessageBox("#hl2r_mountsucess_title", "#hl2r_mountsucess_text", this);
			box->SetOKButtonText("#hl2r_quit_title");
			box->SetCommand("quitcommand");
			box->AddActionSignalTarget(this);
			box->DoModal();*/
			
			engine->ClientCmd("_restart");
			break;
		}

	case FAILED_TO_EXTRACT_VPK:
		{
			MessageBox *box = new MessageBox("#hl2r_error_title", "#hl2r_mounterror_1", this);
			box->DoModal();
			break;
		}

	case FAILED_TO_MOUNT_FILES:
		{
			MessageBox *box = new MessageBox("#hl2r_error_title", "#hl2r_mounterror_2", this);
			box->DoModal();
			break;
		}

/*	case FAILED_TO_TRANSFER_GAMEINFO:
		{
			MessageBox *box = new MessageBox("#hl2r_error_title", "#hl2r_mounterror_3", this);
			box->DoModal();
			break;
		}

	case FAILED_TO_RETRIEVE_SAVE_FILES:
		{
			MessageBox *box = new QueryBox("#hl2r_error_title", "#hl2r_mounterror_4_a", this);
			box->DoModal();
			break;
		}

	case FAILED_TO_STORE_SAVE_FILES:
		{
			MessageBox *box = new QueryBox("#hl2r_error_title", "#hl2r_mounterror_4_b", this);
			box->DoModal();
			break;
		}*/
	}

	RefreshList();
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
void CSelectedCampaignPanel::SetSelected( CAMPAIGN_HANDLE campaign )
{
	if ( GetActivePage()->GetName() == m_EditPanel->GetName() )
	{
		GetActivePage()->SetAlpha(0);
		GetAnimationController()->RunAnimationCommand(GetActivePage(), "Alpha", 255.0f, 0.15, 0.15, AnimationController::INTERPOLATOR_LINEAR);
	}

	m_BrowserPanel->SetSelected(campaign);
	m_EditPanel->SetCampaign(campaign);
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
		
	SetBounds(0, 0, GetAdjustedSize(HL2R_PANEL_WIDTH), GetAdjustedSize(HL2R_PANEL_HEIGHT) );
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
// Purpose:
//------------------------------------------------------------------------------
//ConVar hl2r_campaign_launcher_show_disclaimer( "hl2r_campaign_launcher_show_disclaimer", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE );

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

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CON_COMMAND(StartWorkshopCampaign, "Starts the currently mounted campaign")
{
	CCampaignDatabase *database = GetCampaignDatabase();

	CAMPAIGN_HANDLE hMountedCampaign = GetCampaignDatabase()->GetMountedCampaignHandle();
	if ( !database->ValidCampaign(hMountedCampaign) )
	{
		MessageBox *box = new MessageBox("#hl2r_warning_title", "#hl2r_begincampaign_error", FindGameUIChildPanel("BaseGameUIPanel"));
		box->DoModal();

		return;
	}
	int szMapIndex = database->GetMountedCampaign()->startingmap;

	char szMapCommand[64];
	V_sprintf_safe(szMapCommand, "map %s", database->GetMountedCampaign()->maplist[szMapIndex] );

	engine->ClientCmd(szMapCommand);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

ConVar hl2r_options_disable_disclaimer( "hl2r_options_disable_disclaimer", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE );
CON_COMMAND(Hl2rOpenOptionsDialog, "Opens the options panel")
{
	char szCommand[32];
	V_strcpy(szCommand, "gamemenucommand OpenOptionsDialog");

	char szConVar[32];
	V_strcpy(szConVar, "hl2r_options_disable_disclaimer");

	ConVarRef var(szConVar);
	if ( !var.GetBool() )
	{
		CommandDisclaimerBox *disclaimerbox = new CommandDisclaimerBox("NOTICE", "HL2R is built off an older version of hl2 so not everything will be functional", 
			szCommand, szConVar, FindGameUIChildPanel("BaseGameUIPanel"));

		disclaimerbox->SetCancelButtonText("Don't show this again");
		disclaimerbox->DoModal();
	}
	else
	{
		engine->ClientCmd("gamemenucommand OpenOptionsDialog");
	}
}
