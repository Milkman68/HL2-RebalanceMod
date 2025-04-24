#ifndef hl2r_baseoptions_h
#define hl2r_baseoptions_h
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/CheckButton.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/ScrollableEditablePanel.h>

//------------------------------------------------------------------------------
// Check Buttons
//------------------------------------------------------------------------------
struct CheckButton_t
{
	char		Name[32];
	char		Convar[128];
	bool		bInvert;
	
	int			Size;
	CheckButton	*Button;
	
	// Initializes a given CheckButton array.
	void InitCheckButtons( Panel *parent, CheckButton_t checkButtons[] )
	{
		for ( int i = 0; i < checkButtons[0].Size; i++ )
		{
			checkButtons[i].Button = new CheckButton(parent, checkButtons[i].Name, "");
			
			ConVarRef var( checkButtons[i].Convar );
			checkButtons[i].Button->SetSelected( checkButtons[i].bInvert ? !var.GetBool() : var.GetBool() );
		}
	}
	
	// Updates ConVars related to a given CheckButton array.
	void UpdateConVars( Panel *parent, CheckButton_t checkButtons[] )
	{
		for ( int i = 0; i < checkButtons[0].Size; i++ )
		{
			ConVarRef var( checkButtons[i].Convar );
			var.SetValue( checkButtons[i].bInvert ? !checkButtons[i].Button->IsSelected() : checkButtons[i].Button->IsSelected() );
		}
	}
};

//------------------------------------------------------------------------------
// Radio Buttons
//------------------------------------------------------------------------------
#define MAX_RADIO_BUTTONS 8
struct RadioButton_t
{
	char		*Names[MAX_RADIO_BUTTONS];
	int			iValueList[MAX_RADIO_BUTTONS];
	char		Convar[128];

	int			Size;
	RadioButton* Buttons[MAX_RADIO_BUTTONS];

	// Initializes a given RadioButton array.
	void InitRadioButtons(Panel* parent, RadioButton_t* radioButtons)
	{
		for (int i = 0; i < radioButtons->Size; i++)
		{
			radioButtons->Buttons[i] = new RadioButton(parent, radioButtons->Names[i], "");

			ConVarRef var(radioButtons->Convar);
			radioButtons->Buttons[i]->SetSelected(iValueList[i] == var.GetInt());
		}
	}

	// Updates ConVars related to a given RadioButton array.
	void UpdateConVars(Panel* parent, RadioButton_t* radioButtons)
	{
		ConVarRef var(radioButtons->Convar);
		for (int i = 0; i < radioButtons->Size; i++)
		{
			if ( radioButtons->Buttons[i]->IsSelected() )
				var.SetValue(iValueList[i]);
		}
	}
};

//------------------------------------------------------------------------------
// Box Buttons
//------------------------------------------------------------------------------

#define MAX_BOX_ELEMENTS 5
struct BoxButton_t
{
	char		Name[32];
	char 		*ElementNames[MAX_BOX_ELEMENTS];
	char		Convar[128];
	
	// Reverses the index value of elements.
	// EG: [0 1 2] becomes: [2 1 0]. Good for convar control.
	bool		bReversedIndexes;
	
	int			Size;
	ComboBox	*Box;
	
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
	void InitBoxButtons( Panel *parent, BoxButton_t boxButtons[] )
	{
		for ( int i = 0; i < boxButtons[0].Size; i++ )
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
	void UpdateConVars( Panel *parent, BoxButton_t boxButtons[] )
	{
		for ( int i = 0; i < boxButtons[0].Size; i++ )
		{
			int iValue = GetElementIndex( boxButtons[i], boxButtons[i].Box->GetActiveItem() );
			
			ConVarRef var( boxButtons[i].Convar );
			var.SetValue(iValue);
		}
	}
};

//------------------------------------------------------------------------------
// Tick Sliders
//------------------------------------------------------------------------------
struct TickSlider_t
{
	char		Name[32];
	int			min, max; // Value range for ConVars
	int			numticks; // Number of visible ticks on the slider.
	float		scale; // Scales the value difference between slider positions.
	char		Convar[128];
	
