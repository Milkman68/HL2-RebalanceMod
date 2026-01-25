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

START_ELEMENT_ARRAY( game_Elements, 8 )
	//				[Name]						[ConVar]						[Parameters list]
	ADD_CHECKBOX(	"MirrorModeButton",			"r_mirrored",					"0",	"1" )
	ADD_CHECKBOX(	"Ep2FlashlightButton",		"hl2r_episodic_flashlight",		"0",	"1" )

	// Auto Reload
	ADD_CHECKBOX(	"EnableAutoReloadButton",	"sk_allow_auto_reload",			"0",	"1" )
	ADD_TICKSLIDER( "AutoreloadTimeSlider",		"sk_auto_reload_time",			"58",	"20",	"3",	"60")

	// Autoaim
	ADD_CHECKBOX(	"EnableAutoaimButton",		"sk_allow_autoaim",				"0",	"1" )

	ADD_TICKSLIDER( "AutoaimScaleSlider",		"autoaim_viewcorrection_scale",	"5",	"5",	"1",	"5")
	ADD_TICKSLIDER( "AutoaimSpeedSlider",		"autoaim_viewcorrection_speed",	"5",	"5",	"500",	"3000" )
	ADD_CHECKBOX(	"AutoaimCrosshairButton",	"hud_draw_active_reticle",		"0",	"1" )

END_ELEMENT_ARRAY



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

	virtual void OnControlModified()
	{
		BaseClass::OnControlModified();

		// Auto-aim
		bool bAutoAimEnabled = GetElementPtr( CConvarCheckButtonElement, "EnableAutoaimButton" )->GetButton()->IsSelected();
		GetElementPanel("AutoaimScaleSlider")->SetEnabled(bAutoAimEnabled);
		GetElementPanel("AutoaimSpeedSlider")->SetEnabled(bAutoAimEnabled);
		GetElementPanel("AutoaimCrosshairButton")->SetEnabled(bAutoAimEnabled);

		// Auto-reload
		bool bAutoReloadEnabled = GetElementPtr( CConvarCheckButtonElement, "EnableAutoReloadButton" )->GetButton()->IsSelected();
		GetElementPanel("AutoreloadTimeSlider")->SetEnabled(bAutoReloadEnabled);

		CConvarTickSliderElement *pAutoreloadSlider = GetElementPtr(CConvarTickSliderElement, "AutoreloadTimeSlider" );
		m_pAutoreloadTimeLabel->SetText( VarArgs("%i", (int)pAutoreloadSlider->GetValueFromSliderPos()) );
	}

private:
	Divider	*m_pAutoaimBoxDivider;
	Divider	*m_pAutoReloadBoxDivider;

	Label	*m_pAutoreloadTimeLabel;
};

//------------------------------------------------------------------------------
// Visual panel
//------------------------------------------------------------------------------

START_ELEMENT_ARRAY( visual_Elements, 7 )
	//				[Name]						[ConVar]						[Parameters list]
	ADD_CHECKBOX(	"WeaponHintsButton",		"hl2r_hudhints",				"0",	"1" )
	ADD_CHECKBOX(	"QuickInfoButton",			"hud_quickinfo",				"0",	"1" )
	ADD_CHECKBOX(	"OldCrosshairsButton",		"hl2r_old_crosshair",			"1",	"0" )
	ADD_TICKSLIDER( "ViewRollSlider",			"hl2r_rollangle",				"11",	"11",	"0",	"10" )
	ADD_TICKSLIDER( "ViewModelFovSlider",		"Viewmodel_fov",				"37",	"37",	"54",	"90" )
	ADD_BOXBUTTON(	"DynamicLightBoxButton",	"hl2r_dynamic_light_level",		"#hl2r_default #hl2r_less #hl2r_none", "0 1 2")
	ADD_CHECKBOX(	"BulletTracerButton",		"hl2r_bullet_tracer_freq",		"2",	"1" )

END_ELEMENT_ARRAY


class CSubVisualPanel : public CSubPanel
{
	DECLARE_CLASS_SIMPLE(CSubVisualPanel, CSubPanel);
	
public:
	CSubVisualPanel( vgui::PropertyDialog *parent, GenericElement_t *pElementList ) 
	: CSubPanel(parent, pElementList)
	{
	}

	virtual void InitElements( Panel *parent, const char *resource )
	{
		m_pViewRollAngleLabel = new Label(this, "ViewRollAngleLabel", "");
		m_pViewmodelFovLabel = new Label(this, "ViewmodelFovLabel", "");

		BaseClass::InitElements(parent, resource);
	}

	virtual void OnControlModified()
	{
		BaseClass::OnControlModified();

		// View roll degree value label
		CConvarTickSliderElement *pViewRollSlider = GetElementPtr(CConvarTickSliderElement, "ViewRollSlider" );
		m_pViewRollAngleLabel->SetText( VarArgs("%i", (int)pViewRollSlider->GetValueFromSliderPos()) );

		// Weapon Fov value label
		CConvarTickSliderElement *pViewModelFovSlider = GetElementPtr(CConvarTickSliderElement, "ViewModelFovSlider" );
		m_pViewmodelFovLabel->SetText( VarArgs("%i", (int)pViewModelFovSlider->GetValueFromSliderPos()) );
	}

private:
	Label	*m_pViewRollAngleLabel;
	Label	*m_pViewmodelFovLabel;
};

//------------------------------------------------------------------------------
// Experimental panel
//------------------------------------------------------------------------------

/*GenericElement_t experimental_Elements[] =
{
	{ 3 },
	//						[Name]						[ConVar]						[Parameters list]
	Projected flashes
	health regen

	
};*/
#endif