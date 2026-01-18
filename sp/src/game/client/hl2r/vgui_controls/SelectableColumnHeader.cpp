#include "cbase.h"
using namespace vgui;
#include "SelectableColumnHeader.h"

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