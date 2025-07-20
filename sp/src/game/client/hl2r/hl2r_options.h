#ifndef hl2r_options_h
#define hl2r_options_h
#ifdef _WIN32
#pragma once
#endif

#include "hl2r_baseoptions.h"

//------------------------------------------------------------------------------
// Challenge panel
//------------------------------------------------------------------------------
class CSubChallengePanel : public CSubPanel
{
	DECLARE_CLASS_SIMPLE(CSubChallengePanel, CSubPanel);
	
public:
	CSubChallengePanel( vgui::Panel* parent ) : CSubPanel(parent) {}
	~CSubChallengePanel() {}
	
	virtual void SubPanelInit( void );
	virtual void OnCommand(const char* pcCommand);
	
private:
	Button* toggleAllButton;
};
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
// Challenge panel elements
//------------------------------------------------------------------------------
CheckButton_t challenges_CheckButtons[] =
{
	{ "ExplosiveCrabsButton", "hl2r_explosive_crabs", false, 7 }, // The 0th index contains this array's size.
	{ "ShorterSprintButton", "hl2r_shorter_sprint", false },
	{ "EnemyPromotionButton", "hl2r_enemy_promotion", false },
	{ "RandomWeaponsButton", "hl2r_random_weapons", false },
	{ "ReducedAssistsButton", "hl2r_reduced_assists", false },
	{ "SmallerReservesButton", "hl2r_smaller_reserves", false },
};
//------------------------------------------------------------------------------
// Game panel elements
//------------------------------------------------------------------------------
BoxButton_t game_BoxButtons[] =
{
	// The 0th index contains this array's size.
	{ "EpisodicLightBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_episodic_flashlight", true, 2 }, // The 0th index contains this array's size.
	{ "ZoomToggleBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_togglezoom", true },
};

TickSlider_t game_TickSliders[] =
{
	{ "viewRollSlider", 0, 10, 10, 1, "hl2r_rollangle", 2 }, // The 0th index contains this array's size.
	{ "WeaponFOVSlider", 54, 90, 12, 3, "viewmodel_fov" },
};
//------------------------------------------------------------------------------
// Visual panel elements
//------------------------------------------------------------------------------
BoxButton_t visuals_BoxButtons[] =
{
	{ "QuickinfoBox", { "#hl2r_on", "#hl2r_off" }, "hud_quickinfo", true, 5 }, // The 0th index contains this array's size.
	{ "MinCrosshairBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_old_crosshair",  false }, // Renamed to "New Crosshairs" in the options.
	{ "HudHintBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_hudhints", true },
	{ "LightDetailBox", { "#hl2r_default", "#hl2r_less", "#hl2r_none" }, "hl2r_dynamic_light_level", false },
	{ "ProjectedMuzzleflashBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_projected_muzzleflash", true },
};
//------------------------------------------------------------------------------
// Parent panel pages
//------------------------------------------------------------------------------
OptionsTab_t hl2r_OptionsTabs[] =
{
	{ "GamePage",  NULL, NULL, game_BoxButtons, game_TickSliders, 0.0f, 3 }, // The 0th index contains this array's size.
	{ "VisualsPage", NULL, NULL, visuals_BoxButtons, NULL, 0.0f },
	{ "ChallengePage", challenges_CheckButtons, NULL, NULL, NULL, 15.0f },
};

#endif