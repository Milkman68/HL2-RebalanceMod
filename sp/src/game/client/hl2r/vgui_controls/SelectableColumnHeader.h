#ifndef SELECTABLECOLUMNHEADER
#define SELECTABLECOLUMNHEADER
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/CheckButton.h>
#include <vgui_controls/SectionedListPanel.h>

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
#endif