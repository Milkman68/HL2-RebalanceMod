#ifndef hl2r_baseoptions_h
#define hl2r_baseoptions_h
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Slider.h>

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

#endif