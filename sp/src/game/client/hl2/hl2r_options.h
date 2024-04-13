#ifndef hl2r_options_h
#define hl2r_options_h
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyPage.h"

//------------------------------------------------------------------------------
// Check Buttons
//------------------------------------------------------------------------------
struct CheckButton_t
{
	CheckButton	*Button;
	char		Name[32];
	char		Convar[128];
	
	// Initializes a given CheckButton array.
	void InitCheckButtons( Panel *parent, CheckButton_t checkButtons[], int size )
	{
		for ( int i = 0; i < size; i++ )
		{
			checkButtons[i].Button = new CheckButton(parent, checkButtons[i].Name, "");
			
			ConVarRef var( checkButtons[i].Convar );
			checkButtons[i].Button->SetSelected(var.GetBool());
		}
	}
	
	// Updates ConVars related to a given CheckButton array.
	void UpdateConVars( Panel *parent, CheckButton_t checkButtons[], int size )
	{
		for ( int i = 0; i < size; i++ )
		{
			ConVarRef var( checkButtons[i].Convar );
			var.SetValue( checkButtons[i].Button->IsSelected());
		}
	}
};

CheckButton_t CheckButtonRef;

// List of all CheckButtons for the Game Submenu:
CheckButton* ExplosiveCrabsButton;
CheckButton* FearMovementButton;
CheckButton* ShorterSprintButton;
CheckButton* LessAmmoButton;
CheckButton* EnemyPromotionButton;
CheckButton* RandomWeaponsButton;
CheckButton* ReducedAssistsButton;

CheckButton_t G_CheckButtons[] =
{
	{ ExplosiveCrabsButton, "ExplosiveCrabsButton", "hl2r_explosive_crabs" },
	{ FearMovementButton, "FearMovementButton", "hl2r_fear_style_movement" },
	{ ShorterSprintButton, "ShorterSprintButton", "hl2r_shorter_sprint" },
	{ LessAmmoButton, "LessAmmoButton", "hl2r_less_ammo" },
	{ EnemyPromotionButton, "EnemyPromotionButton", "hl2r_enemy_promotion" },
	{ RandomWeaponsButton, "RandomWeaponsButton", "hl2r_random_weapons" },
	{ ReducedAssistsButton, "ReducedAssistsButton", "hl2r_reduced_assists" },
};

//------------------------------------------------------------------------------
// Box Buttons
//------------------------------------------------------------------------------

#define MAX_BOX_ELEMENTS 5
struct BoxButton_t
{
	ComboBox	*Box;
	char		Name[32];
	char 		*ElementNames[MAX_BOX_ELEMENTS];
	char		Convar[128];
	
	// Reverses the index value of elements.
	// EG: [0 1 2] becomes: [2 1 0]. Good for convar control.
	bool		bReversedIndexes;
	
	int GetElementIndex( BoxButton_t mBox, int iElement )
	{
		int element = iElement;
			
		// Handle reversed index's.
		if ( mBox.bReversedIndexes )
		{
			int iNumElements = -1;
			for ( int i = 0; mBox.ElementNames[i] != NULL; i++ )
			{
				iNumElements++;
			}
				
			element = iNumElements - element;
		}
		
		return element;
	}
	
	// Initializes a given BoxButton array.
	void InitBoxButtons( Panel *parent, BoxButton_t boxButtons[], int size )
	{
		for ( int i = 0; i < size; i++ )
		{
			boxButtons[i].Box = new ComboBox(parent, boxButtons[i].Name, MAX_BOX_ELEMENTS, false);
			
			// Handle elements contained in the box:
			for ( int j = 0; boxButtons[i].ElementNames[j] != NULL; j++ )
			{
				boxButtons[i].Box->AddItem( boxButtons[i].ElementNames[j], NULL );
			}
			
			ConVarRef var( boxButtons[i].Convar );
			
			int iValue = GetElementIndex( boxButtons[i], var.GetInt() );
			boxButtons[i].Box->ActivateItem( iValue );
		}
	}
	
	// Updates ConVars related to a given BoxButton array.
	void UpdateConVars( Panel *parent, BoxButton_t boxButtons[], int size )
	{
		for ( int i = 0; i < size; i++ )
		{
			int iValue = GetElementIndex( boxButtons[i], boxButtons[i].Box->GetActiveItem() );
			
			ConVarRef var( boxButtons[i].Convar );
			var.SetValue(iValue);
		}
	}
};

BoxButton_t BoxButtonRef;

// List of all BoxButtons for the Misc Submenu:
ComboBox* LightDetailBox;
ComboBox* QuickinfoBox;
ComboBox* MinCrosshairBox;
ComboBox* EpisodicLightBox;
ComboBox* HudHintBox;

BoxButton_t M_BoxButtons[] =
{
	{ LightDetailBox, "LightDetailBox", { "#hl2r_default", "#hl2r_less", "#hl2r_none" }, "hl2r_dynamic_light_level", false },
	{ QuickinfoBox, "QuickinfoBox", { "#hl2r_on", "#hl2r_off" }, "hud_quickinfo", true },
	{ MinCrosshairBox, "MinCrosshairBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_old_crosshair", true },
	{ EpisodicLightBox, "EpisodicLightBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_episodic_flashlight", true },
	{ HudHintBox, "HudHintBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_hudhints", true },
};

//------------------------------------------------------------------------------
// Tick Sliders
//------------------------------------------------------------------------------
struct TickSlider_t
{
	Slider		*TickSlider;
	char		Name[32];
	int			min, max; // Value range for ConVars
	int			numticks; // Number of visible ticks on the slider.
	char		Convar[128];
	
	// Initializes a given TickSlider array.
	void InitTickSliders( Panel *parent, TickSlider_t tickSliders[], int size )
	{
		for ( int i = 0; i < size; i++ )
		{
			tickSliders[i].TickSlider = new Slider(parent, tickSliders[i].Name);
			tickSliders[i].TickSlider->SetRange(tickSliders[i].min, tickSliders[i].max); 
			tickSliders[i].TickSlider->SetNumTicks(tickSliders[i].numticks);
			
			ConVarRef var( tickSliders[i].Convar );
			tickSliders[i].TickSlider->SetValue(var.GetFloat());
		}
	}
	
	// Updates ConVars related to a given TickSlider array.
	void UpdateConVars( Panel *parent, TickSlider_t tickSliders[], int size )
	{
		for ( int i = 0; i < size; i++ )
		{
			ConVarRef var( tickSliders[i].Convar );
			var.SetValue( tickSliders[i].TickSlider->GetValue());
		}
	}
};

TickSlider_t TickSliderRef;

// List of all TickSliders for the Misc Submenu:
Slider* viewRollSlider;

TickSlider_t M_TickSliders[] =
{
	{ viewRollSlider, "viewRollSlider", 0, 10, 5, "hl2r_rollangle" },
};
//------------------------------------------------------------------------------
// Game page
//------------------------------------------------------------------------------
class CSubOptionsGameHL2R : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CSubOptionsGameHL2R, vgui::PropertyPage);

public:
	CSubOptionsGameHL2R(vgui::Panel* parent);
	~CSubOptionsGameHL2R() {}

	virtual void OnApplyChanges();

protected:
	virtual void OnCommand(const char* pcCommand);
private:
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel);
	
	Button* toggleAllButton;
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
	CSubMiscOptionsHL2R* m_pSubMiscOptionsHL2R;
};

#endif