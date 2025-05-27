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
	m_SelectedCampaignPanel->SetSelected(NULL);

	LoadControlSettings("resource/ui/hl2r_campaignlist.res");

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
		const char *SelectedItemID = m_ListPanel->GetSelectedItemData()->GetString("id");
		CampaignData_t *pSelectedCampaign = GetCampaignDatabase()->GetCampaignDataFromID(SelectedItemID);
		
		m_SelectedCampaignPanel->SetSelected(pSelectedCampaign);

		m_MountButton->SetEnabled(false);

		if ( Q_stricmp( pSelectedCampaign->name, "undefined" ) && pSelectedCampaign->game != -1 && !pSelectedCampaign->mounted )
			m_MountButton->SetEnabled(true);
	}
	else	
	{
		m_SelectedCampaignPanel->SetSelected(NULL);
	}
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

	m_SelectedCampaignPanel->SetSelected(NULL);

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
	m_ListPanel->RemoveAllSections();

	CreateListColumns();
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
	for (int i = 0; i < GetCampaignDatabase()->GetCampaignCount(); i++ )
	{
		CampaignData_t *pDatabaseCampaign = GetCampaignDatabase()->GetCampaignData(i);

		// Don't do anything if this campaign is not currently not installed.
		if ( !pDatabaseCampaign->installed )
			continue;

		KeyValues *pCampaign = GetCampaignDatabase()->GetKeyValuesFromCampaign(pDatabaseCampaign);

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
		CampaignData_t *pCampaign = GetCampaignDatabase()->GetCampaignDataFromID(szCampaignID);

		if ( pCampaign->startingmap == -1 )
		{
			MessageBox *box = new MessageBox("#hl2r_warning_title", "#hl2r_mountwarning", this);
			box->DoModal();

			BaseClass::OnCommand(pcCommand);
			return;
		}

		// Create a disclaimer box confirming if this is our intended action.
		QueryBox *box = new QueryBox("#hl2r_mountdisclaimer_title", "#hl2r_mountdisclaimer", this);
		box->SetOKButtonText("#hl2r_mountdisclaimer_accept");
		box->SetOKCommand(new KeyValues("Command", "command", "mountconfirmed"));
		box->AddActionSignalTarget(this);
		box->DoModal();
	}
	if (!stricmp(pcCommand, "mountconfirmed"))
	{
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
void CCampaignListPanel::HandleCampaignMount( const char *szCampaignID )
{

switch (GetCampaignDatabase()->MountCampaign(szCampaignID))
	{
	case SUCESSFULLY_MOUNTED:
		{
			MessageBox *box = new MessageBox("#hl2r_mountsucess_title", "#hl2r_mountsucess_text", this);
			box->SetOKButtonText("#hl2r_quit_title");
			box->SetCommand("quitcommand");
			box->AddActionSignalTarget(this);
			box->DoModal();
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

	case FAILED_TO_TRANSFER_GAMEINFO:
		{
			MessageBox *box = new MessageBox("#hl2r_error_title", "#hl2r_mounterror_3", this);
			box->DoModal();
			break;
		}
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

CON_COMMAND(StartWorkshopCampaign, "Starts the currently mounted campaign")
{
	CampaignData_t *pMountedCampaign = GetCampaignDatabase()->GetMountedCampaign();
	if ( !pMountedCampaign )
	{
		MessageBox *box = new MessageBox("#hl2r_warning_title", "#hl2r_begincampaign_error", FindGameUIChildPanel("BaseGameUIPanel"));
		box->DoModal();

		return;
	}
	int szMapIndex = pMountedCampaign->startingmap;

	char szMapCommand[64];
	V_sprintf_safe(szMapCommand, "map %s", GetCampaignDatabase()->GetMountedCampaign()->maplist[szMapIndex] );

	engine->ClientCmd(szMapCommand);
}