	int			Size;
	Slider		*TickSlider;
	
	// Initializes a given TickSlider array.
	void InitTickSliders( Panel *parent, TickSlider_t tickSliders[] )
	{
		for ( int i = 0; i < tickSliders[0].Size; i++ )
		{
			tickSliders[i].TickSlider = new Slider(parent, tickSliders[i].Name);
			
			int min = tickSliders[i].min / tickSliders[i].scale;
			int max = tickSliders[i].max / tickSliders[i].scale;
			tickSliders[i].TickSlider->SetRange(min, max); 
			
			tickSliders[i].TickSlider->SetNumTicks(tickSliders[i].numticks);
			ConVarRef var( tickSliders[i].Convar );
			tickSliders[i].TickSlider->SetValue(var.GetFloat() / tickSliders[i].scale);
		}
	}
	
	// Updates ConVars related to a given TickSlider array.
	void UpdateConVars( Panel *parent, TickSlider_t tickSliders[] )
	{
		for ( int i = 0; i < tickSliders[0].Size; i++ )
		{
			ConVarRef var( tickSliders[i].Convar );
			var.SetValue( tickSliders[i].TickSlider->GetValue() * tickSliders[i].scale );
		}
	}
};

static void GetFixedName( char *name, char *key )
{
	// Get our name string.
	char s[32];
	V_strcpy( s, name );
		
	char *ident; 
	ident = Q_strstr( s, key );
		
	// Don't use it if it doesn't contain our keyword.
	if ( ident == NULL )
		return;
		
	// Remove the keyword from our name and turn it lowercase.
	Q_strncpy( ident, "", 4 );
	Q_strlower(s);
		
	V_strcpy( name, s );
}


//------------------------------------------------------------------------------
// Options Panel
//------------------------------------------------------------------------------

class CSubPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CSubPanel, vgui::EditablePanel);

public:

	CSubPanel(vgui::Panel* parent ) : EditablePanel(parent, NULL) {}
	~CSubPanel() {}
	
	// Initialize all our control methods.
	virtual void SubPanelInit( void ) 
	{
		if ( m_CheckButtonList != NULL )
			m_CheckButtonList->InitCheckButtons( this, m_CheckButtonList );

		if (m_RadioButtonList != NULL)
			m_RadioButtonList->InitRadioButtons(this, m_RadioButtonList);
		
		if ( m_BoxButtonList != NULL )
			m_BoxButtonList->InitBoxButtons( this, m_BoxButtonList );
		
		if ( m_TickSliderList != NULL )
			m_TickSliderList->InitTickSliders( this, m_TickSliderList );
	}
	
private:
	// Updates all our control methods.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" )
	{
		if ( m_CheckButtonList != NULL )
			m_CheckButtonList->UpdateConVars( this, m_CheckButtonList );

		if (m_RadioButtonList != NULL)
			m_RadioButtonList->UpdateConVars(this, m_RadioButtonList);
		
		if ( m_BoxButtonList != NULL )
			m_BoxButtonList->UpdateConVars( this, m_BoxButtonList );
		
		if ( m_TickSliderList != NULL )
			m_TickSliderList->UpdateConVars( this, m_TickSliderList );
	}

	// Execute OnControlModified() when any action should enable the apply button.
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel) { OnControlModified(); }
	MESSAGE_FUNC_PTR(OnRadioButtonChecked, "RadioButtonChecked", panel) { OnControlModified(); }
	MESSAGE_FUNC( OnTextChanged, "TextChanged" ) { OnControlModified(); }
	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data ) { OnControlModified(); }
	
	// Enable the apply button.
	MESSAGE_FUNC( OnControlModified, "ControlModified" ) { PostActionSignal(new KeyValues("ApplyButtonEnable")); }
	
