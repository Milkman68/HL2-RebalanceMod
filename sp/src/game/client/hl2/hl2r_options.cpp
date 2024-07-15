#include "cbase.h"
using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/ScrollableEditablePanel.h>
#include "ienginevgui.h"
#include "hl2r_options.h"

//------------------------------------------------------------------------------
// Purpose : HL2R's Game options parent panel
//------------------------------------------------------------------------------
CGameHL2RBasePanel::CGameHL2RBasePanel(vgui::Panel* parent, vgui::EditablePanel* child) : ScrollableEditablePanel(parent, child, NULL){}

//------------------------------------------------------------------------------
// Purpose : We need this because the main panel sends the apply message to here 
// instead of the panel parented to this one. So we need to send it manually to the child panel.
//------------------------------------------------------------------------------
void CGameHL2RBasePanel::OnApplyChanges()
{
	ipanel()->SendMessage(GetChild(0)->GetVPanel(), new KeyValues("ApplyChanges"), GetVPanel());
}
//------------------------------------------------------------------------------
// Purpose : HL2R's Game options page
//------------------------------------------------------------------------------
CSubOptionsGameHL2R::CSubOptionsGameHL2R(vgui::Panel* parent) : EditablePanel(parent, NULL)
{
 	CheckButtonRef.InitCheckButtons( this, G_CheckButtons, ARRAYSIZE(G_CheckButtons) );
	SetBounds(0, 0, 422, 775);
	
	toggleAllButton = new Button(this, "toggleAllButton", "");
	toggleAllButton->SetCommand("toggleall");

	LoadControlSettings("resource/ui/hl2r_optionssubgame.res");
}

void CSubOptionsGameHL2R::OnApplyChanges()
{
	CheckButtonRef.UpdateConVars( this, G_CheckButtons, ARRAYSIZE(G_CheckButtons) );
}

void CSubOptionsGameHL2R::OnCheckButtonChecked(Panel* panel)
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

void CSubOptionsGameHL2R::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);
	if (!Q_stricmp(pcCommand, "toggleall"))
	{
		bool bState = true;
		
		for ( int i = 0; i < ARRAYSIZE(G_CheckButtons); i++ )
		{
			if ( G_CheckButtons[i].Button->IsSelected() )
			{
				bState = false;
				break;
			}
		}
		
		for ( int i = 0; i < ARRAYSIZE(G_CheckButtons); i++ )
		{
			G_CheckButtons[i].Button->SetSelected(bState);
		}
	}
}
//------------------------------------------------------------------------------
// Purpose : HL2R's Misc options page
//------------------------------------------------------------------------------
CSubMiscOptionsHL2R::CSubMiscOptionsHL2R(vgui::Panel* parent) : PropertyPage(parent, NULL)
{
	BoxButtonRef.InitBoxButtons( this, M_BoxButtons, ARRAYSIZE(M_BoxButtons) );
	TickSliderRef.InitTickSliders( this, M_TickSliders, ARRAYSIZE(M_TickSliders) );
	
	LoadControlSettings("resource/ui/hl2r_optionssubmisc.res");
} 

void CSubMiscOptionsHL2R::OnApplyChanges()
{
	BoxButtonRef.UpdateConVars( this, M_BoxButtons, ARRAYSIZE(M_BoxButtons) );
	TickSliderRef.UpdateConVars( this, M_TickSliders, ARRAYSIZE(M_TickSliders) );
}

void CSubMiscOptionsHL2R::OnControlModified()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}
//------------------------------------------------------------------------------
// Purpose : HL2R's Hud options page
//------------------------------------------------------------------------------
CSubHudOptionsHL2R::CSubHudOptionsHL2R(vgui::Panel* parent) : PropertyPage(parent, NULL)
{
	BoxButtonRef.InitBoxButtons( this, H_BoxButtons, ARRAYSIZE(H_BoxButtons) );
	
	LoadControlSettings("resource/ui/hl2r_optionssubhud.res");
} 

void CSubHudOptionsHL2R::OnApplyChanges()
{
	BoxButtonRef.UpdateConVars( this, H_BoxButtons, ARRAYSIZE(H_BoxButtons) );
}

void CSubHudOptionsHL2R::OnControlModified()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}
//------------------------------------------------------------------------------
// Purpose : HL2R's Options panel
//------------------------------------------------------------------------------
CHL2RMenu::CHL2RMenu(vgui::VPANEL parent) : BaseClass(NULL, "HL2RMenu")
{
	SetDeleteSelfOnClose(true);
	SetBounds(0, 0, 422, 590);
	SetSizeable( false );
	MoveToCenterOfScreen();

	SetTitle("#hl2r_options_title", true);
	
	m_pSubOptionsGameHL2R = new CSubOptionsGameHL2R(this);
	
	// Parent the game page to a separate scrollable-panel.
	m_pGameHL2RBasePanel = new CGameHL2RBasePanel(this, m_pSubOptionsGameHL2R);
	AddPage(m_pGameHL2RBasePanel, "#hl2r_options_game");
	
	m_pSubHudOptionsHL2R = new CSubHudOptionsHL2R(this);
	AddPage(m_pSubHudOptionsHL2R, "#hl2r_options_hud");

 	m_pSubMiscOptionsHL2R = new CSubMiscOptionsHL2R(this);
	AddPage(m_pSubMiscOptionsHL2R, "#hl2r_options_misc");
	
	SetApplyButtonVisible(true);
	GetPropertySheet()->SetTabWidth(168);
}

void CHL2RMenu::Activate()
{
	BaseClass::Activate();
	EnableApplyButton(false);
}

void CHL2RMenu::OnThink()
{
	BaseClass::OnThink();
	if (engine->IsPaused() || engine->IsLevelMainMenuBackground() || !engine->IsInGame())
		SetVisible(true);
	else
		SetVisible(false);
}

bool isHL2RActive = false;

void CHL2RMenu::OnClose()
{
	BaseClass::OnClose();
	isHL2RActive = false;
}

CHL2RMenu* HL2RMenu = NULL;

CON_COMMAND(EnableHL2RPanel, "Turns on the HL2R options Panel")
{
	VPANEL gameToolParent = enginevgui->GetPanel(PANEL_CLIENTDLL_TOOLS);
	if (!isHL2RActive)
	{
		HL2RMenu = new CHL2RMenu(gameToolParent);
		isHL2RActive = true;
	}

	HL2RMenu->Activate();
}
