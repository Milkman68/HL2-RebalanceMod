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

// The purpose of all these is to contain instructions to create elements.
// Not contain the elements themselves

//------------------------------------------------------------------------------
// Check Buttons
//------------------------------------------------------------------------------
struct CheckButton_t
{
	char		Name[32];
	char		Convar[128];
	bool		bInvert;
	
	CheckButton	*panel;
	
	// Initializes a given CheckButton array.
	void Init( Panel *parent )
	{
		panel = new CheckButton(parent, Name, "");

		ConVarRef var(Convar);
		panel->SetSelected(bInvert ? !var.GetBool() : var.GetBool());
	}

	// Updates ConVars related to a given CheckButton array.
	void Update( Panel *parent )
	{
		ConVarRef var(Convar);
		var.SetValue(bInvert ? !panel->IsSelected() : panel->IsSelected());
	}
};

/*//------------------------------------------------------------------------------
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
*/
#if 0
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
#endif
//------------------------------------------------------------------------------
// Tick Sliders
//------------------------------------------------------------------------------
struct TickSlider_t
{
	char		Name[32];
	char		Convar[128];

	int			numticks;		// Number of selectable ticks on the slider.
	int			numindicators;	// Number of visible line indicators on the slider.

	float		min, max; // Value range

	Slider		*panel;
	
	// Initializes a given TickSlider array.
	void Init( Panel *parent )
	{
		panel = new Slider(parent, Name);

		panel->SetRange(0, numticks - 1); 
		panel->SetNumTicks(numindicators - 1);

		ConVarRef var( Convar );
		int iSelectedTick = (int)RemapVal(var.GetFloat(), min, max, 0, numticks);

		panel->SetValue( iSelectedTick );
	}
	
	// Updates ConVars related to a given TickSlider array.
	void Update( Panel *parent )
	{
		ConVarRef var( Convar );
		var.SetValue( RemapVal(panel->GetValue(), 0.0f, numticks - 1, min, max) );
	}
};

enum eControlTypes
{
	TYPE_NULL = 0,

	TYPE_CHECKBOX,
	TYPE_RADIOBUTTON,
	TYPE_BOXBUTTON,
	TYPE_TICKSLIDER,
};
//------------------------------------------------------------------------------
// Control Elements
//------------------------------------------------------------------------------
#define CREATE_NEW_ELEMENT(x, y) y x; V_strcpy_safe(x.Name, controlElements[i].name); V_strcpy_safe(x.Convar, controlElements[i].convar);
#define INIT_NEW_ELEMENT(x, parent) x.Init(parent); controlElements[i].elementPanel = x.panel;
#define ELEMENT_PARAMETER(x) controlElements[i].parameter_##x

struct ControlElement_t
{
public:
	int type;
	char name[128];
	char convar[128];

	char parameter_1[128];char parameter_2[128];char parameter_3[128];char parameter_4[128];char parameter_5[128];
	char parameter_6[128];char parameter_7[128];char parameter_8[128];char parameter_9[128];char parameter_10[128];

	Panel *elementPanel;
	
public:
	// Initializes a given ControlElement array.
	void InitElements( Panel *parent, ControlElement_t controlElements[] )
	{
		// The number of elements is stored in the 0th index of a controlElements array.
		int iNumElements = controlElements[0].type;

		for (int i = 1; i < iNumElements + 1; i++)
		{
			switch( controlElements[i].type )
			{
			case TYPE_CHECKBOX:
				{
					CREATE_NEW_ELEMENT(checkbutton, CheckButton_t);

					// Parameters
					checkbutton.bInvert = GetParamBool(ELEMENT_PARAMETER(1));

					INIT_NEW_ELEMENT(checkbutton, parent);
				}
				break;

			case TYPE_TICKSLIDER:
				{
					CREATE_NEW_ELEMENT(tickslider, TickSlider_t);

					// Parameters
					tickslider.numticks =		GetParamInt(ELEMENT_PARAMETER(1));
					tickslider.numindicators = GetParamInt(ELEMENT_PARAMETER(2));

					tickslider.min = GetParamFloat(ELEMENT_PARAMETER(3));
					tickslider.max = GetParamFloat(ELEMENT_PARAMETER(4));

					INIT_NEW_ELEMENT(tickslider, parent);
				}
				break;
			}
		}
	}
	
