#ifndef hl2r_options_h
#define hl2r_options_h
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyPage.h"
#include "hl2r_baseoptions.h"

//------------------------------------------------------------------------------
// Game parent panel
//------------------------------------------------------------------------------
class CGameHL2RBasePanel : public vgui::ScrollableEditablePanel
{
	DECLARE_CLASS_SIMPLE(CGameHL2RBasePanel, vgui::ScrollableEditablePanel);

public:
	CGameHL2RBasePanel(vgui::Panel* parent, vgui::EditablePanel* child);
	~CGameHL2RBasePanel() {}
	
	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" );
};
//------------------------------------------------------------------------------
// Game page
//------------------------------------------------------------------------------
class CSubOptionsGameHL2R : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CSubOptionsGameHL2R, vgui::EditablePanel);

public:
	CSubOptionsGameHL2R(vgui::Panel* parent);
	~CSubOptionsGameHL2R() {}

	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" );

protected:
	virtual void OnCommand(const char* pcCommand);
private:
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel);
	
	Button* toggleAllButton;
};
//------------------------------------------------------------------------------
// Game page elements
//------------------------------------------------------------------------------
CheckButton* ExplosiveCrabsButton;
CheckButton* FearMovementButton;
CheckButton* ShorterSprintButton;
CheckButton* LessAmmoButton;
CheckButton* EnemyPromotionButton;
CheckButton* RandomWeaponsButton;
CheckButton* ReducedAssistsButton;
CheckButton* SmallerReservesButton;

// List of all CheckButtons:
CheckButton_t G_CheckButtons[] =
{
	{ ExplosiveCrabsButton, "ExplosiveCrabsButton", "hl2r_explosive_crabs" },
	{ FearMovementButton, "FearMovementButton", "hl2r_fear_style_movement" },
	{ ShorterSprintButton, "ShorterSprintButton", "hl2r_shorter_sprint" },
	{ LessAmmoButton, "LessAmmoButton", "hl2r_less_ammo" },
	{ EnemyPromotionButton, "EnemyPromotionButton", "hl2r_enemy_promotion" },
	{ RandomWeaponsButton, "RandomWeaponsButton", "hl2r_random_weapons" },
	{ ReducedAssistsButton, "ReducedAssistsButton", "hl2r_reduced_assists" },
	{ SmallerReservesButton, "SmallerReservesButton", "hl2r_smaller_reserves" },
};
//------------------------------------------------------------------------------
// Misc page
//------------------------------------------------------------------------------
class CSubMiscOptionsHL2R : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CSubMiscOptionsHL2R, vgui::PropertyPage);

public:
	CSubMiscOptionsHL2R(vgui::Panel* parent);
	~CSubMiscOptionsHL2R() {}
	
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
// Misc page elements
//------------------------------------------------------------------------------
ComboBox* LightDetailBox;
ComboBox* QuickinfoBox;
ComboBox* MinCrosshairBox;
ComboBox* EpisodicLightBox;
ComboBox* HudHintBox;

// List of all BoxButtons:
BoxButton_t M_BoxButtons[] =
{
	{ LightDetailBox, "LightDetailBox", { "#hl2r_default", "#hl2r_less", "#hl2r_none" }, "hl2r_dynamic_light_level", false },
	{ QuickinfoBox, "QuickinfoBox", { "#hl2r_on", "#hl2r_off" }, "hud_quickinfo", true },
	{ MinCrosshairBox, "MinCrosshairBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_old_crosshair", true },
	{ EpisodicLightBox, "EpisodicLightBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_episodic_flashlight", true },
	{ HudHintBox, "HudHintBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_hudhints", true },
};

Slider* viewRollSlider;

// List of all TickSliders:
TickSlider_t M_TickSliders[] =
{
	{ viewRollSlider, "viewRollSlider", 0, 10, 5, "hl2r_rollangle" },
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
	CSubOptionsGameHL2R* m_pSubOptionsGameHL2R;
	CGameHL2RBasePanel* m_pGameHL2RBasePanel;
	
	CSubMiscOptionsHL2R* m_pSubMiscOptionsHL2R;
};

#endif