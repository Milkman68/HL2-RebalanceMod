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
// Gameplay panel
//------------------------------------------------------------------------------

GenericElement_t game_Elements[] =
{
	{ 8 },
	//						[Name]						[ConVar]						[Parameters list]
	{ TYPE_CVAR_CHECKBOX,	"MirrorModeButton",			"r_mirrored",					{"0 1"} },
	{ TYPE_CVAR_CHECKBOX,	"Ep2FlashlightButton",		"hl2r_episodic_flashlight",		{"0 1"} },

	// Auto Reload
	{ TYPE_CVAR_CHECKBOX,	"EnableAutoReloadButton",	"sk_allow_auto_reload",			{"0 1"} },
	{ TYPE_CVAR_TICKSLIDER, "AutoreloadTimeSlider",		"sk_auto_reload_time",			{"58",		"20",		"3",		"60"} },

	// Autoaim
	{ TYPE_CVAR_CHECKBOX,	"EnableAutoaimButton",		"sk_allow_autoaim",				{"0 1"} },

	{ TYPE_CVAR_TICKSLIDER, "AutoaimScaleSlider",		"autoaim_viewcorrection_scale",	{"5",		"5",		"1",		"5"}  },
	{ TYPE_CVAR_TICKSLIDER, "AutoaimSpeedSlider",		"autoaim_viewcorrection_speed",	{"5",		"5",		"500",		"3000"} },
	{ TYPE_CVAR_CHECKBOX,	"AutoaimCrosshairButton",	"hud_draw_active_reticle",		{"0 1"} },
};


class CSubGamePanel : public CSubPanel
{
	DECLARE_CLASS_SIMPLE(CSubGamePanel, CSubPanel);
	
public:
	CSubGamePanel( vgui::PropertyDialog *parent, GenericElement_t *pElementList ) 
	: CSubPanel(parent, pElementList)
	{
	}

	virtual void InitElements( Panel *parent, const char *resource )
	{
		m_pAutoaimBoxDivider = new Divider(this, "AutoaimElementsDivider");
		m_pAutoReloadBoxDivider = new Divider(this, "AutoReloadElementsDivider");

		m_pAutoreloadTimeLabel = new Label(this, "AutoreloadTimeLabel", "");

		BaseClass::InitElements(parent, resource);
	}

	#define ENABLE_ELEMENT(type, name, enabled) GET_ELEMENT_PANEL(type, name)->SetEnabled(enabled);
	virtual void OnControlModified()
	{
		BaseClass::OnControlModified();

		// Auto-aim
		CConvarCheckButtonElement *pAutoaimButtonElement;
		GET_ELEMENT(pAutoaimButtonElement, "EnableAutoaimButton");

		bool bAutoAimEnabled = pAutoaimButtonElement->GetButton()->IsSelected();

		ENABLE_ELEMENT(CConvarTickSliderElement, "AutoaimScaleSlider", bAutoAimEnabled);
		ENABLE_ELEMENT(CConvarTickSliderElement, "AutoaimSpeedSlider", bAutoAimEnabled);
		ENABLE_ELEMENT(CConvarCheckButtonElement, "AutoaimCrosshairButton", bAutoAimEnabled);

		// Auto-reload
		CConvarCheckButtonElement *pAutoReloadElement;
		GET_ELEMENT(pAutoReloadElement, "EnableAutoReloadButton");

		bool bAutoReloadEnabled = pAutoReloadElement->GetButton()->IsSelected();
		ENABLE_ELEMENT(CConvarTickSliderElement, "AutoreloadTimeSlider", bAutoReloadEnabled);

		CConvarTickSliderElement *pAutoreloadSlider;
		GET_ELEMENT(pAutoreloadSlider, "AutoreloadTimeSlider");

		// Auto-reload label
		int numticks = pAutoreloadSlider->numticks;
		float min = pAutoreloadSlider->min;
		float max = pAutoreloadSlider->max;

		int iValue = RemapVal(pAutoreloadSlider->GetTickSlider()->GetValue(), 0.0f, numticks - 1, min, max);

		char szAutoReloadValue[8];
		V_sprintf_safe( szAutoReloadValue, "%i", iValue);
		m_pAutoreloadTimeLabel->SetText(szAutoReloadValue);
	}
private:
	void EnableElementPanel(const char *pName, bool bEnable)
	{
		CControlElement *element = NULL;
		element = GetElement(element, pName);

		element->GetPanel()->SetEnabled(false);
	}

private:
	Divider	*m_pAutoaimBoxDivider;
	Divider	*m_pAutoReloadBoxDivider;

	Label	*m_pAutoreloadTimeLabel;
};

//------------------------------------------------------------------------------
// Player panel
//------------------------------------------------------------------------------

GenericElement_t visual_Elements[] =
{
	{ 7 },
	//						[Name]						[ConVar]						[Parameters list]
	{ TYPE_CVAR_CHECKBOX,	"WeaponHintsButton",		"hl2r_hudhints",				{"0 1"} },
	{ TYPE_CVAR_CHECKBOX,	"QuickInfoButton",			"hud_quickinfo",				{"0 1"} },
	{ TYPE_CVAR_CHECKBOX,	"OldCrosshairsButton",		"hl2r_old_crosshair",			{"1 0"} },
	{ TYPE_CVAR_TICKSLIDER, "ViewRollSlider",			"hl2r_rollangle",				{"11",	"11",	"0",	"10"}  },
	{ TYPE_CVAR_TICKSLIDER, "ViewModelFovSlider",		"Viewmodel_fov",				{"37",	"37",	"54",	"90"}  },
	{ TYPE_CVAR_BOXBUTTON,	"DynamicLightBoxButton",	"hl2r_dynamic_light_level",		{"Default Minimal Diabled", "0 1 2"} },
	{ TYPE_CVAR_CHECKBOX,	"BulletTracerButton",		"hl2r_bullet_tracer_freq",		{"2 1"}  },
};

//------------------------------------------------------------------------------
// Game panel
//------------------------------------------------------------------------------
/*
GenericElement_t fx_Elements[] =
{
	{ 2 },
	//						[Name]						[ConVar]						[Parameters list]
	{ TYPE_CVAR_BOXBUTTON,	"DynamicLightBoxButton",	"hl2r_dynamic_light_level",		{"Default Minimal Diabled", "2 1 0"} },
	{ TYPE_CVAR_TICKSLIDER, "BulletTracerSlider",		"hl2r_bullet_tracer_freq",		{"4",	"4",	"0",	"4"}  },
};*/
#endif