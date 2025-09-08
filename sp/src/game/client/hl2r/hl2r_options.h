#ifndef hl2r_options_h
#define hl2r_options_h
#ifdef _WIN32
#pragma once
#endif

#include "hl2r_baseoptions.h"
#include <vgui_controls/Divider.h>

//------------------------------------------------------------------------------
// Parent panel
//------------------------------------------------------------------------------
class CHL2RMenu : public vgui::PropertyDialog
{
	DECLARE_CLASS_SIMPLE(CHL2RMenu, vgui::PropertyDialog);

public:
	CHL2RMenu();
	~CHL2RMenu() {}
	
	virtual void Activate();
	virtual void OnThink();
	
	virtual void OnClose();
	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);
};

//------------------------------------------------------------------------------
// Game panel
//------------------------------------------------------------------------------

ControlElement_t game_Elements[] =
{
	{ 8 },
//					[Name]						[Convar]						[Invert]
	{ TYPE_CHECKBOX, "MirrorModeButton",		"r_mirrored",					"false" },
	{ TYPE_CHECKBOX, "Ep2FlashlightButton",		"hl2r_episodic_flashlight",		"false" },

	{ TYPE_CHECKBOX, "AutoaimCrosshairButton",	"hud_draw_active_reticle",		"false" },
	{ TYPE_CHECKBOX, "EnableAutoaimButton",		"sk_allow_autoaim",				"false" },
	{ TYPE_CHECKBOX, "EnableAutoReloadButton",	"sk_allow_auto_reload",			"false" },

//						[Name]						[Convar]						[Numticks] [Numindicators]	[Minval]	[Maxval]
	{ TYPE_TICKSLIDER, "AutoaimScaleSlider",		"autoaim_viewcorrection_scale",		"5",		"5",		"1",		"5",  },
	{ TYPE_TICKSLIDER, "AutoaimSpeedSlider",		"autoaim_viewcorrection_speed",		"5",		"5",		"500",		"3000" },
	{ TYPE_TICKSLIDER, "AutoreloadTimeSlider",		"sk_auto_reload_time",				"20",		"20",		"3",		"60" },
};


class CSubGamePanel : public CSubPanel
{
	DECLARE_CLASS_SIMPLE(CSubGamePanel, CSubPanel);
	
public:
	CSubGamePanel( vgui::Panel* parent ) : CSubPanel(parent) {}
	~CSubGamePanel() {}
	
	virtual void SubPanelInit( void )
	{
		BaseClass::SubPanelInit();

		m_pAutoaimBoxDivider = new Divider(this, "AutoaimElementsDivider");

		m_AutoaimElements.AddToTail(game_Elements[6].elementPanel);
		m_AutoaimElements.AddToTail(game_Elements[7].elementPanel);
		m_AutoaimElements.AddToTail(game_Elements[3].elementPanel);
	}

	virtual void OnControlModified(Panel *panel)
	{
		BaseClass::OnControlModified(panel);

		Button *pEnableAutoaimButton = dynamic_cast<Button *>(game_Elements[4].elementPanel);
		Button *pEnableAutoReloadButton = dynamic_cast<Button *>(game_Elements[5].elementPanel);

		bool bEnableAutoaim = pEnableAutoaimButton->IsSelected();
		bool bEnableAutoReload = pEnableAutoReloadButton->IsSelected();

		for (int i = 0; i < m_AutoaimElements.Size(); i++ )
			m_AutoaimElements[i]->SetEnabled(bEnableAutoaim);

		Panel *pAutoReloadSlider = game_Elements[8].elementPanel;
		pAutoReloadSlider->SetEnabled(bEnableAutoReload);
	}


private:

	Divider	*m_pAutoaimBoxDivider;
	CUtlVector<Panel *> m_AutoaimElements;
};
//------------------------------------------------------------------------------
// Options panel
//------------------------------------------------------------------------------
OptionsPage_t hl2r_OptionsPages[] =
{
	{ "4" },

//	[Name]					[Resource path]							[Element List]		[Additional Height]
	{ "Game",				"resource/ui/hl2r_optionssubgame.res",  game_Elements,			0.0f },
	{ "Player",				"",										NULL,					0.0f },
	{ "Hud",				"",										NULL, 					0.0f },
	{ "Cut and Fun Stuff",	"",										NULL,					0.0f },
};
#endif