public:
	CheckButton_t *m_CheckButtonList = NULL;
	RadioButton_t *m_RadioButtonList = NULL;
	BoxButton_t *m_BoxButtonList = NULL;
	TickSlider_t *m_TickSliderList = NULL;
};
//------------------------------------------------------------------------------
// Scrollable Panel
//------------------------------------------------------------------------------
class CSubScrollablePanel : public vgui::ScrollableEditablePanel
{
	DECLARE_CLASS_SIMPLE(CSubScrollablePanel, vgui::ScrollableEditablePanel);
	
public:
	CSubScrollablePanel::CSubScrollablePanel( vgui::Panel* parent, vgui::EditablePanel* child ) : 
	ScrollableEditablePanel(parent, child, NULL) {}
	
	~CSubScrollablePanel() {}
	
private:
	// Since the main panel sends the apply message to this panel, and not its child panel that has all the actual elements on it,
	// we need to manually send the apply message to the child panel.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" ) { ipanel()->SendMessage(GetChild(0)->GetVPanel(), new KeyValues("ApplyChanges"), GetVPanel()); }
};
//------------------------------------------------------------------------------
// Options Tab
//------------------------------------------------------------------------------
struct OptionsTab_t
{
	char			Name[32];
	
	CheckButton_t	*CheckButtons; // List of all checkButtons in this panel.
	RadioButton_t	*RadioButtons;
	BoxButton_t		*BoxButtons; // List of all boxButtons in this panel.
	TickSlider_t	*TickSliders; // List of all tickSliders in this panel.
	
	// if this is greater than 1, add a scrollbar to this panel and extend it's height down by this percentage.
	float			AdditionalHeightPerc; 
	
	int				Size;
	CSubPanel		*Panel;

	// Parses a given OptionsTab array.
	void ParseOptionsPanels( PropertyDialog *parent, OptionsTab_t OptionsTabs[] )
	{
		for ( int i = 0; i < OptionsTabs[0].Size; i++ )
		{
			if (!OptionsTabs[i].Panel)
				OptionsTabs[i].Panel = new CSubPanel(parent);

			// Pass our control methods to the new panel.
			OptionsTabs[i].Panel->m_CheckButtonList = OptionsTabs[i].CheckButtons;
			OptionsTabs[i].Panel->m_RadioButtonList = OptionsTabs[i].RadioButtons;
			OptionsTabs[i].Panel->m_BoxButtonList = OptionsTabs[i].BoxButtons;
			OptionsTabs[i].Panel->m_TickSliderList = OptionsTabs[i].TickSliders;
			
			// Init
			OptionsTabs[i].Panel->SubPanelInit();
			
			// Get our lowercase name as a string. 
			// This is used for loading the corrosponding .res file and label string.
			char s[32];
			V_strcpy( s, OptionsTabs[i].Name );

			GetFixedName(s, "Page");
			
			OptionsTabs[i].Panel->LoadControlSettings( VarArgs( "resource/ui/hl2r_optionssub%s.res", s ) );
				
			// Parent this panel to a scrollable panel if we need extra height.
			if ( OptionsTabs[i].AdditionalHeightPerc > 1.0f )
			{
				CSubScrollablePanel* m_pScrollablePanel;
				m_pScrollablePanel = new CSubScrollablePanel( parent, OptionsTabs[i].Panel );

				OptionsTabs[i].Panel->SetBounds(0, 0, parent->GetWide(), parent->GetTall() * ( 1.0f + ( OptionsTabs[i].AdditionalHeightPerc / 100 ) ) );
				
				// Add this scrollable panel to our parents page-list.
				parent->AddPage(m_pScrollablePanel, VarArgs( "#hl2r_options_%s", s) );
			}
			else
			{
				// Add this panel to our parents page-list.
				parent->AddPage(OptionsTabs[i].Panel, VarArgs( "#hl2r_options_%s", s) );
			}
		}
	}

	void KillOptionsPanels(OptionsTab_t OptionsTabs[])
	{
		for ( int i = 0; i < OptionsTabs[0].Size; i++ )
		{
			if (OptionsTabs[i].Panel)
				OptionsTabs[i].Panel = NULL;
		}	
	}
};

#endif