	// Updates ConVars related to a given ControlElement array.
	void UpdateElements( Panel *parent, ControlElement_t controlElements[] )
	{

	}

private:
	int GetParamInt( const char *param )
	{
		return atoi(param);
	}

	float GetParamFloat( const char *param )
	{
		return atof(param);
	}

	bool GetParamBool( const char *param )
	{
		bool bTrue = !Q_stricmp(param, "true");

		if ( !bTrue && Q_stricmp(param, "false") )
			DevWarning("ControlElement_t GetParamBool() is neither true or false!\n");

		return bTrue;
	}
};

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
		if ( m_ControlElementList != NULL )
			m_ControlElementList->InitElements( this, m_ControlElementList );
	}

	// Enable the apply button.
	virtual void OnControlModified( Panel *panel )
	{ 
		PostActionSignal(new KeyValues("ApplyButtonEnable")); 
	}
	
private:
	// Updates all our control methods.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" )
	{
		if ( m_ControlElementList != NULL )
			m_ControlElementList->UpdateElements( this, m_ControlElementList );
	}

	// Execute OnControlModified() when any action should enable the apply button.
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel) { OnControlModified(panel); }
	MESSAGE_FUNC_PTR(OnRadioButtonChecked, "RadioButtonChecked", panel) { OnControlModified(panel); }
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel )				{ OnControlModified(panel); }
	MESSAGE_FUNC_PTR( OnSliderMoved, "SliderMoved", panel )				{ OnControlModified(panel); }
	
public:
	ControlElement_t *m_ControlElementList = NULL;
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
// Options Page
//------------------------------------------------------------------------------
struct OptionsPage_t
{
	const char	*pLabel;
	const char	*pResourcePath;
	
	ControlElement_t	*ControlElements; // List of all elements in this panel.
	
	// if this is greater than 1, add a scrollbar to this panel and extend it's height down by this percentage.
	float			AdditionalHeightPerc; 
	
	int				Size;
	CSubPanel		*Panel;

	// Parses a given OptionsPage array.
	void ParseOptionsPanels( PropertyDialog *parent, OptionsPage_t OptionsPages[] )
	{
		int iNumElements = atoi(OptionsPages[0].pLabel);
		for ( int i = 1; i < iNumElements; i++ )
		{
			if (!OptionsPages[i].Panel)
				OptionsPages[i].Panel = new CSubPanel(parent);

			// Pass our control methods to the new panel.
			OptionsPages[i].Panel->m_ControlElementList = OptionsPages[i].ControlElements;
			
			// Init
			OptionsPages[i].Panel->SubPanelInit();
			OptionsPages[i].Panel->LoadControlSettings( OptionsPages[i].pResourcePath );
				
			// Parent this panel to a scrollable panel if we need extra height.
			if ( OptionsPages[i].AdditionalHeightPerc > 1.0f )
			{
				CSubScrollablePanel* m_pScrollablePanel;
				m_pScrollablePanel = new CSubScrollablePanel( parent, OptionsPages[i].Panel );

				OptionsPages[i].Panel->SetBounds(0, 0, parent->GetWide(), parent->GetTall() * ( 1.0f + ( OptionsPages[i].AdditionalHeightPerc / 100 ) ) );
				
				// Add this scrollable panel to our parents page-list.
				parent->AddPage(m_pScrollablePanel, OptionsPages[i].pLabel);
			}
			else
			{
				// Add this panel to our parents page-list.
				parent->AddPage(OptionsPages[i].Panel, OptionsPages[i].pLabel);
			}
		}
	}

	void KillOptionsPanels(OptionsPage_t OptionsPages[])
	{
		for ( int i = 0; i < OptionsPages[0].Size; i++ )
		{
			if (OptionsPages[i].Panel)
				OptionsPages[i].Panel = NULL;
		}	
	}
};

#endif