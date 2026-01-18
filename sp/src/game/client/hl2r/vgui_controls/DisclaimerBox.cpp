//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
// This class is a message box that has two buttons, ok and cancel instead of
// just the ok button of a message box. We use a message box class for the ok button
// and implement another button here.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <hl2r/vgui_controls/DisclaimerBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/TextImage.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

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
// Purpose: Constructor
//-----------------------------------------------------------------------------
DisclaimerBox::DisclaimerBox(const char *title, const char *queryText, vgui::Panel *parent) : MessageBox(title, queryText,parent)
{
	SetDeleteSelfOnClose(true);
	m_pDismissButton = new CheckButton(this, "DismissButton", "#hl2r_dismiss");

	m_pOkButton->SetCommand("OK");
	m_pOkCommand = NULL;
	m_pOkButton->SetTabPosition(1);
	m_pDismissButton->SetTabPosition(2);

	m_pCancelButton = new Button(this, NULL, "#MessageBox_Cancel");
	m_pCancelButton->SetCommand( "OnCancel" );
	m_pCancelButton->AddActionSignalTarget(this);
	m_pCancelButton->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
DisclaimerBox::DisclaimerBox(const wchar_t *wszTitle, const wchar_t *wszQueryText,vgui::Panel *parent) : MessageBox(wszTitle, wszQueryText,parent)
{
	SetDeleteSelfOnClose(true);
	m_pDismissButton = new CheckButton(this, "DismissButton", "#hl2r_dismiss");
//	m_pDismissButton->SetCommand("Cancel"); 
	m_pOkButton->SetCommand("OK");
	m_pOkCommand = NULL;

	m_pOkButton->SetTabPosition(1);
	m_pDismissButton->SetTabPosition(2);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
DisclaimerBox::~DisclaimerBox()
{
	delete m_pDismissButton;
}

//-----------------------------------------------------------------------------
// Purpose: Layout the window for drawing 
//-----------------------------------------------------------------------------
void DisclaimerBox::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	wide += x;
	tall += y;

	int oldWide, oldTall;
	int btnWide, btnTall;

	// Button sizes:
	m_pDismissButton->GetSize(oldWide, oldTall);
	m_pDismissButton->GetContentSize(btnWide, btnTall);
	btnWide = max(oldWide, btnWide + GetAdjustedSize(10));
	btnTall = max(oldTall, btnTall + GetAdjustedSize(10));
	m_pDismissButton->SetSize(btnWide, btnTall);

	m_pCancelButton->GetSize(oldWide, oldTall);
	m_pCancelButton->GetContentSize(btnWide, btnTall);
	btnWide = max(oldWide, btnWide + GetAdjustedSize(10));
	btnTall = max(oldTall, btnTall + GetAdjustedSize(10));
	m_pCancelButton->SetSize(btnWide, btnTall);

	// Layout:
	int iMessageXOffset = (wide/2)-(m_pMessageLabel->GetWide()/2);
	m_pOkButton->SetPos(iMessageXOffset + x, tall - m_pOkButton->GetTall() - GetAdjustedSize(15) );
	m_pDismissButton->SetPos(iMessageXOffset + x + GetAdjustedSize(16) + m_pOkButton->GetWide(), tall - m_pDismissButton->GetTall() - GetAdjustedSize(15));

	if ( m_pCancelButton->IsVisible() )
	{
		m_pCancelButton->SetPos(iMessageXOffset + x + GetAdjustedSize(16) + m_pOkButton->GetWide(), tall - m_pCancelButton->GetTall() - GetAdjustedSize(15));
		m_pDismissButton->SetPos((wide/2)+(m_pMessageLabel->GetWide()/2) + x - m_pDismissButton->GetWide(), tall - m_pDismissButton->GetTall() - GetAdjustedSize(15) );
	}

}

//-----------------------------------------------------------------------------
// Purpose: Handles command text from the buttons
//			Deletes self when closed
//-----------------------------------------------------------------------------
void DisclaimerBox::OnCommand(const char *command)
{
	if (!stricmp(command, "OK"))
	{
		if ( m_pDismissButton->IsSelected() )
		{
			ConVarRef ref(m_pDismissConvar);
			if (ref.IsValid()) 
				ref.SetValue(!ref.GetBool());
		}

		OnCommand("Close");

		if ( m_pOkCommand != NULL )
			engine->ClientCmd_Unrestricted(m_pOkCommand);
	}
	
	BaseClass::OnCommand(command);
	
}

//-----------------------------------------------------------------------------
// Purpose: Set the keyvalues to send when ok button is hit
//-----------------------------------------------------------------------------
void DisclaimerBox::SetOKClientCommand(const char *command)
{
	m_pOkCommand = command;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the cancel button text
//-----------------------------------------------------------------------------
void DisclaimerBox::SetDismissButtonText(const char* buttonText)
{
	m_pDismissButton->SetText(buttonText);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the cancel button text
//-----------------------------------------------------------------------------
void DisclaimerBox::SetDismissButtonText(const wchar_t* wszButtonText)
{
	m_pDismissButton->SetText(wszButtonText);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void DisclaimerBox::SetDismissConvar(const char* convar)
{
	m_pDismissConvar = convar;
}

//------------------------------------------------------------------------------
// Purpose : Code for hooking into the base GameUI panel. Credit goes to: 
// https://github.com/HL2RP/HL2RP
//------------------------------------------------------------------------------
#include "ienginevgui.h"
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