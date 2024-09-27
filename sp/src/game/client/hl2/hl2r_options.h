#ifndef hl2r_options_h
#define hl2r_options_h
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyPage.h"
#include "hl2r_baseoptions.h"

//------------------------------------------------------------------------------
// Challenge parent panel
//------------------------------------------------------------------------------
class CChallengesHL2RBasePanel : public vgui::ScrollableEditablePanel
{
	DECLARE_CLASS_SIMPLE(CChallengesHL2RBasePanel, vgui::ScrollableEditablePanel);

public:
	CChallengesHL2RBasePanel(vgui::Panel* parent, vgui::EditablePanel* child);
	~CChallengesHL2RBasePanel() {}
	
	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" );
};
//------------------------------------------------------------------------------
// Challenge page
//------------------------------------------------------------------------------
class CSubOptionsChallengesHL2R : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CSubOptionsChallengesHL2R, vgui::EditablePanel);

public:
	CSubOptionsChallengesHL2R(vgui::Panel* parent);
	~CSubOptionsChallengesHL2R() {}

	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" );

protected:
	virtual void OnCommand(const char* pcCommand);
private:
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel);
	
	Button* toggleAllButton;
};
//------------------------------------------------------------------------------
// Challenge page elements
//------------------------------------------------------------------------------
CheckButton* ExplosiveCrabsButton;
//CheckButton* FearMovementButton;
CheckButton* WorseArmorButton;
CheckButton* ShorterSprintButton;
//CheckButton* LessAmmoButton;
CheckButton* EnemyPromotionButton;
CheckButton* RandomWeaponsButton;
CheckButton* ReducedAssistsButton;
CheckButton* SmallerReservesButton;

// List of all CheckButtons:
CheckButton_t C_CheckButtons[] =
{
	{ ExplosiveCrabsButton, "ExplosiveCrabsButton", "hl2r_explosive_crabs", false },
	//{ FearMovementButton, "FearMovementButton", "hl2r_fear_style_movement" },
	{ WorseArmorButton, "WorseArmorButton", "player_old_armor", true },
	{ ShorterSprintButton, "ShorterSprintButton", "hl2r_shorter_sprint", false },
	//{ LessAmmoButton, "LessAmmoButton", "hl2r_less_ammo" },
	{ EnemyPromotionButton, "EnemyPromotionButton", "hl2r_enemy_promotion", false },
	{ RandomWeaponsButton, "RandomWeaponsButton", "hl2r_random_weapons", false },
	{ ReducedAssistsButton, "ReducedAssistsButton", "hl2r_reduced_assists", false },
	{ SmallerReservesButton, "SmallerReservesButton", "hl2r_smaller_reserves", false },
};
//------------------------------------------------------------------------------
// Game page
//------------------------------------------------------------------------------
class CSubGameOptionsHL2R : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CSubGameOptionsHL2R, vgui::PropertyPage);

public:
	CSubGameOptionsHL2R(vgui::Panel* parent);
	~CSubGameOptionsHL2R() {}
	
	virtual void OnApplyChanges();

private:
	MESSAGE_FUNC( OnControlModified, "ControlModified" );
	MESSAGE_FUNC( OnTextChanged, "TextChanged" )
	{
		OnControlModified();
	}
	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data )
	{
		OnControlModified();
	}
};
//------------------------------------------------------------------------------
// Game page elements
//------------------------------------------------------------------------------
ComboBox* EpisodicLightBox;
ComboBox* ZoomToggleBox;

// List of all BoxButtons:
BoxButton_t G_BoxButtons[] =
{
	{ EpisodicLightBox, "EpisodicLightBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_episodic_flashlight", true },
	{ ZoomToggleBox, "ZoomToggleBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_togglezoom", true },
};

Slider* viewRollSlider;
Slider* WeaponFOVSlider;

// List of all TickSliders:
TickSlider_t G_TickSliders[] =
{
	{ viewRollSlider, "viewRollSlider", 0, 10, 10, 1, "hl2r_rollangle"},
	{ WeaponFOVSlider, "WeaponFOVSlider", 54, 90, 12, 3, "viewmodel_fov" },
};
//------------------------------------------------------------------------------
// Visual page
//------------------------------------------------------------------------------
class CSubVisualOptionsHL2R : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CSubVisualOptionsHL2R, vgui::PropertyPage);

public:
	CSubVisualOptionsHL2R(vgui::Panel* parent);
	~CSubVisualOptionsHL2R() {}
	
	virtual void OnApplyChanges();

private:
	MESSAGE_FUNC( OnControlModified, "ControlModified" );
	MESSAGE_FUNC( OnTextChanged, "TextChanged" )
	{
		OnControlModified();
	}
};
//------------------------------------------------------------------------------
// Visual page elements
//------------------------------------------------------------------------------
ComboBox* QuickinfoBox;
ComboBox* MinCrosshairBox;
ComboBox* HudHintBox;
ComboBox* LightDetailBox;
ComboBox* ProjectedMuzzleflashBox;

// List of all BoxButtons:
BoxButton_t V_BoxButtons[] =
{
	{ QuickinfoBox, "QuickinfoBox", { "#hl2r_on", "#hl2r_off" }, "hud_quickinfo", true },
	{ MinCrosshairBox, "MinCrosshairBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_old_crosshair", /* true */ false }, // Renamed to "New Crosshairs" in the options.
	{ HudHintBox, "HudHintBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_hudhints", true },
	{ LightDetailBox, "LightDetailBox", { "#hl2r_default", "#hl2r_less", "#hl2r_none" }, "hl2r_dynamic_light_level", false },
	{ ProjectedMuzzleflashBox, "ProjectedMuzzleflashBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_projected_muzzleflash", true },
};
//------------------------------------------------------------------------------
// Main panel
//------------------------------------------------------------------------------
class CHL2RMenu : public vgui::PropertyDialog
{
	DECLARE_CLASS_SIMPLE(CHL2RMenu, vgui::PropertyDialog);

public:
	CHL2RMenu(vgui::VPANEL parent);
	~CHL2RMenu() {}
	virtual void Activate();

protected:
	virtual void OnThink();
	virtual void OnClose();

private:
	CSubOptionsChallengesHL2R* m_pSubOptionsChallengesHL2R;
	CChallengesHL2RBasePanel* m_pChallengesHL2RBasePanel;
	
	CSubGameOptionsHL2R* m_pSubGameOptionsHL2R;
	
	CSubVisualOptionsHL2R* m_pSubVisualOptionsHL2R;
};

#endif